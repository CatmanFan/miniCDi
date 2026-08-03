# <div align=center><img src="https://github.com/CatmanFan/miniCDi/blob/master/res/logo.png" width="25%" /></div>

An experimental multiplatform Philips CD-i emulator written in C++17. (∩ ͡° ͜ʖ ͡°)⊃━☆ﾟ. *

## Features
* Mono-I board (CDI 200, CDI 220/20) fully supported, can run commercial game discs and homebrew
* Mono-II and Mono-IV boards are partially supported, can run player shell but disc emulation is not available.
* Emulation of the fluorescent tube display (FTD) on the player's front-facing panel
* Partial audio support (soundmap playback via CPU is not 100%, but can read audio sectors from disc fine)
* Experimental controller emulation
   * This is still not sorted out due to the nature of the CD-i pointing device being an absolute and not relative (i.e. tablet-style) type. This works fine for the player shell but translates to wanky controls on actual CD-i games.
* And most importantly: confirmed to run on Windows, macOS (courtesy of [yeah-its-gloria](https://github.com/yeah-its-gloria)), (v)Wii, Wii U (WUHB) and 3DS. May not run up to fullspeed on all builds.

## Credits
Special credits to [Stovent](https://github.com/Stovent), [CD-i Fan](https://github.com/cdifan) and [Slamy](https://github.com/Slamy) for helping me wherever possible on this project. The emulator uses [Musashi](https://github.com/kstenerud/Musashi) version 4.10 as a core for the 68070 processor.

Some of the emulation code is ported or adapted from:
* CD-i Fan's [cdichips](https://github.com/cdifan/cdichips) documentation of several components including the MCD212, SCC68070 (UART), IKAT and SLAVE
* Slamy's documentation of the CDIC (see [CDIC_BlackBoxAnalyzer](https://github.com/Slamy/CDIC_BlackBoxAnalyzer))
* Stovent's implementations of the relevant components in [CeDImu](https://github.com/Stovent/CeDImu) (license unknown)
* the MAME CD-i driver by Vincent Halver and Ryan Holtz (licensed under BSD-3) ([global MAME license](https://github.com/mamedev/mame?tab=License-1-ov-file))
* reverse engineering of the [CD-i Emulator](https://www.cdiemu.org/) trace log

## License
The general code for this emulator is released under [GPLv3](https://www.gnu.org/licenses/gpl-3.0.html) (see above credits for other licenses).

## Usage
### Windows / macOS
Run miniCDi using the command line arguments `miniCDi <boot.rom> [disc.bin]`. Alternatively, drag the system ROM file itself into miniCDi to boot the emulated system from the ROM, then drag the disc image into the emulator window.

#### Controls

| CD-i pointing device | Mouse (if focused) | Keyboard   |
|----------------------|--------------------|------------|
| Button 1             | Left-click         | Enter      |
| Button 2             | Right-click        | Space bar  |
| Directional buttons  | Cursor movement    | Arrow keys |
| *Reset emulator*     | -                  | R          |
| *Play button on FP*  | -                  | E          |
| *Toggle FTD*         | -                  | F          |
| *Toggle frame limit* | -                  | T          |
| *Change resolution*  | -                  | V          |

### Nintendo Wii
Place the system ROM(s) in `sd:/miniCDi/rom` and any disc images/games in `sd:/miniCDi/discs`.

Once opened, select a disc image from the menu. Press Home (Wii) or Z (GameCube) to exit emulation and return to the emulator menu.
In some cases the CD-i machine may not start properly. If this happens try going back to the emulator menu and starting over (this may take several tries).

#### Controls

| CD-i pointing device | Wii Remote | Wii Classic Controller |
|----------------------|------------|------------------------|
| Button 1             | 1          | A                      |
| Button 2             | 2          | B                      |
| Directional buttons  | D-Pad      | D-Pad                  |

GameCube controller support is currently only available in the GameCube build.

### Nintendo 3DS
Place the system ROM(s) in `sdmc:/3ds/miniCDi/rom` and any disc images/games in `sdmc:/3ds/miniCDi/discs`.

Once opened, select a disc image from the menu. Press ZR to quit the emulator.

#### Controls

| CD-i pointing device | Nintendo 3DS        |
|----------------------|---------------------|
| Button 1             | A                   |
| Button 2             | B                   |
| Directional buttons  | D-Pad or Circle Pad |

### Nintendo Wii U
Place the system ROM(s) in `/vol/external01/wiiu/apps/miniCDi/rom` and any disc images/games in `/vol/external01/wiiu/apps/miniCDi/discs`.

Once opened, select a disc image from the menu. Press ZR to exit emulation and return to the emulator menu.
In some cases the CD-i machine may not start properly. If this happens try going back to the emulator menu and starting over (this may take several tries).

#### Controls

| CD-i pointing device | Wii U GamePad       |
|----------------------|---------------------|
| Button 1             | A                   |
| Button 2             | B                   |
| Directional buttons  | D-Pad or left stick |

### Configuration
The emulation settings can be configured in `config.ini` relative to the emulator's executable (e.g. .exe, .dol, .3dsx, etc).

An example of the default settings:
```[cdi]
autosavenvram=0
testplug=0
pal=1
analogcolors=0

[minicdi]
frameskip=0
pointeradvance=0
logging=0```

Setting the video mode setting to `0` (NTSC) may negatively affect emulation speed. Frameskip may help on slower consoles but may not reach 100% speed.

## Technical details
### Compatibility
The following boards and chips have been implemented. CD-i Fan has more information regarding hardware at [cdichips](https://github.com/cdifan/cdichips) repository.

* ***Mono-I***: SCC68070, MCD212, CDIC, SLAVE
* ***Mono-II***: SCC68070, MCD212, DRVDSP (stub), SLAVE
* ***Mono-III***, ***Mono-IV***, ***Robocon***: SCC68070, MCD212, CIAP (stub), IKAT

Only the Mono-I driver is capable of playing CD-i discs, since the DRVDSP and CIAP in later boards are not fully emulated. Certain software may softlock due to constant D-Pad movement polling by SLAVE (e.g. Zelda: Wand of Gamelon or [CDi_BadApple](https://github.com/Slamy/CDi_BadApple)).

### Benchmarking
Time taken to process and render a single frame on:

* Windows (x64, Nvidia + Intel i5-12400F): ***~1.7ms*** (fullspeed)
* Wii (vWii overclock): ***~33.77ms*** (~29 fps)
* Wii (normal): ***~56.44ms*** (~18 fps)
* Wii U (WUHB): ***~58.51ms*** (~17 fps)
* New 3DS: ***~78.79ms*** (~13 fps)
* Old 3DS: ***~237ms*** (~4 fps)

All profiled times with the exception of the Windows version are longer than the minimum needed for throttling (16.667ms or 1/60 secs). All are compiled using the fastest compile optimizations available under GCC and are accurate as of commit [`2e14ffa`](https://github.com/CatmanFan/miniCDi/commit/2e14ffaf5d9d73b1a2df3745225006fff5a1945f).

## Screenshots
### Player shell
<div align=center>

| CDI 200, CDI 220/20 (Mono-I) | CDI 220/40 (Mono-II) | CDI 490/00 (Mono-IV) |
|------------------------------|----------------------|----------------------|
| <img src="https://github.com/CatmanFan/miniCDi/blob/master/res/capture_220b.png" /> | <img src="https://github.com/CatmanFan/miniCDi/blob/master/res/capture_220c.png" /> | <img src="https://github.com/CatmanFan/miniCDi/blob/master/res/capture_490.png" />
</div>

### Gameplay
All captured under CDI 200 using Mono-I driver.

<div align=center>

| Hotel Mario | Frog Feast | Zelda: Wand of Gamelon |
|-------------|------------|------------------------|
| <img src="https://github.com/CatmanFan/miniCDi/blob/master/res/capture_200_hotelmario.png" /> | <img src="https://github.com/CatmanFan/miniCDi/blob/master/res/capture_200_frogfeast.png" /> | <img src="https://github.com/CatmanFan/miniCDi/blob/master/res/capture_200_zelda.png" /> |

</div>

## To-Do

### Potential
- [ ] Find faster 68010 emulator for ARM (3DS) + PowerPC? ([Cyclone](https://github.com/notaz/cyclone68000) exists but may need to be modified to support 68010 derivative.)
- [ ] Audio playback support for native homebrew libraries (i.e. non-SDL)
- [ ] Emulate timekeeper on Mono-I/Mono-IV? (should handle NVRAM saving)
- [X] Fix PD on Mono-IV
- Disc-related:
   - [ ] CDIC: Address slowdown when reading sectors (only noticeable on embedded platforms?)
   - [ ] CIAP: Read discs properly
- [ ] LibRetro API compatibility?

## Building
To compile, use devkitPro's `powerpc-eabi-cmake` (GC, (v)Wii, Wii U) or `arm-none-eabi-cmake` (3DS), or the regular MINGW64 CMake if compiling for Windows. The corresponding SDL2 package is required, except on 3DS.

The latest commit is compiled automatically using GitHub Actions (`.github/workflows/*.yml`).