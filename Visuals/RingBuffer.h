
/** $VER: RingBuffer.h (2024.01.17) P. Stuer **/

#pragma once

#include "framework.h"

#pragma once

template<typename T, size_t size>
class RingBuffer
{
public:
    RingBuffer() : _Curr(0), _Count(0), _Items() { }

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

    T First() const
    {
        return _Items[_Curr];
    }

    T Last() const
    {
        return _Items[(_Curr + (_Count - 1)) % size];
    }

    size_t Count() const
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
