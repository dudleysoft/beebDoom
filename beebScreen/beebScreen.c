// beebScreen.c
// 
#include "beebScreen.h"

#include "beebCode.c"

#include <stdlib.h>
#include <string.h>

const unsigned char *WRCHV = (unsigned char *)0x20E;

int bsScreenWidth;
int bsScreenHeight;
int bsColours;
int bsScreenBase[2];
int bsMode;
int bsDoubleBuffer;
int bsNula;
int bsMouse;
int bsShowPointer;
int bsMouseX;
int bsMouseY;
int bsMouseB;

unsigned char *bsBuffer;
int bsBufferW;
int bsBufferH;
int bsBufferFormat;
int bsCurrentFrame;
int bsBufferSize;
int bsHostLomem;
unsigned char bsFrameCounter;
unsigned char *backBuffer[2];
void (*bsCallback)(void);

// Load into UDG memory at &C00, as long as we don't use characters 224-255 we'll be fine
unsigned char *beebCodeBase=(unsigned char*)0xc00;
#define OLD_WRCHV (4)
#define USER1V (7)
#define USER2V (10)
#define VSYNCV (13)
#define TIMERV (16)

void beebScreen_extractRGB444(int v,int *r,int *g,int *b)
{
    *r = v &0x00f;
    *g = (v &0x0f0) >> 4;
    *b = (v & 0xf00) >> 8;
}

void beebScreen_extractRGB555(int v,int *r,int *g,int *b)
{
    *r = (v >> 1) & 0x0f;
    *g = (v >> 6) & 0x0f;
    *b = (v >> 11) & 0x0f;
}

void beebScreen_extractRGB565(int v,int *r,int *g,int *b)
{
    *r = (v >> 1) & 0x0f;
    *g = (v >> 7) & 0x0f;
    *b = (v >> 12) & 0x0f;
}

void beebScreen_extractRGB888(int v,int *r,int *g,int *b)
{
    *r = (v >> 4) & 0x0f;
    *g = (v >> 12) & 0x0f;
    *b = (v >> 20) & 0x0f;
}

void beebScreen_extractBGR444(int v,int *r,int *g,int *b)
{
    *b = v &0x00f;
    *g = (v &0x0f0) >> 4;
    *r = (v & 0xf00) >> 8;
}

void beebScreen_extractBGR555(int v,int *r,int *g,int *b)
{
    *b = (v >> 1) & 0x0f;
    *g = (v >> 6) & 0x0f;
    *r = (v >> 11) & 0x0f;
}

void beebScreen_extractBGR565(int v,int *r,int *g,int *b)
{
    *b = (v >> 1) & 0x0f;
    *g = (v >> 7) & 0x0f;
    *r = (v >> 12) & 0x0f;
}

void beebScreen_extractBGR888(int v,int *r,int *g,int *b)
{
    *b = (v >> 4) & 0x0f;
    *g = (v >> 12) & 0x0f;
    *r = (v >> 20) & 0x0f;
}


#define TRUE (1)
#define FALSE (0)

typedef int BOOL;

unsigned char bsRemap[256];
unsigned char *bsNulaRemap = NULL;

#define GET_R(v) ((v) & 0x000f)
#define GET_G(v) (((v)>> 12) & 0x000f)
#define GET_B(v) (((v)>> 8) & 0x000f)

unsigned char beebScreen_FindPalette(int colour, int *remap,int total)
{
    int sr = GET_R(colour);
    int sg = GET_G(colour);
    int sb = GET_B(colour);

    int dist=10000;
    int idx = -1;

    for(int i = 0; i < total; ++i)
    {
        int rr = GET_R(remap[i]);
        int dr = rr - sr;
        int rg = GET_G(remap[i]);
        int dg = rg - sg;
        int rb = GET_B(remap[i]);
        int db = rb - sb;

        // Weighted distance, 2*red, 3*green, 1*blue
        int newDist = (2 * dr * dr) + (3 * dg * dg) + (1 * db * db);
        
        // If it's the lowest distance
        if (newDist < dist)
        {
            dist = newDist;
            idx = i;
        }
    }

    return idx;
}
/**
 * Creates Remapping between the our palette and the colours used in the screen
 * @param source - source palette we're matching colours from
 * @param remap - palette we're remapping the colours to
 * @param total - total number of colours to map to
 * @param len - total number of colours in the original palette
 */
