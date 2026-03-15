
/** $VER: PresetManager.cpp (2026.03.01) P. Stuer - Manages the component presets. **/

#include "pch.h"

#include "PresetManager.h"

#include "Resources.h"
#include "Support.h"
#include "Log.h"

#include <Path.h>
#include <Error.h>

namespace fio = foobar2000_io;

#pragma hdrstop

/// <summary>
/// Loads a state object from a file.
/// </summary>
bool PresetManager::Load(const path_t & rootPath, const std::wstring & presetName, state_t * state) noexcept
{
    const path_t PresetPath = CreatePresetPath(rootPath, presetName);

    try
    {
        file_ptr File;

        fio::filesystem::g_open(File, msc::WideToUTF8(PresetPath).c_str(), fio::filesystem::open_mode_read, fb2k::noAbort);

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

        state_t NewState;

        NewState._PresetsDirectoryPath = PresetPath.c_str();

        NewState.Read(Reader, Size, fb2k::noAbort, true);

        NewState._PresetsDirectoryPath = rootPath.c_str();

        *state = NewState;

        return true;
    }
    catch (pfc::exception ex)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " failed to read preset \"%s\": %s", presetName.c_str(), ex.what());

        return false;
    }
}

/// <summary>
/// Saves a state object to a file.
/// </summary>
bool PresetManager::Save(const path_t & rootPath, const std::wstring & presetName, const state_t * state) noexcept
{
    const path_t PresetPath = CreatePresetPath(rootPath, presetName);

    try
    {
        file_ptr File;

        fio::filesystem::g_open(File, msc::WideToUTF8(PresetPath).c_str(), fio::filesystem::open_mode_write_new, fb2k::noAbort);

        auto Writer = File.get_ptr();

        Writer->write_object_t(PresetManager::Magic, fb2k::noAbort);
        Writer->write_object_t(PresetManager::Version, fb2k::noAbort);

        state->Write(Writer, fb2k::noAbort, true);

        return true;
    }
    catch (pfc::exception ex)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " failed to write preset \"%s\": %s", presetName.c_str(), ex.what());

        return false;
    }
}

/// <summary>
/// Deletes the specified preset.
/// </summary>
bool PresetManager::Delete(const path_t & rootPath, const std::wstring & presetName) noexcept
{
    const path_t PresetPath = CreatePresetPath(rootPath, presetName);

    const BOOL Success = ::DeleteFileW(PresetPath);

    if (!Success)
    {
        error_t LastError(::GetLastError());

        Log.AtError().Write(STR_COMPONENT_BASENAME " failed to delete preset \"%s\": %s", presetName.c_str(), LastError.Message().c_str());

        return false;
    }

    return true;
}

/// <summary>
/// Gets the names of the preset files.
/// </summary>
bool PresetManager::GetPresetNames(const path_t & rootPath, std::vector<std::wstring> & presetNames) noexcept
{
    std::wstring PresetsDirectoryPath;

    {
        pfc::string Result;

        HRESULT hResult = EvaluateTitleFormatScript(rootPath, Result);

        if (!SUCCEEDED(hResult))
            return false;

        PresetsDirectoryPath = msc::UTF8ToWide(Result.c_str());

        // Make sure the path exists before proceding.
        if (::GetFileAttributesW(PresetsDirectoryPath.c_str()) == INVALID_FILE_ATTRIBUTES)
            return false;
    }

    path_t SearchPath = PresetsDirectoryPath;

    HRESULT hr = path_t::Combine(SearchPath, L"*.fvsa", SearchPath);

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

/// <summary>
/// Creates the full path of a preset.
/// </summary>
path_t PresetManager::CreatePresetPath(const path_t & rootPath, const std::wstring & presetName) noexcept
{
    pfc::string Result;

    HRESULT hResult = EvaluateTitleFormatScript(rootPath, Result);

    const path_t RootPath = SUCCEEDED(hResult) ? msc::UTF8ToWide(Result.c_str()) : rootPath.c_str();

    path_t PresetPath;

    path_t::AddExtension(presetName, L"fvsa", PresetPath);
    path_t::Combine(RootPath, PresetPath, PresetPath);

    return PresetPath;
}
