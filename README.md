# SCATTR

Granular sample scatter instrument (VST3), built with JUCE 8.

## Download a build
1. Open the **Actions** tab, click the latest green run.
2. Download the **SCATTR-VST3-Windows** artifact and unzip it.
3. Copy `SCATTR.vst3` to `C:\Program Files\Common Files\VST3\`.
4. In FL Studio: Options > Manage plugins > Find plugins (verify scan).
   SCATTR appears under Installed > Plugin database > Generators (or VST3).

## Build locally
    cmake -B build -G "Visual Studio 17 2022" -A x64
    cmake --build build --config Release

JUCE is fetched automatically; it is statically linked into the plugin,
so end users do not need JUCE installed.