void beebScreen_CreateRemapColours(int *source, int *remap, int total, int len)
{
	for(int col = 0; col < len; ++col)
	{

        bsRemap[col]=beebScreen_FindPalette(source[col],remap,total);
	}
}

void beebScreen_SetDefaultNulaRemapColours()
{
    bsNulaRemap = bsRemap;
}

void beebScreen_SetNulaRemapColours(unsigned char *remap)
{
    bsNulaRemap = remap;
}

int beebScreen_MakeNulaPal(int value,int index,void (*extractor)(int v,int *r, int *g, int *b))
{
    int r,g,b;

    extractor(value,&r,&g,&b);
    return ((index & 0x0f) << 4) + r + (g << 12) + (b << 8);
}

void beebScreen_SetNulaPal(int *values,int *output,int count, void (*extractor)(int v,int *r,int *g,int *b))
{
    for(int index = 0;index < count; ++index)
    {
        output[index]=beebScreen_MakeNulaPal(values[index],index,extractor);
    }
}

int bsRemapBeebPalette[30]={
  MAPRGB(0,0,0),MAPRGB(15,0,0),MAPRGB(0,15,0),MAPRGB(15,15,0),MAPRGB(0,0,15),MAPRGB(15,0,15),MAPRGB(0,15,15),MAPRGB(15,15,15),
  MAPRGB(0,0,0),MAPRGB(8,0,0),MAPRGB(0,8,0),MAPRGB(8,8,0),MAPRGB(0,0,8),MAPRGB(8,0,8),MAPRGB(0,8,8),MAPRGB(8,8,8),
  MAPRGB(8,8,8),MAPRGB(15,8,8),MAPRGB(8,15,8),MAPRGB(15,15,8),MAPRGB(8,8,15),MAPRGB(15,8,15),MAPRGB(8,15,15),MAPRGB(15,15,15),
  // R+Y         R+M            G+Y            G+C            B+M            B+C
  MAPRGB(15,8,0),MAPRGB(15,0,8),MAPRGB(8,15,0),MAPRGB(0,15,8),MAPRGB(8,0,15),MAPRGB(0,8,15)
}; // I know the beeb doesn't have half bright, but this is the only way to make this work we dither with black for the half bright

void beebScreen_SendPal(int *pal,int count)
{
    if (bsNula)
    {
        _VDU(BS_CMD_SEND_PAL);
        _VDU(count);
        for(int i=0;i<count;++i)
        {
            _VDU(pal[i]&0xff);
            _VDU(pal[i]>>8);
        }
    }
    else
    {
        switch(bsMode)
        {
        case 2:
            beebScreen_CreateRemapColours(pal, bsRemapBeebPalette, 30, count);
            break;
        default:
            // TODO - Remap to reduced colour palette - needs work since we need to define the colour palette
            for(int i=0; i < count; ++i)
            {
                bsRemap[i]=i;
            }
            break;
        }
    } 
}

void sendCrtc(int reg,int value)
{
    _VDU(BS_CMD_SEND_CRTC);
    _VDU(reg);
    _VDU(value);
}

void sendScreenbase(int addr)
{
    sendCrtc(13,(addr>>3) &0x0ff);
    sendCrtc(12,addr>>11);
}

