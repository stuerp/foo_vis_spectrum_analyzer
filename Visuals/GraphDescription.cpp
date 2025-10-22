
/** $VER: GraphDescription.cpp (2024.03.09) P. Stuer - Describes the layout and setting of a graph. **/

#include "pch.h"

#include "GraphDescription.h"
#include "Support.h"

#pragma hdrstop

/// <summary>
/// Scales the specified value to a relative amplitude between 0.0 and 1.0.
/// </summary>
double graph_description_t::ScaleAmplitude(double value) const
{
    switch (_YAxisMode)
    {
        default:

        case YAxisMode::None:

        case YAxisMode::Decibels:
            return msc::Map(ToDecibel(value), _AmplitudeLo, _AmplitudeHi, 0.0, 1.0);

        case YAxisMode::Linear:
        {
            const double Exponent = 1.0 / _Gamma;

            return msc::Map(::pow(value, Exponent), _UseAbsolute ? 0.0 : ::pow(ToMagnitude(_AmplitudeLo), Exponent), ::pow(ToMagnitude(_AmplitudeHi), Exponent), 0.0, 1.0);
        }
    }
}
