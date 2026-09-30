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

### Nintendo Wii
Place the system ROM(s) in `sd:/miniCDi/rom` and any disc images/games in `sd:/miniCDi/discs`.

Once opened, select a disc image from the menu. Press Home (Wii) or Z (GameCube) to exit emulation and return to the emulator menu.
In some cases the CD-i machine may not start properly. If this happens try going back to the emulator menu and starting over (this may take several tries).

### Nintendo 3DS
Place the system ROM(s) in `sdmc:/3ds/miniCDi/rom` and any disc images/games in `sdmc:/3ds/miniCDi/discs`.

Once opened, select a disc image from the menu. Press ZR to quit the emulator.

### Nintendo Wii U
Place the system ROM(s) in `/vol/external01/wiiu/apps/miniCDi/rom` and any disc images/games in `/vol/external01/wiiu/apps/miniCDi/discs`.

Once opened, select a disc image from the menu. Press ZR to exit emulation and return to the emulator menu.
In some cases the CD-i machine may not start properly. If this happens try going back to the emulator menu and starting over (this may take several tries).

## Technical details
### Compatibility
The following boards and chips have been implemented. CD-i Fan has more information regarding hardware at [cdichips](https://github.com/cdifan/cdichips) repository.

* ***Mono-I***: SCC68070, MCD212, CDIC, SLAVE
* ***Mono-II***: SCC68070, MCD212, DRVDSP (stub), SLAVE
* ***Mono-III***, ***Mono-IV***, ***Robocon***: SCC68070, MCD212, CIAP (stub), IKAT

Only the Mono-I driver is capable of playing CD-i discs, since the DRVDSP and CIAP in later boards are not fully emulated. Certain software may softlock due to constant D-Pad movement polling by SLAVE (e.g. Zelda: Wand of Gamelon or [CDi_BadApple](https://github.com/Slamy/CDi_BadApple)).

## To-Do
- [ ] Check audiomap-to-XA switching
- [ ] Rewrite scheduler (possibly also chips?) based on GB emulator experience
- [ ] Emulate timekeeper on Mono-I/Mono-IV? (should handle NVRAM saving)

## Building
To compile, use devkitPro's `powerpc-eabi-cmake` (GC, (v)Wii, Wii U) or `arm-none-eabi-cmake` (3DS), or the regular MINGW64 CMake if compiling for Windows. The corresponding SDL2 package is required, except on 3DS.

The latest commit is compiled automatically using GitHub Actions (`.github/workflows/*.yml`).