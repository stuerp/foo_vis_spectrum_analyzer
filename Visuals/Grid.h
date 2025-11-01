
/** $VER: Grid.h (2024.03.09) P. Stuer - Implements a grid layout. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>

#include <vector>

#include "Graph.h"
#include "Support.h"

/// <summary>
/// Implements a grid layout.
/// </summary>
class grid_item_t
{
public:
    grid_item_t(graph_t * graph, FLOAT hRatio, FLOAT vRatio) : _Graph(graph), _HRatio(hRatio), _VRatio(vRatio) { }

public:
    graph_t * _Graph;
    FLOAT _HRatio;
    FLOAT _VRatio;
};

class grid_t
{
public:
    using grid_items_t = std::vector<grid_item_t>;

    void Initialize(size_t rowCount, size_t colCount)
    {
        _RowCount = rowCount;
        _ColCount = colCount;
    }

    void Resize(FLOAT width, FLOAT height)
    {
        D2D1_RECT_F Rect = { };

        for (size_t i = 0; i < _RowCount; ++i)
        {
            for (size_t j = 0; j < _ColCount; ++j)
            {
                grid_item_t & gi = _Items[(i * _ColCount) + j];

                FLOAT w = width  * gi._HRatio;
                FLOAT h = height * gi._VRatio;

                Rect.left    = Rect.right;
                Rect.right  += w;
                Rect.bottom  = Rect.top + h;

                gi._Graph->Move(Rect);
            }

            Rect.top = Rect.bottom;
            Rect.right = 0.f;
        }
    }

    void clear()
    {
        _RowCount = 0;
        _ColCount = 0;

        _Items.clear();
    }

    void push_back(const grid_item_t & gi) { _Items.push_back(gi); }

    grid_items_t::iterator begin() { return _Items.begin(); }
    grid_items_t::iterator end()   { return _Items.end(); } 

private:
    size_t _RowCount;
    size_t _ColCount;

    grid_items_t _Items;
};
