
/** $VER: PresetManager.cpp (2024.03.09) P. Stuer **/

#include "PresetManager.h"

#include <Path.h>
#include <Convert.h>
#include <Error.h>

#include "Log.h"

#pragma hdrstop

/// <summary>
/// Loads a state object from a file.
/// </summary>
bool PresetManager::Load(const Path & rootPath, const std::wstring & presetName, State * state) noexcept
{
    Path PresetPath;

    Path::AddExtension(presetName, L"fvsa", PresetPath);
    Path::Combine(rootPath, PresetPath, PresetPath);

    try
    {
        file_ptr File;

        filesystem::g_open(File, Convert::To(PresetPath), filesystem::open_mode_read, fb2k::noAbort);

        auto Reader = File.get_ptr();

        size_t Size = (size_t) Reader->get_size_ex(fb2k::noAbort);

        uint32_t Value;

        Reader->read_object_t(Value, fb2k::noAbort);
        Size -= sizeof(Value);

        if (Value != PresetManager::Magic)
            return false;

        Reader->read_object_t(Value, fb2k::noAbort);
        Size -= sizeof(Value);

        if (Value != PresetManager::Version)
            return false;

        State NewState;

        NewState.Read(Reader, Size, fb2k::noAbort);

        *state = NewState;

        return true;
    }
    catch (pfc::exception ex)
    {
        Log::Write(Log::Level::Error, "%s: Failed to read preset \"%s\": %s", core_api::get_my_file_name(), presetName.c_str(), ex.what());

        return false;
    }
}

/// <summary>
/// Saves a state object to a file.
/// </summary>
bool PresetManager::Save(const Path & rootPath, const std::wstring & presetName, const State * state) noexcept
{
    Path PresetPath;

    Path::AddExtension(presetName, L"fvsa", PresetPath);
    Path::Combine(rootPath, PresetPath, PresetPath);

    try
    {
        file_ptr File;

        filesystem::g_open(File, Convert::To(PresetPath), filesystem::open_mode_write_new, fb2k::noAbort);

        auto Writer = File.get_ptr();

        Writer->write_object_t(PresetManager::Magic, fb2k::noAbort);
        Writer->write_object_t(PresetManager::Version, fb2k::noAbort);

        state->Write(Writer, fb2k::noAbort);

        return true;
    }
    catch (pfc::exception ex)
    {
        Log::Write(Log::Level::Error, "%s: Failed to write preset \"%s\": %s", core_api::get_my_file_name(), presetName.c_str(), ex.what());

        return false;
    }
}

/// <summary>
/// Deletes the specified preset.
/// </summary>
bool PresetManager::Delete(const Path & rootPath, const std::wstring & presetName) noexcept
{
    Path PresetPath;

    Path::AddExtension(presetName, L"fvsa", PresetPath);
    Path::Combine(rootPath, PresetPath, PresetPath);

    BOOL Success = ::DeleteFileW(PresetPath);

    if (!Success)
    {
        Error LastError(::GetLastError());

        Log::Write(Log::Level::Error, "%s: Failed to delete preset \"%s\": %s", core_api::get_my_file_name(), presetName.c_str(), LastError.Message().c_str());

        return false;
    }

    return true;
}

/// <summary>
/// Gets the names of the preset files.
/// </summary>
bool PresetManager::GetPresetNames(const Path & rootPath, std::vector<std::wstring> & presetNames) noexcept
{
    Path SearchPath = rootPath;

    HRESULT hr = Path::Combine(SearchPath, L"*.fvsa", SearchPath);

    if (!SUCCEEDED(hr))
        return false;

    WIN32_FIND_DATAW ffd;

    HANDLE hFind = ::FindFirstFileW(SearchPath, &ffd);

    BOOL Success = (hFind != INVALID_HANDLE_VALUE);

    while (Success)
    {
        ::PathCchRemoveExtension(ffd.cFileName, _countof(ffd.cFileName));

        presetNames.push_back(ffd.cFileName);

        Success = ::FindNextFileW(hFind, &ffd);
    }

    return true;
}
