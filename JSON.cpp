
/** $VER: JSON.cpp (2023.08.15) P. Stuer **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4710 4711 4820 5045 ALL_CPPCORECHECK_WARNINGS)

#include "JSON.h"

#pragma hdrstop

using namespace JSON;

#pragma region(Value methods)
void Value::SetType(Type type) noexcept
{
    _Type = type;

    switch (_Type)
    {
        case Type::Null: _Number = 0.f; break;
        case Type::String: _String.clear(); break;
        case Type::Number: _Number = NAN; break;
        case Type::Boolean: _Boolean = false; break;
        case Type::Array: _Array.clear(); break;
        case Type::Object: _Object.clear(); break;
    };
}
#pragma endregion

#pragma region(Reader methods)
bool Reader::Read(Value & value)
{
    if (_Curr == nullptr)
        return false;

    while (*_Curr)
    {
        while (isspace(*_Curr))
            _Curr++;

        if (*_Curr == '\0')
            return false;

        switch (*_Curr)
        {
            case '\"':
            {
               _Curr++;

                value.SetType(Value::Type::String);

                while (*_Curr)
                {
                    if (*_Curr < ' ')
                        throw ReaderException(ReaderException::InvalidCharacterInString, _Curr - _Head);

                    switch (*_Curr)
                    {
                        case '\\':
                        {
                            _Curr++;

                            switch (*_Curr)
                            {
                                case '\0':
                                    throw ReaderException(ReaderException::IncompleteEscapeSequence, _Curr - _Head);

                                case '0' : value._String.push_back('\0'); break;
                                case 'b' : value._String.push_back('\b'); break;
                                case 't' : value._String.push_back('\t'); break;
                                case 'n' : value._String.push_back('\n'); break;
                                case 'f' : value._String.push_back('\f'); break;
                                case 'r' : value._String.push_back('\r'); break;
                                case '\\': value._String.push_back('\\'); break;
                                case '"' : value._String.push_back('"');  break;
                                case '/' : value._String.push_back('/');  break;
                                case 'u' :
                                {
                                    _Curr++;

                                    if (!(::isxdigit(_Curr[0]) && ::isxdigit(_Curr[1]) && ::isxdigit(_Curr[2]) && ::isxdigit(_Curr[3])))
                                        throw ReaderException(ReaderException::HexadecimalEscapeSequenceTooShort, _Curr - _Head);

                                    wchar_t Code = 0;

                                    for (int i = 0; *_Curr && i < 4; ++_Curr, ++i)
                                    {
                                        Code *= 16;

                                        if ((*_Curr >= '0') && (*_Curr <= '9'))
                                            Code += *_Curr - '0';
                                        else
                                        {
                                            int Digit = ::toupper(*_Curr);

                                            if ((Digit >= 'A') && (Digit <= 'F'))
                                                Code += (uint16_t) (10 + (Digit - 'A'));
                                            else
                                                throw ReaderException(ReaderException::InvalidHexadecimalEscapeSequence, _Curr - _Head);
                                        }
                                    }

                                    _Curr--;

                                    value._String.push_back(Code);
                                    break;
                                }

                                default:
                                    throw ReaderException(ReaderException::UnknownEscapeSequence, _Curr - _Head);
                            }
                            break;
                        }

                        case '"':
                        {
                            _Curr++;

                            return true;
                        }

                        default:
                            value._String.push_back(*_Curr);
                    }

                    _Curr++;
                }

                throw ReaderException(ReaderException::UnterminatedString, _Curr - _Head);
            }

            case 't':
            {
                if (::wcsncmp(_Curr, L"true", 4) != 0)
                    throw ReaderException(ReaderException::InvalidBoolean, _Curr - _Head);

                value = true;
                _Curr += 4;

                return true;
            }

            case 'f':
            {
                if (::wcsncmp(_Curr, L"false", 5) != 0)
                    throw ReaderException(ReaderException::InvalidBoolean, _Curr - _Head);

                value = false;
                _Curr += 5;

                return true;
            }

            case 'n':
            {
                if (::wcsncmp(_Curr, L"null", 4) != 0)
                    throw ReaderException(ReaderException::InvalidNull, _Curr - _Head);

                value._Type = Value::Type::Null;
                _Curr += 4;

                return true;
            }

            case '[':
            {
                _State = State::ReadingArray;

                value.SetType(Value::Type::Array);

                {
                    Reader Reader(++_Curr);

                    Value Value;

                    bool Success = Reader.Read(Value);
                    bool ExpectValue = false;

                    while (Success)
                    {
                        value._Array.push_back(Value);

                        while (isspace(*Reader._Curr))
                            Reader._Curr++;

                        if (*Reader._Curr == ',')
                        {
                            Reader._Curr++;
                            ExpectValue = true;
                        }
                        else
                            ExpectValue = false;

                        Success = Reader.Read(Value);
                    }

                    _Curr = Reader._Curr - 1; // Position cursor before the closing ']'.

                    if (ExpectValue)
                        throw ReaderException(ReaderException::MissingArrayValue, _Curr - _Head);
                }    
                break;
            }

            case ']':
            {
                if (_State != State::ReadingArray)
                    return false;

                _Curr++;
                _State = State::Ready;

                return true;
            }

            case '{':
            {
                _State = State::ReadingObject;

                value.SetType(Value::Type::Object);

                {
                    Reader Reader(++_Curr);

                    Value Name;

                    bool Success = Reader.Read(Name) && Name.IsString();

                    while (Success)
                    {
                        while (isspace(*Reader._Curr))
                            Reader._Curr++;

                        if (*Reader._Curr != ':')
                        {
                            _Curr = Reader._Curr;
                            throw ReaderException(ReaderException::MissingValueSeparator, _Curr - _Head);
                        }

                        Reader._Curr++;

                        Value Value;

                        if (!Reader.Read(Value))
                        {
                            _Curr = Reader._Curr;
                            throw ReaderException(ReaderException::MissingValue, _Curr - _Head);
                        }

                        value._Object.insert({ Name, Value });

                        Success = Reader.Read(Name) && Name.IsString();
                    }

                    _Curr = Reader._Curr - 1; // Position cursor before the closing '}'.
                }    
                break;
            }

            case '}':
            {
                if (_State != State::ReadingObject)
                    return false;

                _Curr++;
                _State = State::Ready;

                return true;
            }

            case ',':
                break;

            default: // Assume anything else is a number.
            {
                if (!(isdigit(*_Curr) || (*_Curr == '-')))
                    throw ReaderException(ReaderException::InvalidNumber, _Curr - _Head);

                wchar_t * Tail;

                value = ::wcstod(_Curr, &Tail);

                if (::wcscmp(_Curr, Tail) == 0)
                    return false; // No covered by any test case.

                _Curr = Tail;
                return true;
            }
        }

        _Curr++;
    }

    if (_State == State::ReadingArray)
        throw ReaderException(ReaderException::UnclosedArray, _Curr - _Head);

    if (_State == State::ReadingObject)
        throw ReaderException(ReaderException::UnclosedObject, _Curr - _Head);

    return false; // Stop reading
}
#pragma endregion

#pragma region(Writer methods)
bool Writer::Write(std::wstring & text) noexcept
{
    text = ToString(_Value);

    return true;
}

/// <summary>
/// Returns a string representation of this instance.
/// </summary>
std::wstring Writer::ToString(const Value & value) noexcept
{
    std::wstring Text;

    switch (value._Type)
    {
        case Value::Type::Null:
            Text += L"null";
            break;

        case Value::Type::String:
            Text += std::wstring(L"\"");

            for (auto Code : value._String)
            {
                switch (Code)
                {
                    case '\0': Text += LR"(\0)"; break;
                    case '\b': Text += LR"(\b)"; break;
                    case '\t': Text += LR"(\t)"; break;
                    case '\n': Text += LR"(\n)"; break;
                    case '\f': Text += LR"(\f)"; break;
                    case '\r': Text += LR"(\r)"; break;
                    case '"' : Text += LR"(\")"; break;
                    case '/' : Text += LR"(\/")"; break;
                    case '\\': Text += LR"(\\")"; break;
                    default:
                    {
                        if ((' ' <= Code) && (Code <= 127))
                            Text += Code;
                        else
                        {
                            wchar_t CodePoint[8];

                            ::swprintf_s(CodePoint, LR"(\u%04X)", Code);

                            Text += CodePoint;
                        }
                    }
                }
            }

             Text += L"\"";
            break;

        case Value::Type::Number:
            Text += std::to_wstring(value._Number);
            break;

        case Value::Type::Boolean:
            Text += value._Boolean ? L"true" : L"false";
            break;

        case Value::Type::Array:
        {
            Text += L"[\n"; _Level++;

            if (value._Array.size() > 0)
                Text += Indent();

            size_t i = 0;

            for (auto Element : value._Array)
            {
                Text += ToString(Element);

                if (++i < value._Array.size())
                    Text += L", ";
            };

            _Level--; Text += L"\n" + Indent() + L"]";
            break;
        }

        case Value::Type::Object:
        {
            Text += L"{\n"; _Level++; 

            size_t i = 0;

            for (auto Element : value._Object)
            {
                Text += Indent() + L"\"" + ToString(Element.first) + L"\": " + ToString(Element.second);

                if (++i < value._Object.size())
                    Text += L",\n";
            };

             _Level--; Text += L"\n" + Indent() + L"}";
            break;
        }
    }

    return Text;
}
#pragma endregion

const std::wstring ReaderException::None = L"None";

// String
const std::wstring ReaderException::InvalidCharacterInString = L"Invalid character in string";
const std::wstring ReaderException::UnterminatedString = L"Unterminated string";
const std::wstring ReaderException::IncompleteEscapeSequence = L"Incomplete escape sequence";
const std::wstring ReaderException::UnknownEscapeSequence = L"Unknown escape sequence";
const std::wstring ReaderException::HexadecimalEscapeSequenceTooShort = L"Hexadecimal escape sequence too short";
const std::wstring ReaderException::InvalidHexadecimalEscapeSequence = L"Invalid hexadecimal escape sequence";

// Number
const std::wstring ReaderException::InvalidNumber = L"Invalid number";

// Boolean
const std::wstring ReaderException::InvalidBoolean = L"Invalid boolean";

// Null
const std::wstring ReaderException::InvalidNull = L"Invalid null";

// Array
const std::wstring ReaderException::UnclosedArray = L"Unclosed array";
const std::wstring ReaderException::MissingArrayValue = L"Missing array value";
const std::wstring ReaderException::UnexpectedEndOfArray = L"Unexpected end of array";

// Object
const std::wstring ReaderException::UnclosedObject = L"Unclosed object";
const std::wstring ReaderException::MissingValueSeparator = L"Missing value separator";
const std::wstring ReaderException::MissingValue = L"Missing value";
const std::wstring ReaderException::UnexpectedEndOfObject = L"Unexpected end of object";
