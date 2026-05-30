# miniCDi
An experimental Philips CD-i emulator for the Wii and 3DS. Currently only supports the Mono-I 220/20 player revision, with partial support for Mono-IV boards.

## Includes
- [X] VDSC (experimental)
- [X] Pointer Device (experimental)
- Master CPU:
  - [X] SCC68070 (Mini-MMC, Maxi-MMC, Mono-I, Mono-II, Mono-III, Mono-IV and Robocon)
- Video:
  - [X] MCD212
- CD+Audio:
  - [ ] CDIC (Mono-I)
  - [ ] CIAP (Mono-IV)
- Microcontrollers:
  - [X] SLAVE (Mono-I and Mono-II)
  - [ ] IKAT (Mono-III, Mono-IV and Robocon)

## To-Do
- [ ] Proper reset sequence
- [ ] Emulator CPU/chip timing

## Building
To compile, use devkitPro's `powerpc-eabi-cmake` (GC, (v)Wii, Wii U) or `arm-none-eabi-cmake` (3DS).

## Credits
Special credits to [Stovent](https://github.com/Stovent), [CD-i Fan](https://github.com/cdifan) and [Slamy](https://github.com/Slamy) for helping me on this project. The source code was partially based off of Stovent's implementations of the relevant components in [CeDImu](https://github.com/Stovent/CeDImu), as well as CD-i Fan's [cdichips](https://github.com/cdifan/cdichips) documentation of several components including the MCD212, SCC68070 (UART), IKAT and SLAVE. The emulator uses [Musashi](https://github.com/kstenerud/Musashi) version 4.10 as a core for the 68070 processor.