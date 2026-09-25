
/** $VER: Grid.cpp (2026.09.25) P. Stuer - Implements a grid layout. **/

#include "pch.h"

#include "Grid.h"

/// <summary>
/// Initializes this instance.
/// </summary>
void grid_t::Initialize(size_t rowCount, size_t colCount, bool verticalLayout, bool overlapGraphs) noexcept
{
    Reset();

    _RowCount = rowCount;
    _ColCount = colCount;
    _VerticalLayout = verticalLayout;
    _OverlapGraphs = overlapGraphs;
}

/// <summary>
/// Resizes this instance.
/// </summary>
void grid_t::Resize(FLOAT width, FLOAT height) noexcept
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
                // Move the grid item.
                {
                    const auto & gi = _Items[(i * _ColCount) + j];

                    w = width  * gi->_Analysis._GraphOptions->_HRatio;
                    h = height * gi->_Analysis._GraphOptions->_VRatio;

                    if (!_VerticalLayout)
                        Rect.right += w;
                    else
                        Rect.bottom = Rect.top + h;

                    gi->Move(Rect);
                }

                Rect.left = Rect.right;
            }

            Rect.left = 0.f;
            Rect.top = Rect.bottom;
        }
    }
}

/// <summary>
/// Resets this instance.
/// </summary>
void grid_t::Reset() noexcept
{
    for (auto & Item : _Items)
        delete Item;

    _Items.clear();

    _RowCount = 0;
    _ColCount = 0;
}
