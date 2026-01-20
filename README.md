<p align="center">
  <img src="piflasher_logo-removebg.png" alt="PiFlasher Logo" width="300">
</p>

<h1 align="center">PiFlasher</h1>
<h3 align="center">Xbox 360 NAND Flasher for Raspberry Pi 4</h3>

<p align="center">
A complete rewrite of xbox360-rpi-flasher with universal NAND support, auto-detection, and comprehensive documentation.
</p>

---

## Table of Contents

- [Overview](#overview)
- [Credits](#credits)
- [Original vs This Fork](#original-vs-this-fork)
- [Features](#features)
- [Supported Consoles](#supported-consoles)
- [Hardware Setup](#hardware-setup)
- [Installation](#installation)
- [Usage](#usage)
- [Technical Deep Dive](#technical-deep-dive)
  - [What is NAND Flash](#what-is-nand-flash)
  - [Xbox 360 NAND Architecture](#xbox-360-nand-architecture)
  - [The Boot Chain](#the-boot-chain)
  - [NAND Controller Hardware](#nand-controller-hardware)
  - [SPI Protocol](#spi-protocol)
  - [Flash Configuration](#flash-configuration)
  - [Read Algorithm](#read-algorithm)
  - [Write Algorithm](#write-algorithm)
  - [Erase Algorithm](#erase-algorithm)
- [How The Original Code Worked](#how-the-original-code-worked)
- [What I Changed And Why](#what-i-changed-and-why)
- [RGH3 Explained](#rgh3-explained)
- [JRunner Status](#jrunner-status)
- [Troubleshooting](#troubleshooting)
- [References](#references)
- [License](#license)

---

## Overview

PiFlasher enables reading and writing Xbox 360 NAND flash memory using a Raspberry Pi 4. This fork completely rewrites the original xbox360-rpi-flasher to support all NAND sizes with automatic detection.

---

## The Story Behind This Fork

I was keen on doing RGH3 modifications on my Xbox 360 consoles but didn't have a flasher. Rather than wait a month for a PicoFlasher to arrive from China, I discovered the original xbox360-rpi-flasher project and decided to use my Raspberry Pi 4 instead.

**The successes:** I successfully RGH3'd two consoles using the original project:
- Jasper 16MB
- Falcon 16MB

**The disaster:** Then came my third console - a Jasper 512MB Big Block. I read the NAND three times, compared the dumps (all matched), created the XeLL image with JRunner, and flashed it. The console was bricked.

What I didn't realize was that the original project was **hardcoded for 16MB NANDs only**. When I flashed my 512MB Jasper, it only wrote to the first 16MB worth of blocks with incorrect block sizing, corrupting the NAND structure.

**The recovery:** With a bricked console and no way to boot, I had two choices: wait for the PicoFlasher to arrive, or fix the software myself. I chose to rewrite the entire project with proper NAND size detection. After implementing dynamic flash configuration reading and proper Big Block support, I was able to perform a full recovery of my bricked Jasper 512MB. The console now works perfectly with RGH3.

This fork exists so nobody else has to brick their console learning this lesson the hard way.

---

## Credits

**Original Authors:** xbox360-rpi-flasher contributors  
**Confirmed Working (Original):** Corona 16MB (@mav2010), Jasper 16MB (@fabien4455)  
**Confirmed Working (This Fork):** Jasper 512MB Big Block (@BRMilev22)

---

## Original vs This Fork

### Original Limitations

The original xbox360-rpi-flasher was functional but limited:

```c
// Hardcoded for 16MB only
#define NAND_BLOCKS 0x400  // 1024 blocks = 16MB
```

**Problems:**
- Only 16MB NANDs supported
- Manual code edits required for different sizes
- No auto-detection
- Required recompilation for each operation
- Bitbanged SPI (slow)

### This Fork Improvements

```c
// Dynamic detection
if (major >= 1) {
    if (minor == 3) size = 512;      // 512MB Big Block
    else if (minor == 2) size = 256; // 256MB Big Block
    else size = 16;                   // 16MB Big Block
} else size = 16;                     // 16MB Small Block
```

**Improvements:**
- All NAND sizes auto-detected
- Interactive menu system
- Hardware SPI at 10MHz (200x faster)
- Modular code architecture
- PicoFlasher protocol for JRunner

---

## Features

- Universal NAND support (16/256/512MB)
- Automatic console detection
- Interactive CLI menu
- Triple-read verification
- 10MHz hardware SPI
- Zero-warning build

---

## Supported Consoles

| Motherboard | NAND | Block Type | Status |
|-------------|------|------------|--------|
| Xenon/Zephyr/Falcon/Opus | 16MB | Small | Supported |
| Jasper 16MB | 16MB | Big | Supported |
| Jasper 256MB | 256MB | Big | Supported |
| Jasper 512MB | 512MB | Big | Tested |
| Trinity | 16MB | Small | Supported |
| Corona (SPI) | 16MB | Small | Supported |
| Corona (eMMC) | 4GB | eMMC | Not Supported |

---

## Hardware Setup

### Wiring

| Pi4 GPIO | Xbox Pin | Function |
|----------|----------|----------|
| GPIO 10 | MOSI | Data Out |
| GPIO 9 | MISO | Data In |
| GPIO 11 | SCLK | Clock |
| GPIO 26 | SS | Chip Select |
| GPIO 23 | XX | SMC Reset |
| GPIO 24 | EJ | Debug Enable |
| GND | GND | Ground |

**Important:** Xbox must be in standby (power connected, not turned on).

---

## Installation

```bash
# Install dependencies
sudo apt-get install pigpio

# Enable SPI
sudo raspi-config  # Interface Options -> SPI -> Enable

# Build
git clone https://github.com/BRMilev22/xbox360-rpi-flasher
cd xbox360-rpi-flasher
make
```

---

## Usage

```bash
sudo ./bin/pi4flasher          # Interactive mode
sudo ./bin/pi4flasher info     # Show NAND info
sudo ./bin/pi4flasher read f   # Read to file
sudo ./bin/pi4flasher write f  # Write from file
```

---

## Technical Deep Dive

### What is NAND Flash

NAND flash is non-volatile memory organized into pages and blocks. Key characteristics:

**Physical Structure:**
```
NAND Chip
└── Blocks (erasable unit)
    └── Pages (writable unit)
        └── Bytes + Spare Area
```

**Operations:**
- **Read:** Can read any page
- **Write:** Can only change 1→0
- **Erase:** Sets entire block to 1s

This is why erase is required before write - you cannot flip 0 back to 1 without erasing.

**Source:** [NAND Flash Basics - Embedded](https://www.embedded.com/flash-101-nand-flash-vs-nor-flash/)

### Xbox 360 NAND Architecture

The Xbox 360 stores critical system data in NAND:

**Memory Map:**
```
0x000000-0x003FFF  1BL (Bootrom in silicon)
0x004000-0x007FFF  CB_A (Second Bootloader)
0x008000-0x00BFFF  CB_B (Backup)
0x00C000-0x00FFFF  CD (Fourth Bootloader)
0x010000-0x01FFFF  CE (Hypervisor Loader)
0x020000-0x03FFFF  CF (Kernel Loader)
0x040000-0x047FFF  CG (Config)
0x048000-0x07FFFF  SMC Firmware
0x080000-0x0FFFFF  Keyvault (unique per console)
0x100000-0xFFFFFF  Filesystem
```

**Source:** [Free60 Project](https://free60project.github.io/)

### The Boot Chain

Xbox 360 uses a chain-of-trust boot process:

```
1BL (ROM) ──RSA──> CB ──HMAC──> CD ──> CE ──> CF ──> Kernel
                    ▲
                    │
              RGH attacks here
```

Each stage verifies the next using cryptographic signatures. RGH exploits the verification timing.

**Source:** [Xbox 360 Security Research](https://beta.ivc.no/wiki/index.php/Xbox_360_Hacking)

### NAND Controller Hardware

The Xbox 360 doesn't expose raw NAND pins. Instead, a custom controller provides an SPI interface:

**Block Diagram:**
```
CPU ──> Southbridge ──> NAND Controller ──> SPI Bus ──> NAND Chip
                              │
                        Registers:
                        0x00 CONFIG
                        0x04 STATUS
                        0x08 COMMAND
                        0x0C ADDRESS
                        0x10 DATA
```

I communicate with the controller, not the NAND directly.

**Source:** [Free60 Project - Hardware](https://free60project.github.io/)

### SPI Protocol

SPI (Serial Peripheral Interface) uses 4 wires:

```
      Master (Pi4)              Slave (Xbox)
          │                         │
   SCLK ──┼────────────────────────>│  Clock
   MOSI ──┼────────────────────────>│  Data to slave
   MISO ──┼<────────────────────────│  Data from slave
   SS_N ──┼────────────────────────>│  Enable (active low)
```

**Xbox 360 SPI Specifics:**
- Mode 0 (CPOL=0, CPHA=0)
- LSB first (unusual - requires bit reversal)
- 3.3V logic levels

**Command Format:**
```
Read:  [reg<<2|0x01][0xFF][byte0][byte1][byte2][byte3]
Write: [reg<<2|0x02][byte0][byte1][byte2][byte3]
```

**Bit Reversal Required:**
```c
uint8_t reverse(uint8_t b) {
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}
```

**Source:** [SPI Protocol Overview](https://en.wikipedia.org/wiki/Serial_Peripheral_Interface)

### Flash Configuration

Register 0x00 contains NAND configuration:

```
Bits 17-18: Major version
Bits 4-5:   Minor version

Decoding:
major=0:           Small Block (16KB erase)
major>=1, minor=0: 16MB Big Block (128KB)
major>=1, minor=2: 256MB Big Block (128KB)
major>=1, minor=3: 512MB Big Block (256KB)
```

**Example Values:**
| Console | Config | Size |
|---------|--------|------|
| Corona | 0x00043000 | 16MB SB |
| Jasper 512 | 0x01198030 | 512MB BB |

### Read Algorithm

```c
int read_sector(uint32_t sector, uint8_t *data, uint8_t *spare) {
    // 1. Clear status register
    uint32_t status = read_reg(0x04);
    write_reg(0x04, status);
    
    // 2. Set address (sector * 512)
    write_reg(0x0C, sector << 9);
    
    // 3. Issue read command
    write_reg(0x08, 0x03);
    
    // 4. Wait for completion
    while (read_reg(0x04) & 0x01);
    
    // 5. Reset data pointer
    write_reg(0x0C, 0);
    
    // 6. Read 512 bytes data
    for (int i = 0; i < 128; i++) {
        write_reg(0x08, 0x00);  // Clock data
        ((uint32_t*)data)[i] = read_reg(0x10);
    }
    
    // 7. Read 16 bytes spare
    for (int i = 0; i < 4; i++) {
        write_reg(0x08, 0x00);
        ((uint32_t*)spare)[i] = read_reg(0x10);
    }
    
    return 0;
}
```

**Why this works:** The controller buffers data after the read command. I clock it out 4 bytes at a time.

### Write Algorithm

```c
int write_sector(uint32_t sector, uint8_t *data, uint8_t *spare) {
    // 1. Erase block if at boundary
    if (sector % sectors_per_block == 0) {
        erase_block(sector);
    }
    
    // 2. Reset pointer
    write_reg(0x0C, 0);
    
    // 3. Write data to buffer
    for (int i = 0; i < 128; i++) {
        write_reg(0x10, ((uint32_t*)data)[i]);
        write_reg(0x08, 0x01);
    }
    
    // 4. Write spare to buffer
    for (int i = 0; i < 4; i++) {
        write_reg(0x10, ((uint32_t*)spare)[i]);
        write_reg(0x08, 0x01);
    }
    
    // 5. Wait ready
    while (read_reg(0x04) & 0x01);
    
    // 6. Set target address
    write_reg(0x0C, sector << 9);
    
    // 7. Execute program sequence
    write_reg(0x08, 0x55);  // Unlock
    write_reg(0x08, 0xAA);  // Confirm
    write_reg(0x08, 0x04);  // Execute
    
    // 8. Wait complete
    while (read_reg(0x04) & 0x01);
    
    return 0;
}
```

**Why erase first:** NAND can only change 1→0. The 0x55/0xAA sequence unlocks write protection.

### Erase Algorithm

```c
int erase_block(uint32_t sector) {
    // 1. Enable write
    uint32_t cfg = read_reg(0x00);
    write_reg(0x00, cfg | 0x08);
    
    // 2. Set address
    write_reg(0x0C, sector << 9);
    
    // 3. Execute erase
    write_reg(0x08, 0xAA);  // Block mode
    write_reg(0x08, 0x55);  // Unlock
    write_reg(0x08, 0x05);  // Erase
    
    // 4. Wait complete
    while (read_reg(0x04) & 0x01);
    
    return 0;
}
```

---

## How The Original Code Worked

The original xbox360-rpi-flasher used a simple approach:

```c
// XSPI.c - Direct GPIO bitbanging
void spiex_write(uint8_t *buf, int len) {
    for each byte:
        for each bit:
            set MOSI
            toggle CLK
            read MISO
}
```

**Why they did this:**
1. Simple to implement
2. No dependencies on SPI libraries
3. Works on any GPIO-capable Pi

**Limitations:**
1. Very slow (~50KHz)
2. Hardcoded 16MB (0x400 blocks)
3. No error recovery

---

## What I Changed And Why

### 1. Dynamic NAND Detection

**Original:** `#define NAND_BLOCKS 0x400`

**Mine:**
```c
XNAND_Config conf = XNAND_GetConfig();
int major = (conf.flash_config >> 17) & 3;
int minor = (conf.flash_config >> 4) & 3;
// ... determine size from major/minor
```

**Why:** Users couldn't flash 256MB/512MB Jaspers without editing code.

### 2. Hardware SPI

**Original:** Bitbanged GPIO at ~50KHz

**Mine:** pigpio hardware SPI at 10MHz

**Why:** 200x faster reads/writes. A 512MB dump takes minutes instead of hours.

### 3. Modular Architecture

**Original:** All code in 3 files with mixed concerns

**Mine:**
```
gpio.c    - Hardware abstraction
xspi.c    - SPI protocol
xnand.c   - NAND operations
protocol.c - JRunner protocol
main.c    - User interface
```

**Why:** Easier to maintain, test, and extend.

### 4. Interactive Menu

**Original:** Edit code, recompile, run

**Mine:** Menu system with all operations

**Why:** Much more user-friendly, especially for beginners.

### 5. Auto-Detection Display

**Original:** No feedback until operation

**Mine:** Shows console info on startup

**Why:** Users know immediately if connection works.

---

## RGH3 Explained

RGH (Reset Glitch Hack) exploits timing in the boot process:

**Normal Boot:**
```
1BL → Load CB → Verify Signature → SUCCESS → Continue
```

**Glitched Boot:**
```
1BL → Load CB → Verify Signature → [GLITCH PULSE] → 
                Corrupted check returns SUCCESS → Continue with unsigned code
```

**RGH3 Specifics:**
- SMC firmware modified to send glitch
- No external chip needed
- Waits for POST 0xD8, glitches at 0xDA
- Timing stored in SMC config

**Source:** [ConsoleMods RGH3 Guide](https://consolemods.org/wiki/Xbox_360:RGH/RGH3)

---

## JRunner Status

**Status:** Protocol implemented, integration incomplete

JRunner source code compiles but has non-functional kernel dropdown. Pre-compiled releases work correctly.

**Workaround:** Use PiFlasher standalone with JRunner release binaries.

**Help wanted:** If you can fix JRunner source compilation, please contribute.

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| GPIO init failed | Run with sudo, check pigpiod |
| Flash Config 0x00000000 | Check wiring, Xbox must be in standby |
| Read errors | Reduce SPI speed, check connections |
| Write failures | Verify XX/EJ connected |

---

## References

1. [Free60 Project](https://free60project.github.io/) - Xbox 360 technical documentation
2. [ConsoleMods Wiki](https://consolemods.org/wiki/Xbox_360) - Modding guides
3. [PicoFlasher GitHub](https://github.com/X360Tools/PicoFlasher) - Protocol reference
4. [WeekendModder](https://weekendmodder.com/picoflasher) - Wiring guides
5. [J-Runner with Extras](https://github.com/Octal450/J-Runner-with-Extras) - Windows tool
6. [SPI Protocol - Wikipedia](https://en.wikipedia.org/wiki/Serial_Peripheral_Interface) - SPI basics
7. [NAND Flash - Wikipedia](https://en.wikipedia.org/wiki/Flash_memory) - Flash memory concepts
8. [Xbox 360 Security Wiki](https://beta.ivc.no/wiki/index.php/Xbox_360_Hacking) - Security research

---

## License

MIT License - See [LICENSE](LICENSE)

---

## Disclaimer

For educational and backup purposes only. Modifying consoles may void warranty. Always backup NAND first. Not responsible for any damage.
