#pragma once

namespace Raw::JournalTree
{
constexpr auto ProcessJournalIndex = Core::RawFunc<
    /* addr = */ Red::AddressLib::JournalTree_ProcessJournalIndex,
    /* type = */ void (*)(uintptr_t a1, Red::JobGroup& aJobGroup)>();
}

namespace Raw::JournalRootFolderEntry
{
#ifdef __APPLE__
// macOS: (root x0, handle& x1, JobQueue& x2), see RED4ext.SDK docs/re/world.md.
constexpr auto Initialize = Core::RawFunc<
    /* addr = */ Red::AddressLib::JournalRootFolderEntry_Initialize,
    /* type = */ void (*)(Red::game::JournalRootFolderEntry* aRoot, uintptr_t a2, Red::JobQueue& aJobQueue)>();
#else
constexpr auto Initialize = Core::RawFunc<
    /* addr = */ Red::AddressLib::JournalRootFolderEntry_Initialize,
    /* type = */ void (*)(Red::game::JournalRootFolderEntry* aRoot,
                          uintptr_t a2, uintptr_t a3, Red::JobQueue& aJobQueue)>();
#endif
}
