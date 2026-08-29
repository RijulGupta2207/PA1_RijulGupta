bits 16
org 0x7c00

start:
    ; The BIOS loads this boot sector at 0x7C00.
    ; Make SI point to the first character of `message`.
    ; `lodsb` will use SI to read characters.
    mov si, message


print:
    ; Read the next character from the string.
    ; After this instruction, AL contains the character
    ; and SI is automatically incremented.
    lodsb

    ; Check whether we reached the end of the string.
    cmp al, 0
    je hang

    ; BIOS video service:
    ; AH = 0x0E means "display the character in AL".
    mov ah, 0x0e

    ; Call the BIOS video service.
    int 0x10

    ; Go back and process the next character.
    jmp print


hang:
    ; We are finished printing.
    ; Disable interrupts so the CPU cannot be woken up.
    cli

    ; Halt the CPU.
    hlt

    ; If the CPU somehow wakes up, jump back to hang
    ; to halt it again (infinite loop as a safety net).
    jmp hang


message:
    db "Hello from my bootloader!", 0


; A boot sector must be exactly 512 bytes.
; Fill the unused space with zeroes up to byte 510.
times 510-($-$$) db 0


; Add the boot-sector signature at bytes 510-511.
; The BIOS checks for 0x55AA to confirm a valid boot sector.
dw 0xAA55
