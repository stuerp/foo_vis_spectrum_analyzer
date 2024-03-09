
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
class GridItem
{
public:
    GridItem(Graph * graph, FLOAT hRatio, FLOAT vRatio) : _Graph(graph), _HRatio(hRatio), _VRatio(vRatio) { }

public:
    Graph * _Graph;
    FLOAT _HRatio;
    FLOAT _VRatio;
};

class Grid
{
public:
    using GridItems = std::vector<GridItem>;

    void Initialize(size_t rowCount, size_t colCount)
    {
        _RowCount = rowCount;
        _ColCount = colCount;
    }

    void Resize(FLOAT width, FLOAT height)
    {
        D2D1_RECT_F Bounds = {  };

        for (size_t i = 0; i < _RowCount; ++i)
        {
            for (size_t j = 0; j < _ColCount; ++j)
            {
                GridItem & gi = _Items[(i * _ColCount) + j];

                FLOAT w = width  * gi._HRatio;
                FLOAT h = height * gi._VRatio;

                Bounds.left    = Bounds.right;
                Bounds.right  += w;
                Bounds.bottom  = Bounds.top + h;

                gi._Graph->Move(Bounds);
            }

            Bounds.top = Bounds.bottom;
            Bounds.right = 0.f;
        }
    }

    void clear()
    {
        _RowCount = 0;
        _ColCount = 0;

        _Items.clear();
    }

    void push_back(const GridItem & gi) { _Items.push_back(gi); }

    GridItems::iterator begin() { return _Items.begin(); }
    GridItems::iterator end()   { return _Items.end(); } 

private:
    size_t _RowCount;
    size_t _ColCount;

    GridItems _Items;
};
