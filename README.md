# miniCDi
An experimental Philips CD-i emulator for the Wii, New 3DS and Wii U. Currently only supports the Mono-I 220/20 player revision, with partial support for Mono-IV boards.

## Credits
Special credits to [Stovent](https://github.com/Stovent), [CD-i Fan](https://github.com/cdifan) and [Slamy](https://github.com/Slamy) for helping me where possible on this project. This project uses partial emulation code ported from the MAME CD-i driver ([see license](https://github.com/mamedev/mame?tab=License-1-ov-file#readme)) as well as Stovent's implementations of the relevant components in [CeDImu](https://github.com/Stovent/CeDImu), and is also based off of CD-i Fan's [cdichips](https://github.com/cdifan/cdichips) documentation of several components including the MCD212, SCC68070 (UART), IKAT and SLAVE and Slamy's documentation of the CDIC (see [CDIC_BlackBoxAnalyzer](https://github.com/Slamy/CDIC_BlackBoxAnalyzer)). The emulator uses [Musashi](https://github.com/kstenerud/Musashi) version 4.10 as a core for the 68070 processor.

## Compatibility
The following boards and chips have been implemented. CD-i Fan has more information regarding hardware at [cdichips](https://github.com/cdifan/cdichips) repository.

* ***Mono-I***: SCC68070, MCD212, CDIC (partial, no audio), SLAVE
* ***~~Mono-II~~***: SCC68070, MCD212, DSP (stub), SLAVE
* ***Mono-III***, ***Mono-IV***, ***Robocon***: SCC68070, MCD212, CIAP (stub), IKAT

## To-Do
### Before official v0.1 release
- [X] Proper reset sequence
- Fix VDSC rendering
   - [X] Color key
   - [ ] RGB decoder formula
   - [ ] DYUV decoder formula
- [ ] Outline instructions/how-to for users
- [ ] Update compatibility information

### Potential
- [X] NVRAM auto-save on exit
- [ ] LibRetro API compatibility?

## Building
To compile, use devkitPro's `powerpc-eabi-cmake` (GC, (v)Wii, Wii U) or `arm-none-eabi-cmake` (3DS). This is automatically done by GitHub Actions.