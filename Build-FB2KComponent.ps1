<#
.SYNOPSIS
    Builds the foobar2000 component package.
.DESCRIPTION
    This script will be executed unconditionally during the Post-build step. It copies all the necessary files to an output directory and creates the zip archive.
.EXAMPLE
    C:\PS> .\Build-FB2KComponent.ps1
.OUTPUTS
    *.fb2k-component
#>

[CmdletBinding()]
param
(
    [parameter(Mandatory, HelpMessage='Target Name')]
        [string] $TargetName,
    [parameter(Mandatory, HelpMessage='Target File Name')]
        [string] $TargetFileName,
    [parameter(Mandatory, HelpMessage='Platform')]
        [string] $Platform,
    [parameter(Mandatory, HelpMessage='OutputPath')]
        [string] $OutputPath
)

#Requires -Version 7.2

Set-StrictMode -Version Latest;
Set-PSDebug -Strict; # Equivalent of VBA "Option Explicit".

$ErrorActionPreference = 'Stop';

# Note: The working directory is the solution directory.

Write-Host "Building package `"$TargetName`" ($Platform)...";

if ($Platform -eq 'x64')
{
    $PackagePath = "../out/$TargetName";

    # Create the package directory (including the x64 subdirectory)
    Write-Host "Creating directory `"$PackagePath`"...";
    $null = New-Item -Path '../out/' -Name "$TargetName/x64" -ItemType 'directory' -Force;

    if (Test-Path -Path "$OutputPath/$TargetFileName")
    {
        Write-Host "Copying $TargetFileName to `"$PackagePath/x64`"...";
        Copy-Item "$OutputPath/$TargetFileName" -Destination "$PackagePath/x64" -Force -Verbose;
    }

    # install the component in the foobar2000 x64 components directory.
    $foobar2000Path = '../bin';

    if (Test-Path -Path "$foobar2000Path/foobar2000.exe")
    {
        $ComponentPath = "$foobar2000Path/profile/user-components-x64";

        Write-Host "Creating directory `"$ComponentPath/$TargetName`"...";
        $null = New-Item -Path "$ComponentPath" -Name "$TargetName" -ItemType 'directory' -Force;

        Write-Host "Installing x64 component in foobar2000 64-bit profile...";
        Copy-Item "$PackagePath/x64/*.dll" "$ComponentPath/$TargetName" -Force -Verbose;
    }
    else
    {
        Write-Host "Skipped component installation: foobar2000 64-bit directory not found.";
    }
}
elseif ($Platform -eq 'Win32')
{
    $PackagePath = "../out/$TargetName";

    # Create the package directory (including the x64 subdirectory)
    Write-Host "Creating directory `"$PackagePath`"...";
    $null = New-Item -Path '../out/' -Name "$TargetName/x64" -ItemType 'directory' -Force;

    if (Test-Path -Path "$OutputPath/$TargetFileName")
    {
        Write-Host "Copying $TargetFileName to `"$PackagePath`"...";
        Copy-Item "$OutputPath/$TargetFileName" -Destination "$PackagePath";
    }

    # install the x86 component in the foobar2000 x86 components directory.
    $foobar2000Path = '../bin/x86';

    if (Test-Path -Path "$foobar2000Path/foobar2000.exe")
    {
        $ComponentPath = "$foobar2000Path/profile/user-components";

        Write-Host "Creating directory `"$ComponentPath/$TargetName`"...";
        $null = New-Item -Path "$ComponentPath" -Name "$TargetName" -ItemType 'directory' -Force;

        Write-Host "Installing x86 component in foobar2000 32-bit profile...";
        Copy-Item "$PackagePath/*.dll" "$ComponentPath/$TargetName" -Force -Verbose;
    }
    else
    {
        Write-Host "Skipped component installation: foobar2000 32-bit directory not found.";
    }
}
else
{
    Write-Host "Unknown platform: $Platform";
    exit;
}

$ArchivePath = "../out/$TargetName.fb2k-component";

Write-Host "Creating component archive `"$ArchivePath`"...";

Compress-Archive -Force -Path ../out/$TargetName/* -DestinationPath $ArchivePath;

Write-Host "Done.";
