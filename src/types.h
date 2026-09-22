// Resource type ktid -> engine class name and extension. Same ids in all KTGL games checked.
#pragma once
#include <cstdint>

struct TypeInfo { uint32_t id; const char* name; const char* ext; };

// Returns null for an unknown id.
const TypeInfo* LookupType(uint32_t id);
