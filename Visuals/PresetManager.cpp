
/** $VER: PresetManager.cpp (2024.03.04) P. Stuer **/

#include "PresetManager.h"

#include "Log.h"

#include <pathcch.h>

#pragma comment(lib, "pathcch")

#pragma hdrstop

/// <summary>
/// Loads a state object from a file.
/// </summary>
bool PresetManager::Load(const pfc::string & filePath, State * state) noexcept
{
    try
    {
        file_ptr File;

        filesystem::g_open(File, filePath, filesystem::open_mode_read, fb2k::noAbort);

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
        Log::Write(Log::Level::Error, "%s: Failed to read preset from \"%s\": %s", core_api::get_my_file_name(), filePath.c_str(), ex.what());

        return false;
    }
}

/// <summary>
/// Saves a state object to a file.
/// </summary>
bool PresetManager::Save(const pfc::string & filePath, const State * state) noexcept
{
    try
    {
        file_ptr File;

        filesystem::g_open(File, filePath, filesystem::open_mode_write_new, fb2k::noAbort);

        auto Writer = File.get_ptr();

        Writer->write_object_t(PresetManager::Magic, fb2k::noAbort);
        Writer->write_object_t(PresetManager::Version, fb2k::noAbort);

        state->Write(Writer, fb2k::noAbort);

        return true;
    }
    catch (pfc::exception ex)
    {
        Log::Write(Log::Level::Error, "%s: Failed to write preset to \"%s\": %s", core_api::get_my_file_name(), filePath.c_str(), ex.what());

        return false;
    }
}

bool PresetManager::GetFileNames(const pfc::string & directoryPathName, std::vector<std::wstring> & FileNames) noexcept
{
    WCHAR DirectoryPathName[MAX_PATH];

    HRESULT hResult = ::PathCchCombine(DirectoryPathName, _countof(DirectoryPathName), pfc::wideFromUTF8(directoryPathName).c_str(), L"*.fvsa");

    WIN32_FIND_DATAW ffd;

    HANDLE hFind = ::FindFirstFileW(DirectoryPathName, &ffd);

    while (hFind != INVALID_HANDLE_VALUE)
    {
        ::FindNextFileW(hFind, &ffd);
    }

    return true;
}