void beebScreen_Init(int mode, int flags)
{
    unsigned char *beebCheck[256];
    // Copy our assembler code to the host
    memcpytoio_slow((void*)beebCodeBase,beebCode_bin,beebCode_bin_len);

    // Copy old WRCHV value into our code
    int wrchv = ReadByteFromIo((void*)WRCHV) + (ReadByteFromIo((void*)&WRCHV[1])<<8);

    WriteByteToIo((void*)&beebCodeBase[4],wrchv & 0xff);
    WriteByteToIo((void*)&beebCodeBase[5],wrchv >> 8);

    // Point the WRCHV to our code
    WriteByteToIo((void*)WRCHV,((int)beebCodeBase)&0xff);
    WriteByteToIo((void*)&WRCHV[1],((int)beebCodeBase)>>8);

    // // Setup video mode
    _VDU(22);
    _VDU(mode);
    // // Turn off cursor
    sendCrtc(10,32);
    // _VDU(23);_VDU(0);_VDU(11);_VDU(32);_VDU(0);_VDU(0);_VDU(0);_VDU(0);_VDU(0);_VDU(0);

    // Turn off cursor editing
    _swi(OS_Byte,_INR(0,1),4,1);
    // Break clears memory and escape disabled
    _swi(OS_Byte,_INR(0,1),200,3);
    // Set ESCAPE to generate the key value
    _swi(OS_Byte,_INR(0,1),229,1);

    bsMode=mode;
    bsNula = flags & BS_INIT_NULA;
    bsDoubleBuffer = flags & BS_INIT_DOUBLE_BUFFER;
    bsMouse = flags & BS_INIT_MOUSE;
    bsShowPointer = 0;
    bsCurrentFrame = 0;
    bsFrameCounter = 0;
    bsCallback = NULL;
    bsHostLomem = (flags & BS_INIT_ADFS ? 0x1600 : 0x1100);
    int bufferSize;
    
    switch(mode)
    {
    case 0:
        bsScreenWidth=640;
        bsScreenHeight=256;
        bsColours=2;
        bsScreenBase[0]=bsScreenBase[1]=0x3000;
        bufferSize=0x5000;
        break;
    case 1:
        bsScreenWidth=320;
        bsScreenHeight=256;
        bsColours=4;
        bsScreenBase[0]=bsScreenBase[1]=0x3000;
        bufferSize=0x5000;
        break;
    case 2:
        bsScreenWidth=160;
        bsScreenHeight=256;
        bsColours=16;
        bsScreenBase[0]=bsScreenBase[1]=0x3000;
        bufferSize=0x5000;
        break;
    case 3:
        bsScreenWidth=640;
        bsScreenHeight=200;
        bsColours=2;
        bsScreenBase[0]=bsScreenBase[1]=0x4000;
        bufferSize=0x4000;
        break;
    case 4:
        bsScreenWidth=320;
        bsScreenHeight=256;
        bsColours=2;
        bsScreenBase[0]=0x5800;
        bsScreenBase[1]=0x3000;
        bufferSize=0x2800;
        break;
    case 5:
        bsScreenWidth=160;
        bsScreenHeight=256;
        bsColours=4;
        bsScreenBase[0]=0x5800;
        bsScreenBase[1]=0x3000;
        bufferSize=0x2800;
        break;
    case 6:
        bsScreenWidth=320;
        bsScreenHeight=200;
        bsColours=2;
        bsScreenBase[0]=0x6000;
        bsScreenBase[1]=0x4000;
        bufferSize=0x2000;
        break;
    }

    bsBuffer = NULL;

    backBuffer[0]=malloc(bufferSize);
    memset(backBuffer[0],0,bufferSize);
    if (bsDoubleBuffer)
    {
        backBuffer[1]=malloc(bufferSize);
        memset(backBuffer[1],0,bufferSize);
    }
    bsBufferSize = bufferSize;
    
    sendScreenbase(bsScreenBase[0]);

    if (bsNula)
    {
        _swi(OS_Byte,_INR(0,2),151,34,0x80);
        _swi(OS_Byte,_INR(0,2),151,34,0x90);
    }

}

void beebScreen_InjectCode(unsigned char *code, int length,int dest)
{
    memcpytoio_slow((void*)dest,code,length);
}

void beebScreen_SetUserVector(int vector,int addr)
{
    int low = addr & 0xff;
    int high = addr >> 8;
    // printf("Set vector %d: %04x\n",vector,addr);
    switch(vector)
    {
    case BS_VECTOR_USER1:
        WriteByteToIo((void*)&beebCodeBase[USER1V],low);
        WriteByteToIo((void*)&beebCodeBase[USER1V+1],high);
        break;
    case BS_VECTOR_USER2:
        WriteByteToIo((void*)&beebCodeBase[USER2V],low);
        WriteByteToIo((void*)&beebCodeBase[USER2V+1],high);
        break;
    case BS_VECTOR_VSYNC:
        WriteByteToIo((void*)&beebCodeBase[VSYNCV],low);
        WriteByteToIo((void*)&beebCodeBase[VSYNCV+1],high);
        break;
    case BS_VECTOR_TIMER:
        WriteByteToIo((void*)&beebCodeBase[TIMERV],low);
        WriteByteToIo((void*)&beebCodeBase[TIMERV+1],high);
        break;
    }
}

