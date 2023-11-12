<#
.SYNOPSIS
    Builds the foobar2000 component package.
.DESCRIPTION
    This script will be executed unconditionally during the Post-build step. It copies all the necessary files to an output directory and creates the zip archive.
.EXAMPLE
    C:\PS> .\Build-FB2KComponent.ps1
.OUTPUTS
    foo_input_pmd.fb2k-component
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
    $PackagePath = "../out/$TargetName/x64";

    if (!(Test-Path -Path $PackagePath))
    {
        Write-Host "Creating directory `"$PackagePath`"...";
        $null = New-Item -Path '../out/' -Name "$TargetName/x64" -ItemType 'directory';
    }

    if (Test-Path -Path "$OutputPath/$TargetFileName")
    {
        Write-Host "Copying $TargetFileName to `"$PackagePath`"...";
        Copy-Item "$OutputPath/$TargetFileName" -Destination "$PackagePath";
    }

    # install the component in the foobar2000 x86 components directory.
    $foobar2000Path = '../bin/x86';

    if (Test-Path -Path "$foobar2000Path/foobar2000.exe")
    {
        $ComponentPath = "$foobar2000Path/profile/user-components/$TargetName";

        Write-Host "Creating directory `"$ComponentPath`"...";
        $null = New-Item -Path "$foobar2000Path/profile/user-components/" -Name "$TargetName" -ItemType 'directory' -Force;

        Write-Host "Installing component in foobar2000 32-bit...";
        Copy-Item "$PackagePath/../*" "$foobar2000Path/profile/user-components/$TargetName" -Force;
    }
    else
    {
        Write-Host "Skipped component installation: foobar2000 32-bit directory not found.";
    }

    # install the component in the foobar2000 x64 components directory.
    $foobar2000Path = '../bin';

    if (Test-Path -Path "$foobar2000Path/foobar2000.exe")
    {
        $ComponentPath = "$foobar2000Path/profile/user-components-x64/$TargetName";

        Write-Host "Creating directory `"$ComponentPath`"...";
        $null = New-Item -Path "$foobar2000Path/profile/user-components-x64/" -Name "$TargetName" -ItemType 'directory' -Force;

        Write-Host "Installing component in foobar2000 64-bit...";
        Copy-Item "$PackagePath/*" $ComponentPath -Force;
    }
    else
    {
        Write-Host "Skipped component installation: foobar2000 64-bit directory not found.";
    }
}
elseif ($Platform -eq 'Win32')
{
    $PackagePath = "../out/$TargetName";

    if (!(Test-Path -Path $PackagePath))
    {
        Write-Host "Creating directory `"$PackagePath`"...";
        $null = New-Item -Path '../out/' -Name "$TargetName/x64" -ItemType 'directory' -Force;
    }

    if (Test-Path -Path "$OutputPath/$TargetFileName")
    {
        Write-Host "Copying $TargetFileName to `"$PackagePath`"...";
        Copy-Item "$OutputPath/$TargetFileName" -Destination "$PackagePath";
    }

    # install the component in the foobar2000 x86 components directory.
    $foobar2000Path = '../bin/x86';

    if (Test-Path -Path "$foobar2000Path/foobar2000.exe")
    {
        $ComponentPath = "$foobar2000Path/profile/user-components/$TargetName";

        Write-Host "Creating directory `"$ComponentPath`"...";
        $null = New-Item -Path "$foobar2000Path/profile/user-components/" -Name "$TargetName" -ItemType 'directory' -Force;

        Write-Host "Copying component files to profile...";
        Copy-Item "$PackagePath/*" $ComponentPath -Recurse -Force;
    }
    else
    {
        Write-Host "Skipped component installation: foobar2000 32-bit directory not found.";
    }

    # install the component in the foobar2000 x64 components directory.
    $foobar2000Path = '../bin';

    if (Test-Path -Path "$foobar2000Path/foobar2000.exe")
    {
        $ComponentPath = "$foobar2000Path/profile/user-components-x64/$TargetName";

        Write-Host "Creating directory `"$ComponentPath`"...";
        $null = New-Item -Path "$foobar2000Path/profile/user-components-x64/" -Name "$TargetName" -ItemType 'directory' -Force;

        Write-Host "Copying component files to profile...";
        Copy-Item "$PackagePath/*" $ComponentPath -Recurse -Force;
    }
    else
    {
        Write-Host "Skipped component installation: foobar2000 64-bit directory not found.";
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
