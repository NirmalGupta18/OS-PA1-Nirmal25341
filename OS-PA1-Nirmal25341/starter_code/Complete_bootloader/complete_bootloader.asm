bits 16
org 0x7c00

start:
    ; The BIOS loads this boot sector at 0x7C00.
    ; TODO: Make SI point to the first character of `message`.
    ; Hint: `lodsb` will use SI.
    mov si, message
    ; ^ SI is a register `lodsb` uses to know where to read from.
    ;   We point it at the start of our message string.


print:
    ; TODO: Read the next character from the string.
    ; After this instruction, AL should contain the character.
    lodsb
    ; ^ "load string byte": reads the byte at [SI] into AL,
    ;   then automatically increments SI to point at the next byte.

    ; Check whether we reached the end of the string.
    cmp al, 0
    je hang

    ; BIOS video service:
    ; AH = 0x0E means "display the character in AL".
    mov ah, 0x0e

    ; TODO: Call the BIOS video service.
    int 0x10
    ; ^ triggers BIOS interrupt 0x10 (video services). With AH=0x0e
    ;   and AL=our character, this actually prints the character to screen.

    ; TODO: Go back and process the next character.
    jmp print
    ; ^ loop back to read/print the next character, until we hit the null byte.


hang:
    ; We are finished printing.
    ; TODO: Disable interrupts.
    cli
    ; ^ "clear interrupts" — stops the CPU from being interrupted,
    ;   so nothing can pull it out of the halt below.

    ; TODO: Halt the CPU.
    hlt
    ; ^ stops the CPU from executing further instructions (low power state).

    ; TODO: Stay here forever.
    jmp $
    ; ^ "$" means "this current address" — so this jumps to itself,
    ;   an infinite loop, just as a safety net in case hlt ever resumes.


message:
    db "Hello from my bootloader!", 0


; A boot sector must be exactly 512 bytes.
; TODO: Fill the unused space with zeroes.
times 510-($-$$) db 0
; ^ "$" = current address, "$$" = start of this section.
;   So (510 - current_size_so_far) tells us how many zero bytes
;   are needed to pad the file up to exactly 510 bytes.


; TODO: Add the boot-sector signature.
dw 0xaa55
; ^ the last 2 bytes (511-512) must be this exact signature.
;   BIOS checks for it to confirm "yes, this is a valid boot sector."
