# miniCDi
An experimental Philips CD-i emulator for the Wii and 3DS. Currently only supports the Mono-I 220/20 player revision, with partial support for Mono-IV boards.

## Compatibility
The following boards and chips have been implemented. CD-i Fan has more information regarding hardware at [cdichips](https://github.com/cdifan/cdichips) repository.

* ***Mono-I***: SCC68070, MCD212, CDIC (partial), SLAVE
* ***Mono-II***: SCC68070, MCD212, ~~DSP~~, SLAVE
* ***Mono-III***, ***Mono-IV***, ***Robocon***: SCC68070, MCD212, CIAP (partial, no audio), IKAT

## To-Do
### Bugs
- [ ] Weight Factor on Mixing mode (cf. second player shell, does not occur on Mono-I)
- [ ] Address DMA bug which causes CD-i applications to eventually halt
### Other
- [ ] Proper reset sequence
- [ ] Address emulator CPU/chip timing
- [ ] LibRetro API compatibility?

## Building
To compile, use devkitPro's `powerpc-eabi-cmake` (GC, (v)Wii, Wii U) or `arm-none-eabi-cmake` (3DS).

## Credits
Special credits to [Stovent](https://github.com/Stovent), [CD-i Fan](https://github.com/cdifan) and [Slamy](https://github.com/Slamy) for helping me on this project. The source code was partially based off of Stovent's implementations of the relevant components in [CeDImu](https://github.com/Stovent/CeDImu), as well as CD-i Fan's [cdichips](https://github.com/cdifan/cdichips) documentation of several components including the MCD212, SCC68070 (UART), IKAT and SLAVE. The emulator uses [Musashi](https://github.com/kstenerud/Musashi) version 4.10 as a core for the 68070 processor.