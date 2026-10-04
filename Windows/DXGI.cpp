
/** $VER: DXGI.cpp (2026.10.04) P. Stuer **/

#include "pch.h"

#include "DXGI.h"

#pragma comment(lib, "dxgi")

#pragma hdrstop

ComPtr<DXGIFactory::Interface> DXGIFactory::_Factory;
int64_t DXGIFactory::_ReferenceCount = 0;

HRESULT DXGIFactory::Initialize() noexcept
{
    std::lock_guard Lock(_Mutex);

    ++_ReferenceCount;

    if (_Factory)
        return S_OK;

    {
        #ifdef _DEBUG
            constexpr UINT Flags = DXGI_CREATE_FACTORY_DEBUG;
        #else
            constexpr UINT Flags = 0;
        #endif

        return ::CreateDXGIFactory2(Flags, IID_PPV_ARGS(_Factory.GetAddressOf()));
    }
}

void DXGIFactory::Terminate() noexcept
{
    std::lock_guard Lock(_Mutex);

    --_ReferenceCount;

    if (_ReferenceCount == 0)
        _Factory.Reset();
}

/// <summary>
/// Gets the refresh rate of the current display.
/// </summary>
HRESULT DXGI::GetRefreshRate(IDXGIDevice1 * dxgiDevice, double & refreshRate) noexcept
{
    refreshRate = 0.;

    if (dxgiDevice == nullptr)
        return E_POINTER;

    // Get the current desktop resolution for mode matching.
    DEVMODE DisplaySettings = { .dmSize = sizeof(DEVMODE) };

    if (!::EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &DisplaySettings))
        return HRESULT_FROM_WIN32(::GetLastError());

    ComPtr<IDXGIAdapter> DXGIAdapter;

    HRESULT hr = dxgiDevice->GetAdapter(DXGIAdapter.GetAddressOf());

    if (FAILED(hr))
        return hr;

    ComPtr<IDXGIOutput> DXGIOutput;

    // Primary output (index 0)
    hr = DXGIAdapter->EnumOutputs(0, DXGIOutput.GetAddressOf());

    if (FAILED(hr))
        return hr;

    // Find the closest matching mode (includes current refresh rate).
    const DXGI_MODE_DESC TargetMode =
    {
        .Width            = DisplaySettings.dmPelsWidth,
        .Height           = DisplaySettings.dmPelsHeight,
        .Format           = DXGI_FORMAT_R8G8B8A8_UNORM,  // Common format
        .ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
        .Scaling          = DXGI_MODE_SCALING_UNSPECIFIED
    };

    DXGI_MODE_DESC MatchedMode = { };

    hr = DXGIOutput->FindClosestMatchingMode(&TargetMode, &MatchedMode, nullptr);

    if (SUCCEEDED(hr))
    {
        refreshRate = static_cast<double>(MatchedMode.RefreshRate.Numerator) / MatchedMode.RefreshRate.Denominator;

        return hr;
    }

    // Fallback: Enumerate all modes and return the refresh rate of the first exact resolution match.

    UINT Flags = 0;  // Use 0 for current mode matching
    UINT ModeCount = 0;

    hr = DXGIOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, Flags, &ModeCount, nullptr);

    if (FAILED(hr) || (ModeCount == 0))
        return hr;

    std::vector<DXGI_MODE_DESC> Modes(ModeCount);

    hr = DXGIOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, Flags, &ModeCount, Modes.data());

    if (FAILED(hr))
        return hr;

    for (const auto & Mode : Modes)
    {
        if (Mode.Width == TargetMode.Width && Mode.Height == TargetMode.Height)
        {
            refreshRate = static_cast<double>(Mode.RefreshRate.Numerator) / Mode.RefreshRate.Denominator;
            break;
        }
    }

    return hr;
}
