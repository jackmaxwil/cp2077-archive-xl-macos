#pragma once

namespace Raw::WorldNodeInstance
{
using Transform = Core::OffsetPtr<0x30, Red::Transform>;
using Scale = Core::OffsetPtr<0x50, Red::Vector3>;
using Node = Core::OffsetPtr<0x60, Red::Handle<Red::worldNode>>;
}

namespace Raw::WorldNodeRegistry
{
#ifdef __APPLE__
// macOS: the Handle comes back through x8 (vtable 0x10719FA60 +0x198 -> 0x1033425AC: `mov x19, x8`, x1 = node ID).
constexpr auto FindNode = Core::RawVFunc<
    /* addr = */ 0x190,
    /* type = */ Red::Handle<Red::worldINodeInstance> (Red::worldNodeInstanceRegistry::*)(uint64_t aNodeID)>();
#else
constexpr auto FindNode = Core::RawVFunc<
    /* addr = */ 0x190,
    /* type = */ void (Red::worldNodeInstanceRegistry::*)(Red::Handle<Red::worldINodeInstance>& aOut,
                                                          uint64_t aNodeID)>();
#endif
}
