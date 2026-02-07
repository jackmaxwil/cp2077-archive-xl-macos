#pragma once

#include <map>
#include <memory>
#include <set>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#if defined(_WIN32) || defined(_WIN64)
#include <TiltedCore/StlAllocator.hpp>
#include <tsl/hopscotch_map.h>
#include <tsl/hopscotch_set.h>
#endif

namespace Core
{
// Pretty much the same as TiltedCore with StlAllocator,
// but unique ptr allows implicit casting to the base.
// On macOS, we use standard allocators instead.

namespace Detail
{
template<class T>
struct UniqueDeleter
{
    constexpr UniqueDeleter() noexcept = default;

    template<class U>
    requires std::is_base_of_v<T, U>
    UniqueDeleter(const UniqueDeleter<U>& d) noexcept {}

    void operator()(std::conditional_t<std::is_array_v<T>, T, T*> aData)
    {
#if defined(_WIN32) || defined(_WIN64)
        TiltedPhoques::Delete<T>(aData);
#else
        delete aData;
#endif
    }
};
}

#if defined(_WIN32) || defined(_WIN64)
// Windows: Use TiltedPhoques allocators and hopscotch containers
template<class T>
using Vector = std::vector<T, TiltedPhoques::StlAllocator<T>>;

template<class T>
using Set = tsl::hopscotch_set<T, std::hash<T>, std::equal_to<T>, TiltedPhoques::StlAllocator<T>>;

template<class T, class U>
using Map = tsl::hopscotch_map<T, U, std::hash<T>, std::equal_to<T>, TiltedPhoques::StlAllocator<std::pair<T, U>>>;

template<class T, class U>
using SortedMap = std::map<T, U, std::less<T>, TiltedPhoques::StlAllocator<std::pair<const T, U>>>;

#else
// macOS: Use standard containers
template<class T>
using Vector = std::vector<T>;

template<class T>
using Set = std::unordered_set<T>;

template<class T, class U>
using Map = std::unordered_map<T, U>;

template<class T, class U>
using SortedMap = std::map<T, U>;
#endif

template<class T, class Compare = std::less<T>, class Allocator = std::allocator<T>>
using SortedSet = std::set<T, Compare, Allocator>;

template<class T>
using SharedPtr = std::shared_ptr<T>;

template<class T>
using WeakPtr = std::weak_ptr<T>;

template<class T>
using UniquePtr = std::unique_ptr<T, Detail::UniqueDeleter<T>>;

template<typename T>
struct ShareFromThis : public std::enable_shared_from_this<T>
{
    SharedPtr<T> ToShared()
    {
        return std::enable_shared_from_this<T>::shared_from_this();
    }

    WeakPtr<T> ToWeak()
    {
        return std::enable_shared_from_this<T>::weak_from_this();
    }
};

template<typename T, typename... Args>
auto MakeShared(Args&&... aArgs)
{
#if defined(_WIN32) || defined(_WIN64)
    return std::allocate_shared<T>(TiltedPhoques::StlAllocator<T>(), std::forward<Args>(aArgs)...);
#else
    return std::make_shared<T>(std::forward<Args>(aArgs)...);
#endif
}

template<typename T, typename... Args>
auto MakeUnique(Args&&... aArgs)
{
#if defined(_WIN32) || defined(_WIN64)
    return UniquePtr<T>(TiltedPhoques::New<T>(std::forward<Args>(aArgs)...));
#else
    return UniquePtr<T>(new T(std::forward<Args>(aArgs)...));
#endif
}
}
