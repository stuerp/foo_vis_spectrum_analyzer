
/** $VER: Log.cpp (2025.07.13) P. Stuer - Another logger implementation **/

#include "pch.h"

#include "Log.h"

class NullLog : public ILog
{
public:
    NullLog() noexcept { };

    NullLog(const NullLog &) = delete;
    NullLog(const NullLog &&) = delete;
    NullLog & operator=(const NullLog &) = delete;
    NullLog & operator=(NullLog &&) = delete;

    virtual ~NullLog() final { };

    ILog & SetLevel(LogLevel level) noexcept override final { return *this; }
    LogLevel GetLevel() const noexcept override final { return LogLevel::Never; }

    ILog & AtFatal() noexcept override final { return *this; }
    ILog & AtError() noexcept override final { return *this; }
    ILog & AtWarn() noexcept override final { return *this; }
    ILog & AtInfo() noexcept override final { return *this; }
    ILog & AtDebug() noexcept override final { return *this; }
    ILog & AtTrace() noexcept override final { return *this; }
    ILog & Write(const char * format, ... ) noexcept override final { return *this; }
};

ILog & Null = *new NullLog();

class log_impl_t : public ILog
{
public:
#ifdef _DEBUG
    log_impl_t() noexcept { SetLevel(LogLevel::Debug); }
#else
    log_impl_t() noexcept { SetLevel(LogLevel::Info); }
#endif

    log_impl_t(const log_impl_t &) = delete;
    log_impl_t(const log_impl_t &&) = delete;
    log_impl_t & operator=(const log_impl_t &) = delete;
    log_impl_t & operator=(log_impl_t &&) = delete;

    virtual ~log_impl_t() noexcept { };

    ILog & SetLevel(LogLevel level) noexcept override final
    {
        _Level = level;

        return *this;
    }

    LogLevel GetLevel() const noexcept override final
    {
        return _Level;
    }

    ILog & AtFatal() noexcept override final
    {
        return (_Level >= LogLevel::Fatal) ? *this : Null;
    }

    ILog & AtError() noexcept override final
    {
        return (_Level >= LogLevel::Error) ? *this : Null;
    }

    ILog & AtWarn() noexcept override final
    {
        return (_Level >= LogLevel::Warn) ? *this : Null;
    }

    ILog & AtInfo() noexcept override final
    {
        return (_Level >= LogLevel::Info) ? *this : Null;
    }

    ILog & AtDebug() noexcept override final
    {
        return (_Level >= LogLevel::Debug) ? *this : Null;
    }

    ILog & AtTrace() noexcept override final
    {
        return (_Level >= LogLevel::Trace) ? *this : Null;
    }

    ILog & Write(const char * format, ... ) noexcept override final
    {
        char Line[1024] = { };

        va_list args;

        va_start(args, format);

        (void) ::vsnprintf(Line, sizeof(Line) - 1, format, args);
        console::print(Line);

        return *this;
    }

private:
    LogLevel _Level;
};

ILog & Log = *new log_impl_t();

//::GetCurrentThreadId()
