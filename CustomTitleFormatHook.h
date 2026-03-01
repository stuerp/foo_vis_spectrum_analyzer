
/** $VER: CustomTitleFormatHook.h (2026.03.01) P. Stuer - Implements a custom hook to expand title formatting **/

#pragma once

class custom_titleformat_hook_t : public titleformat_hook
{
public:
    custom_titleformat_hook_t() { }

    virtual ~custom_titleformat_hook_t() noexcept { }

    virtual bool process_field(titleformat_text_out * out, const char * name, t_size nameSize, bool & isFound)
    {
        pfc::string Path;

        if (::_stricmp(name, "fb2k_path") == 0)
        {
            ::uGetModuleFileName(NULL, Path);

            Path = pfc::string_directory(Path);

            out->write(titleformat_inputtypes::unknown, Path);

            isFound = true;

            return true;
        }
        else
        if (::_stricmp(name, "fb2k_component_path") == 0)
        {
            ::uGetModuleFileName(core_api::get_my_instance(), Path);

            Path = pfc::string_directory(Path);

            out->write(titleformat_inputtypes::unknown, Path);

            isFound = true;

            return true;
        }
        else
        if (::_stricmp(name, "fb2k_profile_path") == 0)
        {
            Path = core_api::get_profile_path();

            Path = foobar2000_io::filesystem::g_get_native_path(Path);

            out->write(titleformat_inputtypes::unknown, Path);

            isFound = true;

            return true;
        }
        else
        {
            std::string Name = "%"; Name += name; Name += "%";

            Path = ExpandEnvironmentStrings(Name.c_str()).c_str();

            out->write(titleformat_inputtypes::unknown, Path);

            isFound = true;

            return true;
        }

        return false;
    }

    virtual bool process_function(titleformat_text_out * out, const char * name, t_size nameSize, titleformat_hook_function_params * parameters, bool & isFound)
    {
        return false;
    }

private:
    /// <summary>
    /// Expands the environment variables in the specified string.
    /// </summary>
    const std::string ExpandEnvironmentStrings(const char * src) noexcept
    {
        const DWORD Size = ::ExpandEnvironmentStringsA(src, nullptr, 0) + 1;

        std::string Dst;

        Dst.resize(Size);

        ::ExpandEnvironmentStringsA(src, (LPSTR) Dst.data(), (DWORD) Dst.size());

        return Dst;
    }
};
