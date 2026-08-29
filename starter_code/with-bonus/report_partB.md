# Part B — Bootloader Report

## Implementation Overview

The bootloader is a 16-bit x86 assembly program that fits within a single 512-byte boot sector. When the system boots, the BIOS loads this boot sector into memory at address `0x7C00` and transfers control to it. The bootloader prints the message "Hello from my bootloader!" to the screen using BIOS interrupt services and then halts the CPU.

### How it works:
1. The `SI` register is loaded with the address of the message string.
2. The `lodsb` instruction reads one byte from `[SI]` into `AL` and increments `SI`.
3. Each character is printed using BIOS interrupt `0x10` with function `0x0E` (teletype output).
4. The loop continues until a null terminator (`0x00`) is encountered.
5. Once printing is done, interrupts are disabled (`cli`), and the CPU is halted (`hlt`).

---

## Questions and Answers

### 1. Why is the bootloader limited to 512 bytes?

The bootloader is limited to 512 bytes because the BIOS is hardcoded to read exactly **one disk sector** (512 bytes) from the first sector of the boot device (sector 0, head 0, cylinder 0). This convention was established with the original IBM PC in 1981 and has remained the standard for legacy BIOS booting. The BIOS does not know anything about file systems or operating systems — it simply reads 512 bytes, verifies the boot signature, and jumps to it. If a more complex bootloader is needed (like GRUB), the first 512-byte stage loads a second, larger stage from disk.

### 2. Why is 0x7C00 significant?

`0x7C00` is the fixed memory address where the BIOS loads the boot sector. This address was chosen during the design of the IBM PC 5150. The original PC had 32 KB of RAM (addresses `0x0000` to `0x7FFF`). The designers needed to reserve space for:
- The Interrupt Vector Table (IVT) at `0x0000–0x03FF`
- The BIOS Data Area (BDA) at `0x0400–0x04FF`
- The boot sector code itself (512 bytes)
- A stack for the boot sector to use

`0x7C00` was chosen to place the boot sector at the end of the usable memory, leaving room above `0x0500` for the boot sector's stack and data, while the 512 bytes from `0x7C00` to `0x7DFF` hold the bootloader code. The `org 0x7C00` directive in the assembly source tells the assembler to calculate all addresses relative to this load address.

### 3. What does the 0x55AA boot signature represent?

The two bytes `0x55` and `0xAA` at the very end of the boot sector (bytes 510 and 511) form the **boot signature** or **magic number**. The BIOS checks for this signature after reading the first 512 bytes from a disk. If the last two bytes are `0x55` followed by `0xAA`, the BIOS considers the disk bootable and jumps to the code at `0x7C00`. If the signature is absent, the BIOS skips that disk and tries the next boot device in the boot order. In the assembly source, `dw 0xAA55` stores the word in little-endian byte order as `0x55, 0xAA`, which is exactly what the BIOS expects.

### 4. How is QEMU involved in the boot process?

QEMU is a hardware emulator that simulates an entire x86 computer system, including CPU, RAM, storage devices, display, and — crucially — a BIOS firmware. When we run:

```
qemu-system-x86_64 -drive format=raw,file=boot.bin
```

QEMU performs the following:
1. **Initializes a virtual x86 machine** with emulated CPU, memory, and peripherals.
2. **Loads its built-in BIOS** (SeaBIOS), which runs just like a real BIOS on physical hardware.
3. The emulated BIOS performs **POST** (Power-On Self-Test) and then looks for bootable devices.
4. It treats `boot.bin` as a raw disk image and **reads the first 512 bytes** from it.
5. It checks for the `0x55AA` signature at the end — finds it, so the disk is bootable.
6. It **loads those 512 bytes to memory address `0x7C00`** and transfers CPU control there.
7. Our bootloader code executes, prints the message, and halts.

QEMU allows us to test bootloaders without needing physical hardware or writing to actual disks, making it an essential tool for OS development and education.

## Build and Test Commands

```bash
# Assemble the bootloader
nasm -f bin boot.asm -o boot.bin

# Run in QEMU
qemu-system-x86_64 -drive format=raw,file=boot.bin
```
