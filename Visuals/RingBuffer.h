
/** $VER: RingBuffer.h (2023.12.30) P. Stuer **/

#pragma once

#include "framework.h"

#pragma once

template<typename T, size_t size>
class RingBuffer
{
public:
    RingBuffer() : _Curr(0), _Count(0)
    {
        ::memset(_Items, 0, sizeof(T) * size);
    }

    T operator [](size_t index) const
    {
        return _Items[(_Curr + index) % size];
    }

    void Add(T item)
    {
        _Items[(_Curr + _Count) % size] = item;

        if (_Count < size)
            _Count++;
        else
            _Curr = (_Curr + 1) % size;
    }

    T GetFirst() const
    {
        return _Items[_Curr];
    }

    T GetLast() const
    {
        return _Items[(_Curr + (_Count - 1)) % size];
    }

    size_t GetCount() const
    {
        return _Count;
    }

    void Reset()
    {
        _Curr = 0;
        _Count = 0;
    }

private:
    size_t _Curr;
    size_t _Count;
    T _Items[size];
};
