#pragma once

#include <WinBase.h>

template<typename T, UINT size>
class RingBuffer
{
public:
    RingBuffer() : _StartIndex(0), _ItemCount(0)
    {
        ::memset(_Items, 0, sizeof(T) * size);
    }

    void Add(T item)
    {
        _Items[(_StartIndex + _ItemCount) % size] = item;

        if (_ItemCount < size)
            _ItemCount++;
        else
            _StartIndex = (_StartIndex + 1) % size;
    }

    T GetFirst() const
    {
        return _Items[_StartIndex];
    }

    T GetLast() const
    {
        return _Items[(_StartIndex + _ItemCount-1) % size];
    }

    T GetCount() const
    {
        return _ItemCount;
    }

    void Reset()
    {
        _StartIndex = 0;
        _ItemCount = 0;
    }

private:
    UINT _StartIndex;
    UINT _ItemCount;
    T _Items[size];
};