void beebScreen_SetGeometry(int w,int h,int setCrtc)
{
    bsScreenWidth = w;
    bsScreenHeight = h;
    
    int crtW = w;

    switch(bsColours)
    {
    case 2:
        crtW >>=3;
        break;
    case 4:
        crtW >>=2;
        break;
    case 16:
        crtW >>=1;
        break;
    }
    if (!setCrtc)
        return;
    if (bsMode <4)
    {
        sendCrtc(1,crtW);
        int pos=18+crtW+((80-crtW)/2);
        sendCrtc(2,pos);
    }
    else
    {
        sendCrtc(1,crtW);
        int pos=9+crtW+((40-crtW)/2);
        sendCrtc(2,pos);
    }
    int crtH = h>>3;    
    sendCrtc(6,crtH);
    int hpos=34 - ((32-crtH)/2);
    sendCrtc(7,hpos);
}

void beebScreen_SetScreenBase(int address,int secondBuffer)
{
    int buffer = secondBuffer ? 1 : 0;
    bsScreenBase[buffer] = address;
    if (bsCurrentFrame == buffer)
    {
        sendScreenbase(address);
    }
}

int getScreenSize();

int beebScreen_CalcScreenBase(int secondBuffer)
{
    int size = getScreenSize();
    int base = 0x8000 - (size * (secondBuffer ? 2: 1));

    if (base < bsHostLomem)
    {
        base = 0x8000 - size;
    }
    return base;
}

void beebScreen_UseDefaultScreenBases()
{
    beebScreen_SetScreenBase(beebScreen_CalcScreenBase(0),0);
    if (bsDoubleBuffer)
    {
        beebScreen_SetScreenBase(beebScreen_CalcScreenBase(1),1);
    }
}

void beebScreen_SetBuffer(unsigned char *buffer, int format,int w,int h)
{
    bsBuffer = buffer;
    bsBufferFormat = format;
    bsBufferW = w;
    bsBufferH = h;
}

void beebScreen_FlipCallback(void (*callback)(void))
{
    bsCallback = callback;
}

unsigned char beebBuffer[20480];

void convert2Col(unsigned char *map)
{

}

void convert2Dither(unsigned char *map)
{

}

const unsigned char mode1Mask[] = {
    0x00,
    0x01,
    0x10,
    0x11
};

const unsigned char mode1Dither1[] = {
    0x00,
    0x01,
    0x10,
    0x11
};

const unsigned char mode1Dither2[] = {
    0x00,
    0x01,
    0x10,
    0x11
};

void convert4Col(unsigned char *map)
{
	unsigned char *src;
	unsigned char *dest;
	int x;
    int y=0;

	src = &bsBuffer[y * bsBufferW];

    int w = bsScreenWidth * 2;
    int charW = bsScreenWidth >> 2;
    int Xstep = bsBufferW / bsScreenWidth;
    int Ystep = bsBufferH / bsScreenHeight;

	do
	{
        int addr = ((y>>3) * w) + (y & 7);
        //printf("addr: %04x\n",addr);
    	dest = &beebBuffer[addr];
		for(x=0; x< charW; x++)
		{
			int pix1 = map ? map[*src] : *src;
			src+=Xstep;
			int pix2 = map ? map[*src] : *src;
			src+=Xstep;
            int pix3 = map ? map[*src] : *src;
            src+=Xstep;
            int pix4 = map ? map[*src] : *src;
            src+=Xstep;

			*dest = (mode1Mask[pix1]<<3) +(mode1Mask[pix2]<<2) + (mode1Mask[pix3]<<1) + mode1Mask[pix4];
			dest+=8;
		}
		y++;

	} while (y < bsScreenHeight);
}

void convert4Dither(unsigned char *map)
{
	unsigned char *src;
	unsigned char *dest;
	int x;
    int y=0;

	src = &bsBuffer[y * bsBufferW];

    int w = bsScreenWidth * 2;
    int charW = bsScreenWidth >> 2;
    int Xstep = bsBufferW / bsScreenWidth;
    int Ystep = bsBufferH / bsScreenHeight;

	do
	{
        int addr = ((y>>3) * w) + (y & 7);
        //printf("addr: %04x\n",addr);
    	dest = &beebBuffer[addr];
		for(x=0; x< charW; x++)
		{
			int pix1 = map ? map[*src] : *src;
			src+=Xstep;
			int pix2 = map ? map[*src] : *src;
			src+=Xstep;
			int pix3 = map ? map[*src] : *src;
			src+=Xstep;
			int pix4 = map ? map[*src] : *src;
			src+=Xstep;
			*dest = (y & 1) ? ((mode1Dither2[pix1]<<3) + (mode1Dither1[pix2]<<2) + (mode1Dither2[pix3]<<1) + mode1Dither1[pix4])
            : ((mode1Dither1[pix1]<<3) + (mode1Dither2[pix2]<<2) + (mode1Dither1[pix3]<<1) + mode1Dither2[pix4]);
			dest+=8;
		}
		y++;

	} while (y < bsScreenHeight);
}

