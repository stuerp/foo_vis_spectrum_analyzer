
/** $VER: JSON.h (2023.11.13) P. Stuer **/

#pragma once

#include <stdint.h>

#include <string>
#include <vector>
#include <unordered_map>

#pragma warning(disable:  4514) // Unreferenced inline function has been removed
#pragma warning(disable:  4820) // Padding added
#pragma warning(disable:  5038) // Order of initialization
#pragma warning(disable: 26495) // Padding added

namespace JSON
{

// Forward declarations
class Reader;
class Writer;

/// <summary>
/// Represents a JSON exception.
/// </summary>
class ReaderException : public std::exception
{
public:
    ReaderException() : std::exception(), _ErrorPosition() { }

    ReaderException(const exception & right) : std::exception(right), _ErrorPosition() { }
    ReaderException & operator=(const ReaderException & rhs)
    {
        __super::operator=(rhs);

        return *this;
    }

    virtual ~ReaderException() { };

    ReaderException(const std::wstring & errorMessage, int64_t errorPosition) : std::exception(), _ErrorMessage(errorMessage), _ErrorPosition(errorPosition) { }

    const std::wstring & GetMessage() const { return _ErrorMessage; }
    int64_t GetPosition() const { return _ErrorPosition; }

public:
    static const std::wstring None;

    // String
    static const std::wstring InvalidCharacterInString;
    static const std::wstring UnterminatedString;
    static const std::wstring IncompleteEscapeSequence;
    static const std::wstring UnknownEscapeSequence;
    static const std::wstring HexadecimalEscapeSequenceTooShort;
    static const std::wstring InvalidHexadecimalEscapeSequence;

    // Number
    static const std::wstring InvalidNumber;

    // Boolean
    static const std::wstring InvalidBoolean;

    // Null
    static const std::wstring InvalidNull;

    // Array
    static const std::wstring UnclosedArray;
    static const std::wstring MissingArrayValue;
    static const std::wstring UnexpectedEndOfArray;

    // Object
    static const std::wstring UnclosedObject;
    static const std::wstring MissingValueSeparator;
    static const std::wstring MissingValue;
    static const std::wstring UnexpectedEndOfObject;

private:
    const std::wstring _ErrorMessage;
    const int64_t _ErrorPosition;
};

/// <summary>
/// Represents a JSON value.
/// </summary>
class Value
{
public:
    enum class Type
    {
        Null,
        String,
        Boolean,
        Number,
        Array,
        Object
    };

    explicit Value() noexcept : _Type(Type::Null), _Number() { }

    Value(const Value & other) noexcept : _Type(Type::Null), _Number()
    {
        operator=(other);
    }

    Value & operator=(const Value & other) noexcept
    {
        if (this != &other)
        {
            _Type = other._Type;

            switch (_Type)
            {
                case Type::Null:    break;
                case Type::String:  _String  = other._String; break;
                case Type::Number:  _Number  = other._Number; break;
                case Type::Boolean: _Boolean = other._Boolean; break;
                case Type::Array:   _Array   = other._Array; break;
                case Type::Object:  _Object  = other._Object; break;
            }
        }

        return *this;
    }

    Value(Value && other) noexcept : _Type(Type::Null), _Number()
    {
        if (this == &other)
            return;

        _Type = other._Type;

        switch (_Type)
        {
            case Type::Null:    break;
            case Type::String:  _String  = std::move(other._String); break;
            case Type::Number:  _Number  = other._Number; break;
            case Type::Boolean: _Boolean = other._Boolean; break;
            case Type::Array:   _Array   = std::move(other._Array); break;
            case Type::Object:  _Object  = std::move(other._Object); break;
        }
    }

    Value & operator=(Value && other) noexcept
    {
        if (this != &other)
            *this = std::move(other);

        return *this;
    }

    Value(const wchar_t * value) noexcept : _Type(Type::String), _String(value) { }
    Value(std::wstring value) noexcept : _Type(Type::String), _String(value) { }
    Value(int value) noexcept : _Type(Type::Number), _Number(value) { }
    Value(double value) noexcept : _Type(Type::Number), _Number(value) { }
    Value(bool value) noexcept : _Type(Type::Boolean), _Boolean(value) { }

    virtual ~Value() noexcept { }

    #pragma region(Assignment operators)
    Value & operator=(const std::wstring rhs) noexcept { return operator()(rhs); }
    Value & operator=(int rhs) noexcept { return operator()(rhs); }
    Value & operator=(double rhs) noexcept { return operator()(rhs); }
    Value & operator=(bool rhs) noexcept { return operator()(rhs); }

    Value & operator()(const std::wstring rhs) noexcept
    {
        _Type = Type::String;
        _String = rhs;

        return *this;
    }

    Value & operator()(int rhs) noexcept
    {
        _Type = Type::Number;
        _Number = (double) rhs;

        return *this;
    }

    Value & operator()(double rhs) noexcept
    {
        _Type = Type::Number;
        _Number = rhs;

        return *this;
    }

    Value & operator()(bool rhs) noexcept
    {
        _Type = Type::Boolean;
        _Boolean = rhs;

        return *this;
    }
    #pragma endregion

    #pragma region(Conversion operators)
    operator const wchar_t *() const noexcept
    {
        return _String.c_str();
    }

