
/** $VER: Grid.h (2026.09.25) P. Stuer - Implements a grid layout. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>

#include <vector>

#include "Graph.h"

class grid_t
{
public:
    void Initialize(size_t rowCount, size_t colCount, bool verticalLayout, bool overlapGraphs) noexcept;
    void Resize(FLOAT width, FLOAT height) noexcept;
    void Reset() noexcept;

    using grid_items_t = std::vector<graph_t *>;

    void push_back(graph_t * graph)
    {
        _Items.push_back(graph);
    }

    grid_items_t::iterator begin()
    {
        return _Items.begin();
    }

    grid_items_t::iterator end()
    {
        return _Items.end();
    } 

private:
    grid_items_t _Items;

    size_t _RowCount { 0 };
    size_t _ColCount { 0 };

    bool _VerticalLayout { false };
    bool _OverlapGraphs { false };
};
