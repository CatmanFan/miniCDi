# <div align=center><img src="https://github.com/CatmanFan/miniCDi/blob/master/res/logo.png" width="25%" /></div>

An experimental Philips CD-i emulator meant to run on embedded consoles such as the Wii, 3DS and Wii U. Audio support is currently partially broken but it can run commercial games and homebrew and is designed to be somewhat portable.

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

## Compatibility
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

### Before official v0.1 beta release
- [X] Fix soundmap issue
- [ ] Update compatibility information

### Potential
- [ ] Use global namespace instead of class? (e.g. `CDi::init()`)
- [ ] Audio playback support for native homebrew libraries (i.e. non-SDL)
- [ ] Emulate timekeeper on Mono-I/Mono-IV? (should handle NVRAM saving)
- [ ] Fix PD on Mono-IV
- Disc-related:
   - [ ] CDIC: Address slowdown when reading sectors (only noticeable on embedded platforms?)
   - [ ] CIAP: Read discs properly
- [ ] LibRetro API compatibility?

## Building
To compile, use devkitPro's `powerpc-eabi-cmake` (GC, (v)Wii, Wii U) or `arm-none-eabi-cmake` (3DS). This is automatically done by GitHub Actions on every commit.

## Credits
Special credits to [Stovent](https://github.com/Stovent), [CD-i Fan](https://github.com/cdifan) and [Slamy](https://github.com/Slamy) for helping me where possible on this project. The emulator uses [Musashi](https://github.com/kstenerud/Musashi) version 4.10 as a core for the 68070 processor.

Some of the emulation code is ported or adapted from:
* CD-i Fan's [cdichips](https://github.com/cdifan/cdichips) documentation of several components including the MCD212, SCC68070 (UART), IKAT and SLAVE
* Slamy's documentation of the CDIC (see [CDIC_BlackBoxAnalyzer](https://github.com/Slamy/CDIC_BlackBoxAnalyzer))
* Stovent's implementations of the relevant components in [CeDImu](https://github.com/Stovent/CeDImu) (license unknown)
* the MAME CD-i driver by Vincent Halver and Ryan Holtz (licensed under BSD-3) ([global MAME license](https://github.com/mamedev/mame?tab=License-1-ov-file))
* reverse engineering of the [CD-i Emulator](https://www.cdiemu.org/) trace log

## License
The general code for this emulator is released under [GPLv3](https://www.gnu.org/licenses/gpl-3.0.html) (see above credits for other licenses).