    operator const std::wstring &() const noexcept
    {
        return _String;
    }

    operator int() const noexcept
    {
        return (int) _Number;
    }

    operator double() const noexcept
    {
        return _Number;
    }

    operator bool() const noexcept
    {
        return _Boolean;
    }

    operator const std::vector<Value> &() const noexcept
    {
        return _Array;
    }

    operator const std::unordered_map<std::wstring, Value> &() const noexcept
    {
        return _Object;
    }
    #pragma endregion

    #pragma region(Comparison operators)
    bool operator!() const noexcept
    {
        return _Type != Type::Null;
    }

    bool operator==(const Value & other) const noexcept
    {
         if (_Type != other._Type)
            return false;

        switch (_Type)
        {
            case Type::Null:    return true;
            case Type::String:  return _String  == other._String;
            case Type::Number:  return _Number  == other._Number;
            case Type::Boolean: return _Boolean == other._Boolean;
            case Type::Array:   return _Array   == other._Array;
            case Type::Object:  return _Object  == other._Object;
        }
    }

    bool operator!=(const Value & other) const noexcept
    {
         if (_Type != other._Type)
            return false;

        switch (_Type)
        {
            case Type::Null:    return false;
            case Type::String:  return _String  != other._String;
            case Type::Number:  return _Number  != other._Number;
            case Type::Boolean: return _Boolean != other._Boolean;
            case Type::Array:   return _Array   != other._Array;
            case Type::Object:  return _Object  != other._Object;
        }
    }

    bool operator<=(const Value & other) const noexcept
    {
         if (_Type != other._Type)
            return false;

        switch (_Type)
        {
            case Type::Null:    return true;
            case Type::String:  return _String  <= other._String;
            case Type::Number:  return _Number  <= other._Number;
            case Type::Boolean: return _Boolean <= other._Boolean;
            case Type::Array:   return true;
            case Type::Object:  return true;
        }
    }

    bool operator>=(const Value & other) const noexcept
    {
         if (_Type != other._Type)
            return false;

        switch (_Type)
        {
            case Type::Null:    return false;
            case Type::String:  return _String  >= other._String;
            case Type::Number:  return _Number  >= other._Number;
            case Type::Boolean: return _Boolean >= other._Boolean;
            case Type::Array:   return false;
            case Type::Object:  return false;
        }
    }

    bool operator<(const Value & other) const noexcept
    {
         if (_Type != other._Type)
            return false;

        switch (_Type)
        {
            case Type::Null:    return false;
            case Type::String:  return _String  < other._String;
            case Type::Number:  return _Number  < other._Number;
            case Type::Boolean: return _Boolean < other._Boolean;
            case Type::Array:   return false;
            case Type::Object:  return false;
        }
    }

    bool operator>(const Value & other) const noexcept
    {
         if (_Type != other._Type)
            return false;

        switch (_Type)
        {
            case Type::String:  return _String  > other._String;
            case Type::Number:  return _Number  > other._Number;
            case Type::Boolean: return _Boolean > other._Boolean;
            case Type::Null:    return false;
            case Type::Array:   return false;
            case Type::Object:  return false;
        }
    }
    #pragma endregion

    #pragma region(Subscripting operators)
    Value & operator[](int index) noexcept
    {
        return _Array[(size_t) index];
    }

    Value & operator[](const wchar_t * index) noexcept
    {
        return _Object[index];
    }
    #pragma endregion

    void SetType(Type type) noexcept;

    bool IsNull() const noexcept { return _Type == Type::Null; }
    bool IsString() const noexcept { return _Type == Type::String; }
    bool IsNumber() const noexcept { return _Type == Type::Number; }
    bool IsTrue() const noexcept { return _Type == Type::Boolean && _Boolean; }
    bool IsFalse() const noexcept { return _Type == Type::Boolean && !_Boolean; }

    bool Contains(const wchar_t * index)
    {
        return _Object.contains(index);
    }

private:
    std::wstring _String;
    std::vector<Value> _Array;
    std::unordered_map<std::wstring, Value> _Object;
    union
    {
        double _Number;
        bool _Boolean;
    };

    Type _Type;

    friend Reader;
    friend Writer;
};

/// <summary>
/// Implements a JSON reader.
/// </summary>
class Reader
{
public:
    enum class State
    {
        Ready,

        ReadingArray,
        ReadingObject
    };

    Reader() : Reader((const wchar_t *) nullptr) { }

    Reader(const wchar_t * text) : _Head(text), _Curr(text), _State(State::Ready) { }

    Reader(const Reader &) = delete;
    Reader & operator=(const Reader &) = delete;
    Reader(Reader &&) = delete;

    ~Reader() { }

    bool Read(Value & value);

private:
    const wchar_t * _Head;
    const wchar_t * _Curr;

    State _State;
};

/// <summary>
/// Implements a JSON writer.
/// </summary>
class Writer
{
public:
    Writer() : _Level() { }
    Writer(const Value & value) : _Value(value), _Level() { }
    ~Writer() { }

    bool Write(std::wstring & text) noexcept;

private:
    std::wstring ToString(const Value & value) noexcept;
    const std::wstring Indent() const noexcept
    {
        return std::wstring(_Level * 2, ' ');
    }

private:
    Value _Value;
    size_t _Level;
};

}