const unsigned char mode2Mask[] = {
	0x00,
	0x01,
	0x04,
	0x05,

	0x10,
	0x11,
	0x14,
	0x15,

	0x40,
	0x41,
	0x44,
	0x45,
	0x50,
	0x51,
	0x54,
	0x55
};

const unsigned char mode2Dither1[] = {
	0x00,
	0x01,
	0x04,
	0x05,
	0x10,
	0x11,
	0x14,
	0x15,

	0x00,
	0x01,
	0x04,
	0x05,
	0x10,
	0x11,
	0x14,
	0x15,

	0x15,
	0x15,
	0x15,
	0x15, 
	0x15,
	0x15,
	0x15,
	0x15,

    0x01,
    0x01,
    0x04,
    0x04,
    0x10,
    0x10
};

const unsigned char mode2Dither2[] = {
	0x00,
	0x01,
	0x04,
	0x05,
	0x10,
	0x11,
	0x14,
	0x15,

	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,

	0x00,
	0x01,
	0x04,
	0x05,
	0x10,
	0x11,
	0x14,
	0x15,
  
    0x05,
    0x11,
    0x05,
    0x14,
    0x11,
    0x14
};

void convert16Col(unsigned char *map)
{
	unsigned char *src;
	unsigned char *dest;
	int x;
    int yPos=0;

    int w = bsScreenWidth * 4;
    int charW = bsScreenWidth >> 1;
    int Xstep = (bsBufferW << 8) / bsScreenWidth;
    int Ystep = (bsBufferH << 8) / bsScreenHeight;
    int line = 0;

	do
	{
        int y = yPos>>8;
    	src = &bsBuffer[y * bsBufferW];
        int xPos = 0;
        int addr = ((line>>3) * w) + (line & 7);
        //printf("addr: %04x\n",addr);
    	dest = &beebBuffer[addr];
		for(x=0; x< charW; x++)
		{
			int pix1 = map ? map[src[xPos>>8]] : src[xPos>>8];
			xPos+=Xstep;
			int pix2 = map ? map[src[xPos>>8]] : src[xPos>>8];
			xPos+=Xstep;
			*dest = (mode2Mask[pix1]<<1) + mode2Mask[pix2];
			dest+=8;
		}
		yPos+=Ystep;
        line++;

	} while (line < bsScreenHeight);
}

void convert16Dither(unsigned char *map)
{
	unsigned char *src;
	unsigned char *dest;
	int x;
    int yPos=0;


    int w = bsScreenWidth * 4;
    int charW = bsScreenWidth >> 1;
    int Xstep = (bsBufferW << 8) / bsScreenWidth;
    int Ystep = (bsBufferH << 8) / bsScreenHeight;
    int line = 0;

	do
	{
        int y = yPos>>8;
    	src = &bsBuffer[y * bsBufferW];
        int xPos = 0;
        int addr = ((line>>3) * w) + (line & 7);
        //printf("addr: %04x\n",addr);
    	dest = &beebBuffer[addr];
		for(x=0; x< charW; x++)
		{
			int pix1 = map ? map[src[xPos>>8]] : src[xPos>>8];
			xPos+=Xstep;
			int pix2 = map ? map[src[xPos>>8]] : src[xPos>>8];
			xPos+=Xstep;
			*dest = (line & 1) ? ((mode2Dither2[pix1]<<1) + mode2Dither1[pix2]) : ((mode2Dither1[pix1]<<1) + mode2Dither2[pix2]);
			dest+=8;
		}
		yPos+=Ystep;
        line++;

	} while (line < bsScreenHeight);
}

