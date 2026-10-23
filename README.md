## <div align=center><img src="https://github.com/CatmanFan/miniCDi/blob/master/res/logo.png" width="25%" /><hr />**THIS PROJECT IS NOT VIBE-CODED!!**</div>

An experimental multiplatform Philips CD-i emulator written in C++17. Should run on Windows, macOS (courtesy of [yeah-its-gloria](https://github.com/yeah-its-gloria)), (v)Wii, Wii U (WUHB) and 3DS. May not run up to fullspeed on all builds.

## Usage
### Windows / macOS
Run miniCDi using the command line arguments `miniCDi <boot.rom> [disc.bin]`. Alternatively, drag the system ROM file itself into miniCDi to boot the emulated system from the ROM, then drag the disc image into the emulator window.

### Other
Other system builds expect player ROM files and disc images in the following directories:

| Build              | System ROMs                             | Discs                                     |
| ------------------ | --------------------------------------- | ----------------------------------------- |
| **Nintendo Wii**   | `sd:/miniCDi/rom`                       | `sd:/miniCDi/discs`                       |
| **Nintendo 3DS**   | `sdmc:/3ds/miniCDi/rom`                 | `sdmc:/3ds/miniCDi/discs`                 |
| **Nintendo Wii U** | `/vol/external01/wiiu/apps/miniCDi/rom` | `/vol/external01/wiiu/apps/miniCDi/discs` |

## Supported boards
With the exception of NVRAM, the following boards and chips are emulated:
* ***Mono-I***: SCC68070, MCD212, CDIC, SLAVE
* ***Mono-II***: SCC68070, MCD212, DRVDSP (stub), SLAVE
* ***Mono-III***, ***Mono-IV***, ***Robocon***: SCC68070, MCD212, CIAP (stub), IKAT

As a result of DRVDSP and CIAP lacking proper emulation, the corresponding board types cannot run discs. Only Mono-I is currently fully supported.

## To-Do
- [ ] NVRAM/Timekeeper emulation
- [ ] Rewrite emulator backend to use less pointers and garbage (define what this last word means)?
- [ ] Check audiomap-to-XA switching

## Building
To compile, use devkitPro's `powerpc-eabi-cmake` (GC, (v)Wii, Wii U) or `arm-none-eabi-cmake` (3DS), or the regular MINGW64 CMake if compiling for Windows. The corresponding SDL2 package is required, except on 3DS.

The latest commit is compiled automatically using GitHub Actions (`.github/workflows/*.yml`).

## Credits
Special credits to [Stovent](https://github.com/Stovent), [CD-i Fan](https://github.com/cdifan) and [Slamy](https://github.com/Slamy) for helping me wherever possible on this project. The emulator uses [Musashi](https://github.com/kstenerud/Musashi) version 4.10 as a core for the 68070 processor.

Resources:
* Documents/datasheets as found in [CD-i Technical Documentation / System](http://www.icdia.co.uk/docs/index.html) at [The New International CD-i Association](http://www.icdia.co.uk)
* Slamy's documentation of the CDIC (see [CDIC_BlackBoxAnalyzer](https://github.com/Slamy/CDIC_BlackBoxAnalyzer))
* CD-i Fan's [cdichips](https://github.com/cdifan/cdichips) documenting remaining hardware

Some of the emulation code is ported or adapted from:
* Emulation of IKAT and CIAP in Stovent's [CeDImu](https://github.com/Stovent/CeDImu) (license unknown)
* the MAME CD-i driver by Vincent Halver and Ryan Holtz (licensed under BSD-3) ([global MAME license](https://github.com/mamedev/mame?tab=License-1-ov-file))
* [CD-i Emulator](https://www.cdiemu.org/), via reverse engineering of generated trace logs

## License
The general code for this emulator is released under [GPLv3](https://www.gnu.org/licenses/gpl-3.0.html) (see above credits for other licenses). Work is currently underway to remove any derived code that does not conform with this license as stated above.