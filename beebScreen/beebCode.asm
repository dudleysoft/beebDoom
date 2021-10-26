include "os.inc"

BS_CMD_SEND_SCREEN=255
BS_CMD_SEND_PAL=254
BS_CMD_SEND_CRTC=253
BS_CMD_SEND_USER1=252
BS_CMD_SEND_USER2=251
BS_CMD_SEND_QUIT=250

bsTemp2 = &6c
bsFrameCount = &6d
bsTemp = &6e
bsPalCount =&6F
bsPalData = &70

    org &c00
    ; Make sure we don't go over into NMI code
    ; this is all interrupt and vector code so it needs to be resident
    guard &d00

    ; Entry Vector Table
    ; &c00 - Setup Vector
    ; &c03 - Old WRCHV
    ; &c06 - User CMD 1 Vector
    ; &c09 - User CMD 2 Vector
    ; &c0b - VSync Vector
    ; &c0f - Timer Vector
.start
    jsr setupEnv
.continue
    jmp OSWRCH

.user1V
    jmp doRTS
.user2V
    jmp doRTS
.vsyncV
    jmp doRTS
.timerV
    jmp doRTI

.newWRCH
    cmp #BS_CMD_SEND_SCREEN
    beq sendScreen
    cmp #BS_CMD_SEND_PAL
    beq sendPal
    cmp #BS_CMD_SEND_CRTC
    beq sendCrtc
    cmp #BS_CMD_SEND_USER1
    beq user1V
    cmp #BS_CMD_SEND_USER2
    beq user2V
    cmp #BS_CMD_SEND_QUIT
    bne continue
    ; Restore the original vectors
    lda continue+1
    sta WRCHV
    lda continue+2
    sta WRCHV+1
    SEI
    lda #4
    sta &FE6E
    lda oldIRQ
    sta &204
    lda oldIRQ+1
    sta &205
    cli 
    ; Restore A value
    ;lda #BS_CMD_SEND_QUIT
    ; And return
    rts

    ; Processes a CRTC register change
.sendCrtc
    lda HOST_TUBE_R1DATA    ; First value is register
    sta CRTC_REG            ; Store in Register IO address
    lda HOST_TUBE_R1DATA    ; Second value is the new value
    sta CRTC_DATA           ; Store in Data IO address
    ;lda #BS_CMD_SEND_CRTC   ; Restore A (BS_CMD_SEND_CRTC)
    rts                     ; Return

.sendPal
    ;stx bsTemp              ; Hold X register
    lda HOST_TUBE_R1DATA    ; Load the palette count
    asl a                   ; Double the value 
    sta bsPalCount          ; Store in our palette count value
    tax                     ; Index
.sendPalLoop
    lda HOST_TUBE_R1DATA    ; Load next byte of data (pre-formated for NULA)
    sta &FE23
    dex                     ; Increase index
    bne sendPalLoop         ; Loop if not equal
    ;ldx bsTemp              ; Restore X
    ;lda #BS_CMD_SEND_PAL    ; Restore A (BS_CMD_SEND_PAL)
    rts                     ; Return

.sendScreen
    ;stx bsTemp              ; Store X and Y
    ;sty bsTemp2
.ssStoreAddr
    lda HOST_TUBE_R1DATA    ; Load low byte of start address                                    ; 4
    sta ssAddr+1                                                                                ; 5
    lda HOST_TUBE_R1DATA    ; Load high byte of start address                                   ; 4
    sta ssAddr+2                                                                                ; 5
.ssNextByte
    lda HOST_TUBE_R1DATA    ; Read next byte of stream                                          ; 4
    beq sendScreenEnd       ; 0 terminates                                                      ; 2/3
    bmi ssSkip              ; Negative is skip bytes                                            ; 2/3
    tay                     ; Transfer to Y to count bytes                                      ; 2
    tax                     ; Remember in X to add later                                        ; 2
    dey                     ; decrement Y by one                                                ; 2
.ssLoop
    lda HOST_TUBE_R1DATA    ; Load next byte of data                                            ; 4
.ssAddr
    sta &3000,y             ; Store to address (self-modifying)                                 ; 5
    dey                     ; Decrement counter                                                 ; 2
    bpl ssLoop              ; Loop                                                              ; 3 = 14 cycles per byte
    txa                     ; Get back the counter (will be in X now)                           ; 2
.ssInc
    clc                                                                                         ; 2
    adc ssAddr+1            ; Add to address                                                    ; 4
    sta ssAddr+1            ; Write Back                                                        ; 4
    bcc ssNextByte          ; Don't increment if we haven't gone over                           ; 2/3 = 13
    inc ssAddr+2            ; Increment high byte                                               ; 6
    bne ssNextByte          ; loop (will never be zero since video memory can't go above &7fff) ; 3   = 21
.ssSkip
    and #127                ; Remove top bit                                                    ; 3
    bne ssInc               ; If it's non-zero then we've got the number of bytes already       ; 2/3
    beq ssStoreAddr         ; 128 skips to another address                                      ; 3

.sendScreenEnd
    ;ldx bsTemp              ; Restore X
    ;ldy bsTemp2             ; and Y
    ;lda #BS_CMD_SEND_SCREEN ; Restore A (BS_CMD_SEND_SCREEN)
.doRTS
    rts                     ; Return (default return used by all vectors to start with)
.doRTI
    lda &FC
    rti                     ; Return

.irq
	LDA &FE4D:AND #2:BEQ checkTimer ; VSync timer?
    jsr vsyncV              ; If so then call the vsync vector
    inc bsFrameCount        ; Increment our frame counter
.irqReturn
    jmp &0000               ; Then jump to the original IRQ routine
oldIRQ = irqReturn+1
.checkTimer
    BIT &FE6D:BVC irqReturn ; Timer interrupt?
    jmp timerV              ; Call our timer interrupt vector

    ; Setup environment, called once when we get the first character to our modified OSWRCH vector
.setupEnv
    pha                         ; Store A
    lda #0                      ; Initialise the palette counter
    sta bsPalCount              
    lda #LO(newWRCH)            ; Get the proper address for OSWRCH (note the top byte wont change)
    sta WRCHV       
	SEI                         ; Disable interrupts
	LDA #&82:STA &FE4E			; enable VSync and timer interrupt
    lda #&c0:STA &FE6E
	lda &204:sta oldIRQ
	lda &205:sta oldIRQ+1		; Store old IRQ vector
	LDA #LO(irq):STA &204
	LDA #HI(irq):STA &205		; set interrupt handler
	CLI                         ; Enable interrupts again
    pla                         ; Restore A
	rts                         ; Return (takes us back to the original OSWRCH vector since we JSR'd here)
.end

    ; Save the code for beebScreen
SAVE "beebCode.bin",start,end

soundBuffer = &a00
soundHold = &62
timerCount = &63
soundTemp = &64

    org &900
    guard &a00
.centerMouse
    lda #&80
    sta &da6
    lda #2
    sta &da7
    sta &da9
    lda #0
    sta &da8
    lda #BS_CMD_SEND_USER1
    rts
    ; Sends a block of sample data to be written to the fifo buffer
.extraEnd

    ; Save the extra code for BAGI
SAVE "extraCode.bin",&900,extraEnd
