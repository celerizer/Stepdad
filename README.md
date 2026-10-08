# Stepdad
![stepdad](https://github.com/celerizer/ywnbaw/assets/33245078/5e0f23ac-148d-44af-912b-7c5182323ac4)

Stepdad is an emulator that aims to emulate certain Hitachi H8/300H embedded IR receivers and pedometer devices, for the purpose of communicating with the [melonDS](https://melonds.kuribo64.net/) emulator.

## Supported devices

- NTR-027 (crc32 `82341b9f`): Simple 1-button pedometer device with a two-color LED
- NTR-031 (crc32 `64b40d8d` or `9321792f`): Embedded IR receiver that acts as a serial bus for a game cartridge
- NTR-032 (crc32 `d4a05446`): Complex 3-button pedometer device with grayscale LCD and sound

## Planned features

- [x] H8 CPU emulation
- [x] One-button and three-button physical input
- [x] Reading and writing to 8K and 64K EEPROMs
- [x] LCD and LED video support
- [x] Sound support
- [x] Accelerometer spoofing for step counting
- [ ] IR and SPI forwarding over local network on a configurable port
- [ ] Full melonDS integration

## Special thanks

- nocash for the [GBATEK documentation](http://problemkaputt.de/gbatek-ds-cart-infrared-pedometers.htm)
- PoroCYon for [SPI bus documentation](https://melonds.kuribo64.net/board/thread.php?pid=2762#2762) and [IR dumper](https://git.titandemo.org/PoroCYon/pokewalker-rom-dumper)
- Dmitry for the [fascinating reverse engineering article](https://dmitry.gr/?r=05.Projects&proj=28.%20pokewalker)
- Lawson and Molly for providing natural sample data
- Carol Vorderman