int getScreenSize()
{
    switch(bsColours)
    {
    case 2:
        return (bsScreenWidth * bsScreenHeight) >> 3;
    case 4:
        return (bsScreenWidth * bsScreenHeight) >> 2;
    case 16:
        return (bsScreenWidth * bsScreenHeight) >> 1;
    }
    return bsScreenWidth * bsScreenHeight >> 1;
}

void setBeebPixel0(unsigned char *buffer, int x,int y, int c)
{
    if (c & 1)
        buffer[((y>>3)*bsScreenWidth)+(y&7) +(x&0xff8)] |= 1<<(7-(x&7));
    else
        buffer[((y>>3)*bsScreenWidth)+(y&7) +(x&0xff8)] &= (1<<(7-(x&7)))^255;
}

void setBeebPixel1(unsigned char *buffer, int x,int y,int c)
{
    int pixel = mode1Mask[c] << (3-(x&3));
    int mask = (mode1Mask[3] << (3-(x&3))) ^ 255;
    buffer[((y>>3)*bsScreenWidth*2) + (y & 7) + ((x<<1)&0xff8)] = (buffer[((y>>3)*bsScreenWidth*2) + (y & 7) + ((x<<1)&0xff8)] & mask) + pixel;
}

void setBeebPixel2(unsigned char *buffer, int x,int y,int c)
{
    int pixel = mode2Mask[c] << (1-(x&1));
    int mask = (mode2Mask[15] << (1-(x&1))) ^ 255;
    buffer[((y>>3)*bsScreenWidth*4) + (y & 7) + ((x<<2)&0xff8)] = (buffer[((y>>3)*bsScreenWidth*4) + (y & 7) + ((x<<2)&0xff8)] & mask) + pixel;
}

unsigned char ptrData[] = {
    1,1,0,0,0,0,0,0,
    1,2,1,0,0,0,0,0,
    1,2,2,1,0,0,0,0,
    1,2,2,2,1,0,0,0,
    1,2,2,2,2,1,0,0,
    1,2,2,2,1,0,0,0,
    1,2,1,2,1,0,0,0,
    1,1,0,1,2,1,0,0,
    0,0,0,1,2,1,0,0,
    0,0,0,0,1,2,1,0,
    0,0,0,0,1,2,1,0,
    0,0,0,0,0,1,0,0
};

void addMouseCursor(unsigned char *beebBuffer)
{
    int Xstep = bsBufferW / bsScreenWidth;
    int Ystep = bsBufferH / bsScreenHeight;
    int mx = bsMouseX / Xstep;
    int my = bsMouseY / Ystep;
    void (*ptrFunc)(unsigned char *,int,int,int);
    int ptrColour[]={0, bsNula ? 15 : 7, 0};
    switch(bsMode)
    {
    case 0:
    case 3:
    case 4:
    case 6:
        ptrFunc = setBeebPixel0;
        break;

    case 1:
    case 5:
        ptrFunc = setBeebPixel1;
        break;

    case 2:
        ptrFunc = setBeebPixel2;
        break;
    }

    for(int y = my; y< bsScreenHeight & y < my+12; ++y)
    {
        unsigned char *ptr=&ptrData[(y-my)*8];

        for(int x = mx; x < bsScreenWidth & x < mx+8; ++x)
        {
            int c=*ptr++;
            if (c)
            {
                ptrFunc(beebBuffer,x,y,ptrColour[c]);
            }
        }
    }
}

unsigned char compBuffer[32768];
int compBuffPtr=0;
int outBuffPtr=0;
#define WRITE_BUF(v) compBuffer[compBuffPtr++]=v

void beebScreen_CompressAndCopy(unsigned char *new,unsigned char *old)
{
  unsigned char *p1 = new,*p2 = old;
  int addr = 0;
  int base = bsDoubleBuffer ? bsScreenBase[1-bsCurrentFrame] : bsScreenBase[0];
  int length = getScreenSize();
  compBuffPtr = 0;

  while(addr < length && *p1==*p2)
  {
    p1++;
    p2++;
    addr++;
  }
  // Send start address
  WRITE_BUF(BS_CMD_SEND_SCREEN);
  WRITE_BUF((addr+base) & 0xff);
  WRITE_BUF((addr+base) >> 8);

  int start = addr;
  while(addr < length)
  {
    int count = 0;
    if (*p1 != *p2)
    {
      while(addr < length && count < 127 && p1[count] != p2[count])
      {
        count++;
        addr++;
      }
      WRITE_BUF(count);
      for(int loop=count;loop>0;--loop)
      {
        WRITE_BUF(p1[loop-1]);
        p2++;
      }
      p1+=count;
    }
    else
    {
      while(addr < length && *p1 == *p2)
      {
        count++;
        addr++;
        p1++;
        p2++;
      }
      if (addr < length)
      {
        if (count > 127)
        {
            WRITE_BUF(128);
            WRITE_BUF((addr+base) & 0xff);
            WRITE_BUF((addr+base) >> 8);
        }
        else
        {
            WRITE_BUF(128+count);
        }
      }
    }
  }

  WRITE_BUF(0);
  outBuffPtr = 0;

  // Write the contents of the buffer
  while(outBuffPtr < compBuffPtr)
  {
      _VDU(compBuffer[outBuffPtr++]);
  }
}

