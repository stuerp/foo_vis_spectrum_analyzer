
/** $VER: Toggle.h (2026.07.22) P. Stuer - Adaped from foobar2000 SDK **/

#pragma once

#pragma once

/// <summary>
/// Implements a toggle that returns a variable to its previous value when the toggle goes out of scope.
/// </summary>
template<typename T> class toggle_t
{
public:
    template<typename U> toggle_t(T & variable, U && value) : _Variable(variable)
    {
        _OldValue = std::move(_Variable);
        _Variable = std::forward<U>(value);
    }

    toggle_t(const toggle_t &) = delete;
    void operator=(const toggle_t &) = delete;

    ~toggle_t()
    {
        _Variable = std::move(_OldValue);
    }

private:
    T & _Variable;
    T _OldValue;
};
