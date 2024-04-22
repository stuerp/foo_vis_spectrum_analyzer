
/** $VER: PeakMeterTypes.h (2024.04.22) P. Stuer - Defines the peak meter types. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#define NOMINMAX

#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <Windows.h>

#include <d2d1_2.h>

#include <atlbase.h>

#pragma once

/// <summary>
/// Represents the metrics used to render the gauges.
/// </summary>
struct GaugeMetrics
{
    FLOAT _TotalBarGap;
    FLOAT _TickSize;
    FLOAT _TotalTickSize;

    FLOAT _BarHeight;
    FLOAT _BarWidth;
    FLOAT _TotalBarHeight;
    FLOAT _TotalBarWidth;
    FLOAT _Offset;

    double _dBFSZero;       // Relative position of 0 dBFS, 0.0 .. 1.0
};
