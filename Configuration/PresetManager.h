
/** $VER: PresetManager.h (2024.03.04) P. Stuer - Represents a preset of the component. **/

#pragma once

#include "framework.h"

#include "State.h"

class PresetManager
{
public:
    PresetManager() = delete;

    PresetManager(const PresetManager &) = delete;
    PresetManager & operator=(const PresetManager &) = delete;
    PresetManager(PresetManager &&) = delete;
    PresetManager & operator=(PresetManager &&) = delete;

    ~PresetManager() = delete;

    static bool Load(const pfc::string & filePath, State * state) noexcept;
    static bool Save(const pfc::string & filePath, const State * state) noexcept;

    static bool GetFileNames(const pfc::string & directoryPathName, std::vector<std::wstring> & FileNames) noexcept;

private:
    static const DWORD Magic = mmioFOURCC('F','V','S','A');
    static const DWORD Version = 1;
};
