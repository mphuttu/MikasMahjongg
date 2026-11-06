# MikasMahjongg

MikasMahjongg is a Windows Mahjong solitaire game built with C++, MFC, and
Direct2D. It was inspired by Ron Balewski's Mahjongg. Match free tile pairs to
clear the classic 144-tile turtle layout.

## Features

- Solvable starting deals and layered tile graphics.
- Save and load games, restart the original deal, and undo removed pairs.
- Hints, remaining-tile count, and solvable reshuffles where possible.
- Timed play, pause animation, victory celebration, and a ten-entry Hall of Fame.
- English and Finnish interfaces and compiled help.

Hints indicate a legal move, not necessarily a winning move. A successful
reshuffle clears undo history. Wins using assists can still qualify for the
Hall of Fame. Sound, printing, and other grayed-out menu commands are not
implemented.

## Requirements and installation

The installer targets 64-bit Windows 10 or later. It is a per-user installation
and includes the necessary x64 Microsoft C++ and MFC runtime DLLs alongside the
application. After building it as described below, run
`dist\MikasMahjongg-Setup-x64.exe` and select English or Finnish. The same
selection determines the application's language.

The installer writes `language.ini` beside the executable, with `Code=en` or
`Code=fi` in its `[Language]` section. Without that file the application
defaults to English. Restart after changing it. Windows-owned dialogs can
follow the Windows display language; MFC's toolbar customization dialog is
English even when the game is Finnish.

## Building

Dependencies:

- Visual Studio 2026 Community with the C++ desktop workload, MFC components,
  the v145 toolset, and a Windows SDK.
- HTML Help Workshop to rebuild the CHM files.
- Inno Setup 6 to build the x64 installer.

Open `MikasMahjongg.slnx` and build `Debug | x64` or `Release | x64`.
The solution also exposes an `x86` platform. The project copies the tile PNGs,
credits, and both compiled help files next to the executable. Prebuilt CHM files
are included in the repository; rebuild them after editing help sources:

```powershell
& 'C:\Program Files (x86)\HTML Help Workshop\hhc.exe' 'help\fi.hhp'
& 'C:\Program Files (x86)\HTML Help Workshop\hhc.exe' 'help\en.hhp'
```

HTML Help Compiler returns exit code **1** on success; inspect its output and
the generated files for errors. To build the installer, first build
`Release | x64`, then run:

```powershell
& 'C:\Program Files (x86)\Inno Setup 6\ISCC.exe' 'MikasMahjongg.iss'
```

The installer script defaults to the Visual Studio 2026 x64 redistributables
at version `14.51.36231`. If using another toolset version, pass its matching
redistributable x64 directory through Inno Setup's `/DRedistRoot=...` option.
Build output and the generated installer are not committed.

## Tests

The repository contains two standalone automated test executables. They are
not separate Visual Studio solution projects. Run the game-model and geometry
tests, for example, in a MinGW command prompt:

```text
g++ -std=c++14 -O2 MahjongGame.cpp tests\MahjongGameTests.cpp -o MahjongGameTests.exe
MahjongGameTests.exe
```

They exercise the layout, matching, free-tile rules, save/load validation,
restart, undo, geometry, and 100 generated deals and reshuffles. Keep assertions
enabled: do not compile these tests with `NDEBUG`.

Run the Hall of Fame tests in a Visual Studio x64 Native Tools command prompt:

```text
cl /nologo /utf-8 /EHsc /MDd /D_DEBUG /D_AFXDLL /D_UNICODE /DUNICODE /I. HallOfFame.cpp Localization.cpp tests\HallOfFameTests.cpp /FeHallOfFameTests.exe
HallOfFameTests.exe
```

Those tests use a temporary registry key, not the game's Hall of Fame.

## Documentation

The English and Finnish guides cover the same topics, with a table of contents
and keyword index in each compiled CHM. Open the guide from
**Help > Help Contents** or **Ohje > Ohjeen sisältö**, depending on the
installed language. The HTML sources are also available here:

- **English:** [Welcome](help/en.html), [getting started](help/getting-started.html),
  [rules](help/rules.html), [features](help/commands.html),
  [configuration](help/settings.html), [examples and tips](help/workflows.html),
  [troubleshooting and FAQ](help/troubleshooting.html),
  [technical information](help/technical.html), [release notes](help/release-notes.html),
  and [credits](help/credits.html).
- **Suomi:** [Tervetuloa](help/fi.html), [aloittaminen](help/fi-getting-started.html),
  [säännöt](help/fi-rules.html), [toiminnot](help/fi-commands.html),
  [asetukset](help/fi-settings.html), [esimerkit ja vinkit](help/fi-workflows.html),
  [vianmääritys ja usein kysytyt kysymykset](help/fi-troubleshooting.html),
  [tekniset tiedot](help/fi-technical.html), [versiotiedot](help/fi-release-notes.html)
  ja [tekijätiedot](help/fi-credits.html).

## Screenshots

No project screenshot is currently included. A screenshot can be added here
after capturing the application without personal or unrelated desktop content.

## Troubleshooting

- **Missing tiles or help:** keep `res\tiles`, `Help-en.chm`, and
  `Ohje-fi.chm` alongside the executable, or reinstall the game.
- **Help opens blank:** Windows may block a downloaded CHM. Unblock the trusted
  file through Properties or reinstall it locally.
- **Cannot save or load:** check the path and permissions, and select a valid
  `.mahjongg` file. Keep a separate backup of important saved games.
- **Wrong or invalid language:** check `language.ini` and restart the game.
- **No moves remain:** undo, try a reshuffle, or restart the original deal.

See the [troubleshooting and FAQ](help/troubleshooting.html) for more detail.

## Contributions

Report reproducible issues with the Windows version, build configuration,
steps to reproduce, and any relevant error message. Contributions should keep
the existing C++/MFC conventions, add regression tests for behavior changes,
and update the help when user-visible behavior changes. Build the solution and
run both test executables before proposing changes. Do not commit generated
build output, local saves, user-specific settings, or third-party executables.

## Credits and licensing

Copyright (c) Mika Huttunen. The game is inspired by Ron Balewski's Mahjongg;
the original game's executable and source code are not distributed here.

Mahjong tileset designed by Code Inferno ([www.codeinferno.com](http://www.codeinferno.com)).
Source: [OpenGameArt.org](https://opengameart.org/content/mahjong-tileset).
The tileset is licensed under [CC BY 3.0](https://creativecommons.org/licenses/by/3.0/).
See [CREDITS.txt](CREDITS.txt). That graphics license does not establish a
license for the application's source code; no source-code license is specified
in this repository.
