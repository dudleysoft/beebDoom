; OS Defines
OSWRCH = &FFEE
WRCHV = &20E
OSNEWL = &FFE7
OSASCI = &FFE3
OSRDCH = &FFE0
RDCHV = &210
OSCLI = &FFF7
CLIV = &208
OSBYTE = &FFF4
BYTEV = &20A
OSWORD = &FFF1
WORDV = &20C
OSRDSC = &FFB9
OSWRSC = &FFB3
OSEVEN = &FFBF
OSFILE = &FFDD
FILEV = &212
OSARGS = &FFDA
ARGSV = &214
OSBGET = &FFD7
BGETV = &216
OSBPUT = &FFD4
BPUTV = &218
OSGBPB = &FFD1
GBPBV = &21A
OSFIND = &FFCE
FINDV = &21C
FSCV = &21E

CRTC_REG = &FE00
CRTC_DATA = &FE01

MACRO CRTCI reg,value
	lda #reg
	sta &FE00
	lda #value
	sta &FE01
ENDMACRO

MACRO CRTCA reg,addr
	lda #reg
	sta &FE00
	lda addr
	sta &FE01
ENDMACRO

MACRO TIMER1 time
	lda #LO(time):STA &FE44
	lda #HI(time):STA &FE45
ENDMACRO

MACRO TIMER2 time
	lda #LO(time):STA &FE48
	lda #HI(time):STA &FE49
ENDMACRO

MACRO PALI logical,physical
	lda #((logical and 1) << 5) OR ((logical and 2) << 6) OR (physical eor 7)
	sta &FE21
	ora #&10
	sta &FE21
	eor #&50
	sta &FE21
	ora #&10
	sta &FE21
ENDMACRO

MACRO PALA logical,addr
	lda addr
	eor #((logical and 1) << 5) OR ((logical and 2) << 6) OR 7
	sta &FE21
	ora #&10
	sta &FE21
	eor #&50
	sta &FE21
	ora #&10
	sta &FE21
ENDMACRO

MACRO CALLOSWORD a,addr
	lda #a
	ldx #LO(addr)
	ldy #HI(addr)
	jsr OSWORD
ENDMACRO

MACRO OSBYTEN a
	lda #a
	jsr OSBYTE
ENDMACRO

MACRO OSBYTEX a,xval
	lda #a
	ldx #xval
	jsr OSBYTE
ENDMACRO

MACRO OSBYTEXY a,xval,yval
	lda #a
	ldx #xval
	ldy #yval
	jsr OSBYTE
ENDMACRO

MACRO OSCLII addr
    ldx #LO(addr)
    ldy #HI(addr)
    jsr OSCLI
ENDMACRO

MACRO OSCLIA addr
    ldx addr
    ldy addr+1
    jsr OSCLI
ENDMACRO

MACRO OSWRCHI chr
	lda #chr
	jsr OSWRCH
ENDMACRO

MACRO OSWRCHA addr
	lda addr
	jsr OSWRCH
ENDMACRO

MACRO OSWRCHX addr
	lda addr,x
	jsr OSWRCH
ENDMACRO

MACRO OSWRCHZP addr
	lda (addr),Y
	jsr OSWRCH
ENDMACRO

; Keyboard scancodes

