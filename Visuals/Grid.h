
/** $VER: Grid.h (2026.04.19) P. Stuer - Implements a grid layout. **/

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
    void Initialize(size_t rowCount, size_t colCount, bool verticalLayout, bool overlapGraphs) noexcept
    {
        _RowCount = rowCount;
        _ColCount = colCount;
        _VerticalLayout = verticalLayout;
        _OverlapGraphs = false;//overlapGraphs;
    }

    void Resize(FLOAT width, FLOAT height) noexcept
    {
        if (_OverlapGraphs)
        {
            const D2D1_RECT_F Rect = { 0.f, 0.f, width, height };

            for (auto & Item : _Items)
                Item->Move(Rect);
        }
        else
        {
            D2D1_RECT_F Rect = { };

            FLOAT w = 0.f;
            FLOAT h = 0.f;

            for (size_t i = 0; i < _RowCount; ++i)
            {
                if (!_VerticalLayout)
                    Rect.bottom = height;
                else
                    Rect.right = width;

                for (size_t j = 0; j < _ColCount; ++j)
                {
                    const auto & g = _Items[(i * _ColCount) + j];

                    w = width  * g->_Analysis._GraphDescription->_HRatio;
                    h = height * g->_Analysis._GraphDescription->_VRatio;

                    if (!_VerticalLayout)
                        Rect.right += w;
                    else
                        Rect.bottom = Rect.top + h;

                    g->Move(Rect);

                    Rect.left = Rect.right;
                }

                Rect.left = 0.f;
                Rect.top = Rect.bottom;
            }
        }
    }

    void Clear() noexcept
    {
        _RowCount = 0;
        _ColCount = 0;

        for (auto & Item : _Items)
            delete Item;

        _Items.clear();
    }

    void push_back(graph_t * g)
    {
        _Items.push_back(g);
    }

    using grid_items_t = std::vector<graph_t *>;

    grid_items_t::iterator begin()
    {
        return _Items.begin();
    }

    grid_items_t::iterator end()
    {
        return _Items.end();
    } 

private:
    size_t _RowCount;
    size_t _ColCount;

    grid_items_t _Items;

    bool _VerticalLayout;
    bool _OverlapGraphs;
};
