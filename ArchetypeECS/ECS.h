#pragma once
#include "raylib.h"
#include <cassert> 
#include <queue>
#include <vector>
#include <array>
#include <unordered_map>
#include <bitset>
#include <memory>
#include <cstdint>// std::uint32_t || std::uint8_t
#include <utility> // Required for std::move
#include <functional>

namespace ECS {

	using EntityID = std::uint32_t;
	const EntityID MAX_ENTITIES = 20'000;
	constexpr size_t MAX_COMPONENTS = 64;
	using Signature = std::bitset<MAX_COMPONENTS>;

	inline size_t GetNextComponentID() {
		static size_t id = 0;
		return id++;
	}
	template<typename T>
	static size_t GetComponentID() {
		static size_t id = GetNextComponentID();
		return id;
	}

	//-------------------------------------//
		   // Base component structs //
	//-------------------------------------//
	struct Position {
		float x, y;
	};

	struct Velocity {
		float x, y;
	};

	struct Boids {
		float ax = 0, ay = 0;
	};

	struct Transform2D {
		Vector2 position = { 0,0 };
		Vector2 velocity = { 0,0 };
	};

	struct Control {
		int contorlledBy;
	};

	struct Interactable {
		bool selected = false;
	};

	struct Sprite {
		Texture* texture = nullptr;
	};

	//-------------------------------------//
			// Component Column // 
	//-------------------------------------//
	struct IComponentColumn {
		virtual ~IComponentColumn() = default;
		virtual void MoveTo(size_t row, IComponentColumn* dest) = 0;
		virtual void SwapRemove(size_t row) = 0;
		virtual std::unique_ptr<IComponentColumn> CloneEmpty() const = 0;
		virtual size_t Size() const = 0;
	};

	template<typename T>
	class ComponentColumn : public IComponentColumn {
	public:

		std::vector<T> data;

		void MoveTo(size_t row, IComponentColumn* dest) override {
			auto* typedDest = static_cast<ComponentColumn<T>*>(dest);
			typedDest->data.push_back(std::move(data[row]));
		}

		void SwapRemove(size_t row) override {
			const size_t last = data.size() - 1;
			if (row != last) data[row] = std::move(data[last]);//Guard self-move-assign
			data.pop_back();
		}

		std::unique_ptr<IComponentColumn> CloneEmpty() const override {
			return std::make_unique<ComponentColumn<T>>();
		}

		size_t Size() const override { return data.size(); }
	};

	struct Archetype {
		Signature signature;
		std::vector<EntityID> entityIDs;
		std::vector<std::unique_ptr<IComponentColumn>> columns;//dense: one per set bit
		std::unordered_map<size_t, size_t> compToCol;//componentID -> column index

		IComponentColumn* Col(size_t compID) {
			auto it = compToCol.find(compID);
			assert(it != compToCol.end() && "Archetype lacks that component.");
			return columns[it->second].get();
		}

		template<typename T>
		ComponentColumn<T>* TypedCol() {
			return static_cast<ComponentColumn<T>*>(Col(GetComponentID<T>()));
		}
	};

	class ArchetypeRegistry {
		std::vector<std::unique_ptr<Archetype>> archetypes;
		std::unordered_map<Signature, Archetype*> bySignature;//O(1) GetOrCreate

		// componentID -> factory that makes an empty column of the right type.
		// Populated by RegisterComponent<T>() so GetOrCreate can build columns 
		// from a bare signature (bits) without knowing the concrete types.
		std::unordered_map<size_t, std::function<std::unique_ptr<IComponentColumn>()>> factories;

		struct EntityLocation {
			Archetype* archetype;
			size_t row;
		};

		std::unordered_map<EntityID, EntityLocation> locations;
		EntityID nextEntity = 0;

		Archetype* GetOrCreate(Signature sig) {
			auto it = bySignature.find(sig);
			if (it != bySignature.end()) return it->second;

			auto arch = std::make_unique<Archetype>();
			arch->signature = sig;
			for (size_t id = 0; id < MAX_COMPONENTS; ++id) {
				if (!sig.test(id)) continue;
				auto f = factories.find(id);
				assert(f != factories.end() && "Component used in a signature was never registered.");
				arch->compToCol[id] = arch->columns.size();
				arch->columns.push_back(f->second());
			}

			Archetype* raw = arch.get();
			bySignature[sig] = raw;
			archetypes.push_back(std::move(arch));
			return raw;
		}