KEY_SPACE = &9D			; 0
KEY_COMMA = &99			; 1
KEY_MINUS = &E8			; 2
KEY_PERIOD = &98		; 3
KEY_SLASH = &87			; 4
KEY_TILDE = &E7			; 5
KEY_UNDERSCORE = &D7	; 6
KEY_0 = &D8				; 7
KEY_1 = &CF				; 8
KEY_2 = &CE				; 9
KEY_3 = &EE				; 10
KEY_4 = &ED				; 11
KEY_5 = &EC				; 12
KEY_6 = &CB				; 13
KEY_7 = &DB				; 14
KEY_8 = &EA				; 15
KEY_9 = &D9				; 16
KEY_COLON = &B7			; 17
KEY_SEMICOLON = &A8		; 18
KEY_AT = &B8			; 19
KEY_A = &BE				; 20
KEY_B = &9B				; 21
KEY_C = &AD				; 22
KEY_D = &CD				; 23
KEY_E = &DD				; 24
KEY_F = &BC				; 25
KEY_G = &AC				; 26
KEY_H = &AB				; 27
KEY_I = &DA				; 28
KEY_J = &BA				; 29
KEY_K = &B9				; 30
KEY_L = &A9				; 31
KEY_M = &9A				; 32
KEY_N = &AA				; 33
KEY_O = &C9				; 34
KEY_P = &C8				; 35
KEY_Q = &FE				; 36
KEY_R = &CC				; 37
KEY_S = &AE				; 38
KEY_T = &DC				; 39
KEY_U = &CA				; 40
KEY_V = &9C				; 41
KEY_W = &DE				; 42
KEY_X = &BD				; 43
KEY_Y = &BB				; 44
KEY_Z = &9E				; 45
KEY_OPENBRACE = &C7		; 46
KEY_BACKSLASH = &E7		; 47
KEY_CLOSEBRACE = &D7	; 48
KEY_ESCAPE = &8F		; 49
KEY_TAB = &9F			; 50
KEY_CAPSLOCK = &BF		; 51
KEY_CTRL = &FE			; 52
KEY_SHIFTLOCK = &AF		; 53
KEY_SHIFT = &FF			; 54
KEY_DELETE = &A6		; 55
KEY_COPY = &96			; 56
KEY_RETURN = &B6		; 57
KEY_UP = &C6			; 58
KEY_DOWN = &D6			; 59
KEY_LEFT = &E6			; 60
KEY_RIGHT = &86			; 61
KEY_F0 = &DF			; 62
KEY_F1 = &8E			; 63
KEY_F2 = &8D			; 64
KEY_F3 = &8C			; 65
KEY_F4 = &EB			; 66
KEY_F5 = &8B			; 67
KEY_F6 = &8A			; 68
KEY_F7 = &E9			; 69
KEY_F8 = &89			; 70
KEY_F9 = &88			; 71

; Sheila Addresses

SHEILA_VIDEO = &FE00
SHEILA_SERIALCTRL = &FE08
SHEILA_SERIALULA = &FE10
SHEILA_VIDEOULA = &FE20
SHEILA_ROMSELECT = &FE30
SHEILA_SYSTEMVIA = &FE40
SHEILA_USERVIA = &FE60
SHEILA_FDC = &FE80
SHEILA_ECONET = &FEA0
SHEILA_ADC = &FEC0
SHEILA_TUBE = &FEE0
TUBE_SHEILA = &FEF8

; Read key direct
MACRO ENABLEKEY
	lda #&7f
	sta &FE43
	lda #&0f
	sta &FE42
	lda #&03
	sta &FE40
ENDMACRO

MACRO DISABLEKEY
	lda #&0b
	sta &FE40
ENDMACRO

MACRO READKEY scancode
	lda #255-scancode
	sta &FE4F
	lda &FE4F
	asl a
ENDMACRO

TUBE_ZP_START = &0
TUBE_ZP_END = &EF

HOST_ZP_START = &50
HOST_ZP_END = &a0

; Host side addresses for tube registers
HOST_TUBE_R1STAT = SHEILA_TUBE
HOST_TUBE_R1DATA = SHEILA_TUBE+1
HOST_TUBE_R2STAT = SHEILA_TUBE+2
HOST_TUBE_R2DATA = SHEILA_TUBE+3
HOST_TUBE_R3STAT = SHEILA_TUBE+4
HOST_TUBE_R3DATA = SHEILA_TUBE+5
HOST_TUBE_R4STAT = SHEILA_TUBE+6
HOST_TUBE_R4DATA = SHEILA_TUBE+7

; Tube side addresses for tube registers
TUBE_TUBE_R1STAT = TUBE_SHEILA
TUBE_TUBE_R1DATA = TUBE_SHEILA+1
TUBE_TUBE_R2STAT = TUBE_SHEILA+2
TUBE_TUBE_R2DATA = TUBE_SHEILA+3
TUBE_TUBE_R3STAT = TUBE_SHEILA+4
TUBE_TUBE_R3DATA = TUBE_SHEILA+5
TUBE_TUBE_R4STAT = TUBE_SHEILA+6
TUBE_TUBE_R4DATA = TUBE_SHEILA+7

;Tube Status flags
TUBE_DATA_PRESENT = &80
TUBE_NOT_FILLED = &40
TUBE_NOT_RESET = &20
TUBE_WORD_FIFO = &10
TUBE_PARA_NMI3 = &08
TUBE_PARA_IRQ4 = &04
TUBE_PARA_IRQ1 = &02
TUBE_HOST_IRQ1 = &01
