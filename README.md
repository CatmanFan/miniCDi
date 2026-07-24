# miniCDi
<div align=center><a href=""><img src="https://github.com/CatmanFan/miniCDi/blob/master/res/wii/icon.png" /></a></div>

An experimental Philips CD-i emulator for the Wii, New 3DS and Wii U.

## Usage
### Windows
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

## Compatibility
The following boards and chips have been implemented. CD-i Fan has more information regarding hardware at [cdichips](https://github.com/cdifan/cdichips) repository.

* ***Mono-I***: SCC68070, MCD212, CDIC, SLAVE
* ***Mono-II***: SCC68070, MCD212, DRVDSP (stub), SLAVE
* ***Mono-III***, ***Mono-IV***, ***Robocon***: SCC68070, MCD212, CIAP (stub), IKAT

Only the Mono-I driver is capable of playing CD-i discs, since the DRVDSP and CIAP in later boards are not fully emulated.

### Profiling
Time taken to process and render a single frame on:

* Windows (x64, Nvidia + Intel i5-12400F): ***~1.7ms*** (fullspeed)
* Wii (vWii overclock): ***~33.77ms*** (~29 fps)
* Wii (normal): ***~56.44ms*** (~18 fps)
* Wii U (WUHB): ***~58.51ms*** (~17 fps)
* New 3DS: ***~78.79ms*** (~13 fps)
* Old 3DS: ***~237ms*** (~4 fps)

All profiled times with the exception of the Windows version are longer than the minimum needed for throttling (16.667ms or 1/60 secs). All are compiled using the fastest compile optimizations available under GCC.

## Credits
Special credits to [Stovent](https://github.com/Stovent), [CD-i Fan](https://github.com/cdifan) and [Slamy](https://github.com/Slamy) for helping me where possible on this project. This project uses partial emulation code ported from the MAME CD-i driver ([see license](https://github.com/mamedev/mame?tab=License-1-ov-file#readme)) as well as Stovent's implementations of the relevant components in [CeDImu](https://github.com/Stovent/CeDImu), and is also based off of CD-i Fan's [cdichips](https://github.com/cdifan/cdichips) documentation of several components including the MCD212, SCC68070 (UART), IKAT and SLAVE and Slamy's documentation of the CDIC (see [CDIC_BlackBoxAnalyzer](https://github.com/Slamy/CDIC_BlackBoxAnalyzer)). The emulator uses [Musashi](https://github.com/kstenerud/Musashi) version 4.10 as a core for the 68070 processor.

## To-Do

### Before official v0.1 beta release
- [ ] Update compatibility information

### Potential
- [ ] Audio playback support for native homebrew libraries (i.e. non-SDL)
- [ ] Emulate timekeeper on Mono-I/Mono-IV? (should handle NVRAM saving)
- [ ] Fix PD on Mono-IV
- Disc-related:
   - [ ] CDIC: Address slowdown when reading sectors (not necessary anymore?)
   - [ ] CIAP: Read discs properly
- [ ] LibRetro API compatibility?

## Building
To compile, use devkitPro's `powerpc-eabi-cmake` (GC, (v)Wii, Wii U) or `arm-none-eabi-cmake` (3DS). This is automatically done by GitHub Actions on every commit.

## License
General code is licensed under GPLv3. The original source code for the MAME CD-i driver is by Vincent Halver and Ryan Holtz and is licensed under BSD-3 (see [here](https://github.com/mamedev/mame?tab=License-1-ov-file) for global MAME license).