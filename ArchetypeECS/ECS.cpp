#include "ECS.h"

ECS::IComponentColumn* ECS::Archetype::Col(size_t compID) {
	auto it = compToCol.find(compID);
	assert(it != compToCol.end() && "Archetype lacks that component.");
	return columns[it->second].get();
}

ECS::Archetype* ECS::ArchetypeRegistry::GetOrCreate(Signature sig) {
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

void ECS::ArchetypeRegistry::RemoveRow(Archetype* arch, size_t row) {
	const size_t last = arch->entityIDs.size() - 1;
	for (auto& col : arch->columns) col->SwapRemove(row);

	EntityID moved = arch->entityIDs[last];
	if (row != last) {
		arch->entityIDs[row] = moved;
		locations[moved].row = row;
	}
	arch->entityIDs.pop_back();
}

ECS::EntityID ECS::ArchetypeRegistry::CreateEntity() {
	EntityID e = nextEntity++;
	Archetype* empty = GetOrCreate(Signature{});//signature 0: no columns
	locations[e] = { empty, empty->entityIDs.size() };
	empty->entityIDs.push_back(e);
	return e;
}

void ECS::ArchetypeRegistry::DestroyEntity(EntityID e) {
	auto it = locations.find(e);
	if (it == locations.end()) return;
	RemoveRow(it->second.archetype, it->second.row);
	locations.erase(it);
}

bool ECS::ArchetypeRegistry::Has(EntityID e) const { return locations.count(e) != 0; }
size_t ECS::ArchetypeRegistry::ArchetypeCount() const { return archetypes.size(); }
const std::vector<std::unique_ptr<ECS::Archetype>>& ECS::ArchetypeRegistry::GetArchetypes() const { return archetypes; }