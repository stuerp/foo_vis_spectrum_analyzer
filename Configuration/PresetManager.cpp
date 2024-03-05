
/** $VER: PresetManager.cpp (2024.03.05) P. Stuer **/

#include "PresetManager.h"

#include "Log.h"

#include <pathcch.h>

#pragma comment(lib, "pathcch")

#pragma hdrstop

/// <summary>
/// Loads a state object from a file.
/// </summary>
bool PresetManager::Load(const pfc::string & rootPath, const pfc::string & presetName, State * state) noexcept
{
    WCHAR FileName[MAX_PATH];

    ::wcscpy_s(FileName, _countof(FileName), pfc::wideFromUTF8(presetName));
    ::PathCchAddExtension(FileName, _countof(FileName), L"fvsa");

    pfc::string FilePath = pfc::io::path::combine(rootPath, pfc::utf8FromWide(FileName));

    try
    {
        file_ptr File;

        filesystem::g_open(File, FilePath, filesystem::open_mode_read, fb2k::noAbort);

        auto Reader = File.get_ptr();

        size_t Size = (size_t) Reader->get_size_ex(fb2k::noAbort);

        DWORD Value;

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
bool PresetManager::Save(const pfc::string & rootPath, const pfc::string & presetName, const State * state) noexcept
{
    WCHAR FileName[MAX_PATH];

    ::wcscpy_s(FileName, _countof(FileName), pfc::wideFromUTF8(presetName));
    ::PathCchAddExtension(FileName, _countof(FileName), L"fvsa");

    pfc::string FilePath = pfc::io::path::combine(rootPath, pfc::utf8FromWide(FileName));

    try
    {
        file_ptr File;

        filesystem::g_open(File, FilePath, filesystem::open_mode_write_new, fb2k::noAbort);

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
bool PresetManager::Delete(const pfc::string & rootPath, const pfc::string & presetName) noexcept
{
    WCHAR FileName[MAX_PATH];

    ::wcscpy_s(FileName, _countof(FileName), pfc::wideFromUTF8(presetName));
    ::PathCchAddExtension(FileName, _countof(FileName), L"fvsa");

    pfc::string FilePath = pfc::io::path::combine(rootPath, pfc::utf8FromWide(FileName));

    try
    {
        filesystem::g_remove(FilePath, fb2k::noAbort);

        return true;
    }
    catch (pfc::exception ex)
    {
        Log::Write(Log::Level::Error, "%s: Failed to delete preset \"%s\": %s", core_api::get_my_file_name(), presetName.c_str(), ex.what());

        return false;
    }
}

/// <summary>
/// Gets the names of the preset files.
/// </summary>
bool PresetManager::GetPresetNames(const pfc::string & rootPath, std::vector<std::wstring> & presetNames) noexcept
{
    WCHAR DirectoryPathName[MAX_PATH];

    HRESULT hr = ::PathCchCombine(DirectoryPathName, _countof(DirectoryPathName), pfc::wideFromUTF8(rootPath).c_str(), L"*.fvsa");

    if (!SUCCEEDED(hr))
        return false;

    WIN32_FIND_DATAW ffd;

    HANDLE hFind = ::FindFirstFileW(DirectoryPathName, &ffd);

    BOOL Success = (hFind != INVALID_HANDLE_VALUE);

    while (Success)
    {
        ::PathCchRemoveExtension(ffd.cFileName, _countof(ffd.cFileName));

        presetNames.push_back(ffd.cFileName);

        Success = ::FindNextFileW(hFind, &ffd);
    }

    return true;
}
