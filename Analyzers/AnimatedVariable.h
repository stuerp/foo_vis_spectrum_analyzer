
/** $VER: AnimatedVariable.h (2024.03.27) P. Stuer **/

#pragma once

template <class T> class AnimatedVariable
{
public:
    AnimatedVariable(T sourceValue = T(), T targetValue = T(), T stepValue = T()) : _SourceValue(sourceValue), _TargetValue(targetValue), _StepValue(stepValue) { }

    void SetTargetValue(T targetValue) noexcept
    {
        _TargetValue = targetValue;
    }

    T GetValue() noexcept
    {
        if (_StepValue > T())
        {
            if (_SourceValue < _TargetValue) // Don't test for equality.
            {
                _SourceValue += _StepValue;

                return _SourceValue;
            }
            else
                return _TargetValue;
        }
        else
        {
            if (_SourceValue > _TargetValue) // Don't test for equality.
            {
                _SourceValue += _StepValue;

                return _SourceValue;
            }
            else
                return _TargetValue;
        }
    }

private:
    T _SourceValue;
    T _TargetValue;
    T _StepValue;
};