		void RemoveRow(Archetype* arch, size_t row) {
			const size_t last = arch->entityIDs.size() - 1;
			for (auto& col : arch->columns) col->SwapRemove(row);

			EntityID moved = arch->entityIDs[last];
			if (row != last) {
				arch->entityIDs[row] = moved;
				locations[moved].row = row;
			}
			arch->entityIDs.pop_back();
		}

	public:
		template<typename T>
		void RegisterComponent() {
			size_t id = GetComponentID<T>();
			factories[id] = [] { return std::unique_ptr<IComponentColumn>(std::make_unique<ComponentColumn<T>>()); };
		}

		EntityID CreateEntity() {
			EntityID e = nextEntity++;
			Archetype* empty = GetOrCreate(Signature{});//signature 0: no columns
			locations[e] = { empty, empty->entityIDs.size() };
			empty->entityIDs.push_back(e);
			return e;
		}

		template<typename T> 
		void AddComponent(EntityID e, T value) {
			const size_t compID = GetComponentID<T>();
			EntityLocation loc = locations.at(e);
			Archetype* src = loc.archetype;
			const size_t srcRow = loc.row;

			if (src->signature.test(compID)) {//Already present -> overwrite in place
				src->TypedCol<T>()->data[srcRow] = std::move(value);
				return;
			}

			Signature newSig = src->signature; newSig.set(compID);
			Archetype* dst = GetOrCreate(newSig);
			const size_t dstRow = dst->entityIDs.size();

			//Move every existing comp from src's row into dst's matching column
			for (auto& [cid, colIdx] : src->compToCol)
				src->columns[colIdx]->MoveTo(srcRow, dst->Col(cid));

			//Appeand the new comp and the entityID
			dst->TypedCol<T>()->data.push_back(std::move(value));
			dst->entityIDs.push_back(e);

			//Remove the now-vacated row from src (+ fix the swapped neighbour)
			RemoveRow(src, srcRow);

			//Record the entity's new home 
			locations[e] = { dst, dstRow };
		}

		template<typename T>
		void RemoveComponent(EntityID e) {
			const size_t compID = GetComponentID<T>();
			EntityLocation loc = locations.at(e);
			Archetype* src = loc.archetype;
			const size_t srcRow = loc.row;
			if (!src->signature.test(compID)) return;//nothing to remove

			Signature newSig = src->signature; newSig.reset(compID);
			Archetype* dst = GetOrCreate(newSig);
			const size_t dstRow = dst->entityIDs.size();

			//Move everything EXCEPT the removed comp; its element is simply
			//dropped by RemoveRow's swap-remove below
			for (auto& [cid, colIdx] : src->compToCol) {
				if (cid == compID) continue;
				src->columns[colIdx]->MoveTo(srcRow, dst->Col(cid));
			}

			dst->entityIDs.push_back(e);
			RemoveRow(src, srcRow);
			locations[e] = { dst, dstRow };
		}

		void DestroyEntity(EntityID e) {
			auto it = locations.find(e);
			if (it == locations.end()) return;
			RemoveRow(it->second.archetype, it->second.row);
			locations.erase(it);
		}

		template<typename T>
		T& GetComponent(EntityID e) {
			EntityLocation loc = locations.at(e);
			return loc.archetype->TypedCol<T>()->data[loc.row];
		}

		bool Has(EntityID e) const { return locations.count(e) != 0; }
		size_t ArchetypeCount() const { return archetypes.size(); }
		const std::vector<std::unique_ptr<Archetype>>& GetArchetypes() const { return archetypes; }

		//Typed multi-component iteration: matching archetypes stream their 
		//columns in lockstep. fn is called as fn(EntityID, Comps&...)
		template<typename... Comps, typename Fn>
		void ForEach(Fn fn) {
			Signature required;
			(required.set(GetComponentID<Comps>()), ...);

			for (auto& archPtr : archetypes) {
				Archetype& arch = *archPtr;
				if ((arch.signature & required) != required || arch.entityIDs.empty())
					continue;

				auto cols = std::make_tuple(arch.TypedCol<Comps>()...);//one fetch per column|component
				const std::size_t n = arch.entityIDs.size();
				for (size_t row = 0; row < n; ++row) {
					std::apply([&](auto*... col) {
						fn(arch.entityIDs[row], col->data[row]...);//contiguous, no lookup
					}, cols);
				}
			}
		}
	};
}