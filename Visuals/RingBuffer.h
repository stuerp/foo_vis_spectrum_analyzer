
/** $VER: RingBuffer.h (2026.09.30) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <Windows.h>

#include <array>
#include <type_traits>
#include <utility>

#pragma once

template<typename T, size_t capacity>
class ring_buffer_t
{
    static_assert(capacity > 0, "Capacity must be greater than zero.");

public:
    constexpr ring_buffer_t() noexcept(std::is_nothrow_default_constructible_v<T>) = default;

    [[nodiscard]]
    constexpr const T & operator[](const size_t index) const noexcept
    {
        assert(index < _Count);
        return _Items[Wrap(_First + index)];
    }

    [[nodiscard]]
    constexpr T & operator[](const size_t index) noexcept
    {
        assert(index < _Count);
        return _Items[Wrap(_First + index)];
    }

    constexpr void Add(const T & item) noexcept(std::is_nothrow_copy_assignable_v<T>)
    {
        AddImpl(item);
    }

    constexpr void Add(T && item) noexcept(std::is_nothrow_move_assignable_v<T>)
    {
        AddImpl(std::move(item));
    }

    [[nodiscard]]
    constexpr const T & First() const noexcept
    {
        assert(_Count != 0);

        return _Items[_First];
    }

    [[nodiscard]]
    constexpr T & First() noexcept
    {
        assert(_Count != 0);

        return _Items[_First];
    }

    [[nodiscard]]
    constexpr const T & Last() const noexcept
    {
        assert(_Count != 0);

        return _Items[Wrap(_First + _Count - 1)];
    }

    [[nodiscard]]
    constexpr T & Last() noexcept
    {
        assert(_Count != 0);

        return _Items[Wrap(_First + _Count - 1)];
    }

    [[nodiscard]]
    constexpr size_t Count() const noexcept
    {
        return _Count;
    }

    [[nodiscard]]
    static constexpr size_t Capacity() noexcept
    {
        return capacity;
    }

    [[nodiscard]]
    constexpr bool IsEmpty() const noexcept
    {
        return (_Count == 0);
    }

    [[nodiscard]]
    constexpr bool IsFull() const noexcept
    {
        return (_Count == capacity);
    }

    constexpr void Reset() noexcept
    {
        _First = 0;
        _Count = 0;
    }

private:
    [[nodiscard]]
    static constexpr size_t Wrap(const size_t index) noexcept
    {
        return index < capacity ? index : index - capacity;
    }

    template<typename U>
    constexpr void AddImpl(U && item) noexcept(std::is_nothrow_assignable_v<T &, U &&>)
    {
        if (_Count < capacity)
        {
            _Items[Wrap(_First + _Count)] = std::forward<U>(item);
            ++_Count;
        }
        else
        {
            _Items[_First] = std::forward<U>(item);
            _First = Wrap(_First + 1);
        }
    }

private:
    size_t _First = 0;
    size_t _Count = 0;
    std::array<T, capacity> _Items{};
};