void updateMouse()
{
    int x,y;

    _swi(OS_Byte,_INR(0,1)|_OUTR(1,2),128,7,&x,&y);
    bsMouseX = (x + (y<<8)) * bsBufferW / 1280;
    _swi(OS_Byte,_INR(0,1)|_OUTR(1,2),128,8,&x,&y);
    bsMouseY = bsBufferH - ((x + (y<<8)) * bsBufferH / 1024);
    _swi(OS_Byte,_INR(0,1)|_OUT(1),128,9,&bsMouseB);
}

void beebScreen_Flip()
{  
    // TODO - Add flip screen code
    switch(bsMode)
    {
        case 0:
        case 4:
        case 3:
        case 6:
            if (bsNula)
                convert2Col(bsNulaRemap);
            else
                convert2Dither(bsRemap);
            break;
        case 1:
        case 5:
            if (bsNula)
                convert4Col(bsNulaRemap);
            else
                convert4Dither(bsRemap);
            break;
        case 2:
            if (bsNula)
                convert16Col(bsNulaRemap);
            else
                convert16Dither(bsRemap);
            break;
    }
    if (bsMouse)
    {
        updateMouse();
        if (bsShowPointer)
        {
            addMouseCursor(beebBuffer);
        }
    }
    beebScreen_CompressAndCopy(beebBuffer,backBuffer[bsCurrentFrame]);
    memcpy(backBuffer[bsCurrentFrame],beebBuffer,bsBufferSize);

    if (bsCallback)
    {
        bsCallback();
    }
    else
    {
        // If we've not got a callback then wait for the next VSync
        beebScreen_VSync();
    }

    // Swap buffers if we're double buffering
    if (bsDoubleBuffer)
    {
        bsCurrentFrame=1-bsCurrentFrame;
        sendScreenbase(bsScreenBase[bsCurrentFrame]);
    }

    // Read frame counter from the beeb
    bsFrameCounter = ReadByteFromIo((void*)0x6d);
}

void beebScreen_VSync()
{
    _swi(OS_Byte,_IN(0),19);
}

void beebScreen_Quit()
{
    _VDU(BS_CMD_SEND_QUIT);
    
    // Reset NULA palette
    if (bsNula)
    {
        _swi(OS_Byte,_INR(0,2),151,34,64);
    }
    // Enable cursor editing
    _swi(OS_Byte,_INR(0,1),4,0);
    // Clear buffers
    _swi(OS_Byte,_INR(0,1),15,0);
}

void beebScreen_GetMouse(int *mx,int *my,int *mb)
{
    if (bsMouse)
    {
        *mx = bsMouseX;
        *my = bsMouseY;
        *mb = bsMouseB;
    }
    else
    {
        *mx = bsBufferW >> 1;
        *my = bsBufferH >> 1;
        *mb = 0;
    }
}

void beebScreen_ShowPointer(int show)
{
    bsShowPointer = show;
}

unsigned char beebScreen_GetFrameCounter()
{
    return bsFrameCounter;
}

int beebScreen_ScanKey(int key)
{
    int x,y;

    _swi(OS_Byte,_INR(0,2)|_OUTR(1,2),129,0xff-key,0xff,&x,&y);
    // Fix for a difference between PiTubeDirect and ARM7TDMI, PiTube returns X + (Y<<8) in R1, ARM7TDMI return X
    // This makes the two values compatible
    x |= (y<<8);

    // Checks if we've returned 0xFF in both
    return (x == 0xffff) ? 1 : 0;
}