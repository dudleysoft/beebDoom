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
int bsRgb2Hdmi;
int bsPiVdu;
int bsMouse;
int bsShowPointer;
int bsMouseX;
int bsMouseY;
int bsMouseB;
int bsMouseColour;

unsigned char *bsBuffer;
int bsBufferW;
int bsBufferH;
int bsBufferFormat;
int bsCurrentFrame;
int bsBufferSize;
int bsHostLomem;
unsigned char bsFrameCounter;
unsigned char backBuffer[2][20480];
unsigned char *piVduBuffer;
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

// Remap palette for mode 0 remapping to grey scale values
int bsRemapMode0[16]={0x0000,0x1101,0x2202,0x3303,0x4404,0x5505,0x6606,0x7707,0x8808,0x9909,0xaa0a,0xbb0b,0xcc0c,0xdd0d,0xee0e,0xff0f};

int bsHdmiPal[16];
int bsHdmiCols;

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
    else if (bsRgb2Hdmi)
    {
        bsHdmiCols = count > 16 ? 16 : count;
        for(int i=0;i<bsHdmiCols;++i)
        {
            bsHdmiPal[i]=pal[i];
        }
    }
    else if (bsPiVdu)
    {
        for(int i=0;i<count;++i)
        {
            _VDU(19);
            _VDU(i);
            _VDU(16);
            _VDU((pal[i]&0x0f)<<4);
            _VDU((pal[i]>>8)&0xf0);
            _VDU((pal[i]&0x0f00)>>4);
        }
    }
    else
    {
        switch(bsMode)
        {
            // 2 colour modes, we'll map to a greyscale dither pattern
        case 0:
        case 3:
        case 4:
        case 6:
            beebScreen_CreateRemapColours(pal, bsRemapMode0, 16, count);
            break;

            // Mode 2 uses a 2/2 dither pattern with 30 entries
        case 2:
            beebScreen_CreateRemapColours(pal, bsRemapBeebPalette, 30, count);
            break;

            // Other modes are more complex since we cant guarantee which colours will be available on
            // non-NULA builds, we can choose 4 from 8, so we're limited.
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

int beebScreen_CreatePalMap(int *pal,int count,unsigned char *map)
{
	int used=count;
	map[0] = 0;
	for(int i = 1; i < count; i++)
	{
		map[i] = i;
		for(int j = 0; j < i; j++)
		{
			if ((pal[j] & 0xff0f) == (pal[i] & 0xff0f))
			{
				map[j] = i;
				j = i;
				used--;
			}
		}
	}
	return used;
}

int lastPalIndices[] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};

void beebScreen_CreateDynamicPalette(int* inPal,unsigned char *palMap,int colours,int *outPal,int target)
{
	int counts[colours];
	int most[target];

	// clear counts
	memset(counts, 0, colours * sizeof(int));

	// count unique palette colours, note that non-unique palette entries will
	// count all different values, luckily remap will remap them back to the first reference
	unsigned char *p=bsBuffer;
	for(int i = 0; i < bsBufferW * bsBufferH; ++i)
	{
		int col=palMap ? palMap[*p++] : *p++;
		counts[col]++;
	}

	int used[target];
	int allocated[target];

	// Count the most populous colours
	for(int i = 0; i < target; ++i)
	{
		int found = 0;
		int max = counts[0];

		for(int j = 1; j < colours; ++j)
		{
			if (counts[j] > max)
			{
				max = counts[j];
				found = j;
			}
		}
		most[i] = found;
		counts[found] = 0;
		used[i] = FALSE;
		allocated[i] = FALSE;
	}

	// Find palette entries already in use (try to speed up transfer)
	for(int i = 0; i < target; ++i)
	{
		for(int j = 0; j < target; ++j)
		{
			if (lastPalIndices[j] == most[i])
			{
				allocated[i] = TRUE;
				used[j] = TRUE;
				// Update palette (makes pal change work without distrupting the display)
				outPal[j] = (inPal[most[i]] & 0xff0f) | (j<<4);
				j=16;
			}
		}
	}

	// Find space for the remaining colours
	for(int i = 0; i < 16; ++i)
	{
		int j=0;
		if (!allocated[i])
		{
			// Find the first unused slot
			while((j < 16) && used[j]) j++;

			// Place the palette
			outPal[j] = (inPal[most[i]] & 0xff0f) | (j<<4);
			lastPalIndices[j] = most[i];
			used[j] = TRUE;
		}
	}

	beebScreen_CreateRemapColours(inPal,outPal,target,colours);
}

void sendCrtc(int reg,int value)
{
    if (!bsPiVdu)
    {
        _VDU(BS_CMD_SEND_CRTC);
        _VDU(reg);
        _VDU(value);
    }
}

void sendScreenbase(int addr)
{
    if (!bsPiVdu)
    {
        sendCrtc(13, (addr >> 3) &0x0ff);
        sendCrtc(12, addr >> 11);
    }
}

void beebScreen_SetMode(int mode)
{
    bsMode = mode;
    _VDU(22);
    if (bsRgb2Hdmi && mode == 2)
    {
        _VDU(0);
    }
    else
    {
        _VDU(mode);
    }

    int bufferSize;

    // // Setup video mode
    // _VDU(22);
    // _VDU(mode);
    // // Turn off cursor
    if (!bsPiVdu)
    {
        sendCrtc(10, 32);
    }
    else
    {
        _VDU(23);_VDU(1);_VDU(0);_VDU(0);_VDU(0);_VDU(0);_VDU(0);_VDU(0);_VDU(0);_VDU(0);
    }

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
    case 9:
        bsScreenWidth=320;
        bsScreenHeight=256;
        bsColours=16;
        bsScreenBase[0]=bsScreenBase[1]=(int)piVduBuffer;
        bufferSize = 320*256;
        break;
    case 13:
        bsScreenWidth=320;
        bsScreenHeight=256;
        bsColours=256;
        bsScreenBase[0]=bsScreenBase[1]=(int)piVduBuffer;
        bufferSize = 320*256;
        break;
    }

    bsBuffer = NULL;

    if (!bsPiVdu)
    {
        memset(backBuffer[0],0,bufferSize);

        if (bsDoubleBuffer)
        {
            memset(backBuffer[1],0,bufferSize);
        }
        sendScreenbase(bsScreenBase[0]);
    }

    bsBufferSize = bufferSize;
}

void beebScreen_Init(int mode, int flags)
{
    unsigned char *beebCheck[256];

    bsNula = flags & BS_INIT_NULA;
    bsRgb2Hdmi = flags & BS_INIT_RGB2HDMI;
    bsPiVdu = flags & BS_INIT_PIVDU;
    bsDoubleBuffer = flags & BS_INIT_DOUBLE_BUFFER;
    bsMouse = flags & BS_INIT_MOUSE;

    if (!bsPiVdu)
    {
        // Copy our assembler code to the host
        memcpytoio_slow((void*)beebCodeBase,beebCode_bin,beebCode_bin_len);

        int wrchv = ReadByteFromIo((void*)WRCHV) + (ReadByteFromIo((void*)&WRCHV[1])<<8);

        WriteByteToIo((void*)&beebCodeBase[4],wrchv & 0xff);
        WriteByteToIo((void*)&beebCodeBase[5],wrchv >> 8);

        // Point the WRCHV to our code
        WriteByteToIo((void*)WRCHV,((int)beebCodeBase)&0xff);
        WriteByteToIo((void*)&WRCHV[1],((int)beebCodeBase)>>8);
    }

    if (bsPiVdu)
    {
        // Dummy VDU to enable the code on the host
        // _VDU(0);
        _swi(OS_CLI,_IN(0),"PIVDU 2");
    }

    // Turn off cursor editing
    _swi(OS_Byte, _INR(0, 1), 4, 1);
    // Break clears memory and escape disabled
    _swi(OS_Byte, _INR(0, 1), 200, 3);
    // Set ESCAPE to generate the key value
    _swi(OS_Byte, _INR(0, 1), 229, 1);

    bsShowPointer = 0;
    bsCurrentFrame = 0;
    bsFrameCounter = 0;
    bsCallback = NULL;
    bsHostLomem = (flags & BS_INIT_ADFS ? 0x1600 : 0x1100);
    bsMouseColour = bsNula ? 15 : 7;

    // Use same routine we've made external
    beebScreen_SetMode(mode);


    if (bsNula)
    {
        _swi(OS_Byte, _INR(0, 2), 151, 34, 0x80);
        _swi(OS_Byte, _INR(0, 2), 151, 34, 0x90);
    }
    if (bsPiVdu)
    {
        int vars[2] = {148, -1};
        _swi(OS_ReadVduVariables,_INR(0,1), &vars, &piVduBuffer);
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
        if (bsPiVdu) WriteByteToIo((void*)&beebCodeBase[USER1V-1],0x20);
        WriteByteToIo((void*)&beebCodeBase[VSYNCV],low);
        WriteByteToIo((void*)&beebCodeBase[VSYNCV+1],high);
        break;
    case BS_VECTOR_TIMER:
        if (bsPiVdu) WriteByteToIo((void*)&beebCodeBase[USER1V-1],0x20);
        WriteByteToIo((void*)&beebCodeBase[TIMERV],low);
        WriteByteToIo((void*)&beebCodeBase[TIMERV+1],high);
        break;
    }
}

void beebScreen_SetGeometry(int w,int h,int setCrtc)
{
    bsScreenWidth = w;
    bsScreenHeight = h;
    
    if (bsPiVdu)
    {
        // Setup screen geometry
        _VDU(23);
        _VDU(22);
        _VDU(w%256);    // Width
        _VDU(w>>8);
        _VDU(h%256);    // Height
        _VDU(h>>8);     // Width in chars
        _VDU(w>>3);     // Height in chars
        _VDU(h>>3);
        _VDU(0);    // Colours (0 for 256)
        _VDU(0);    // flags (0 default)
        // Turn off cursor
        _VDU(23);
        _VDU(1);
        _VDU(0);
        _VDU(0);
        _VDU(0);
        _VDU(0);
        _VDU(0);
        _VDU(0);
        _VDU(0);
        _VDU(0);

        bsBufferSize = w*h;

        // Read buffer start address
        int vars[2] = {148, -1};
        _swi(OS_ReadVduVariables,_INR(0,1), &vars, &piVduBuffer);
    }
    else
    {
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

void beebScreen_ClearScreens(int screenMemory)
{
    int addr = bsDoubleBuffer ? bsScreenBase[1] : bsScreenBase[0];
    int end = screenMemory ? 0x8000 : (bsMode < 3 ? 0x3000: 0x5800);
    
    while(addr < end)
    {
        WriteByteToIo((void*)addr++,0);
    }
    memset((void*)backBuffer[0],0,bsBufferSize);
    if (bsDoubleBuffer)
    {        
        memset((void*)backBuffer[1],0,bsBufferSize);
    }
    // Reset last palette indices
    memset(lastPalIndices,-1,sizeof(lastPalIndices));
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

unsigned char dither2[16][4]={
    {0x00,0x00,0x00,0x00},  // 0  - &0000
    {0x11,0x00,0x00,0x00},  // 1  - &1101
    {0x11,0x00,0x44,0x00},  // 2  - &2202
    {0x11,0x00,0x44,0x88},  // 3  - &3303
    {0x11,0x22,0x44,0x88},  // 4  - &4404
    {0x55,0x22,0x44,0x88},  // 5  - &5505
    {0x55,0x22,0x55,0x88},  // 6  - &6606
    {0x55,0x22,0x55,0xaa},  // 7  - &7707
    {0x55,0xaa,0x55,0xaa},  // 8  - &8808
    {0x77,0xaa,0x55,0xaa},  // 9  - &9909
    {0x77,0xaa,0x77,0xaa},  // 10 - &aa0a
    {0x77,0xaa,0x77,0xee},  // 11 - &bb0b
    {0x77,0xee,0x77,0xee},  // 12 - &cc0c
    {0xff,0xee,0x77,0xee},  // 13 - &dd0d
    {0xff,0xee,0xff,0xff},  // 14 - &ee0e
    {0xff,0xff,0xff,0xff},  // 15 - &ff0f
};

void convert2Dither(unsigned char *map)
{
	unsigned char *src;
	unsigned char *dest;
	int x;
    int yPos=0;

    int w = bsScreenWidth;
    int charW = bsScreenWidth >> 3;
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
            int value = 0;
            for(int x2=0;x2<8;++x2)
            {
                int pix = map ? map[src[xPos>>8]] : src[xPos>>8];
                xPos+=Xstep;
                value |= dither2[pix & 15][line&3] & (1<<(7-x2));
            }

			*dest = value;
			dest+=8;
		}
		yPos+=Ystep;
        line++;
	} while (line < bsScreenHeight);
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
    int yPos=0;

    int w = bsScreenWidth * 2;
    int charW = bsScreenWidth >> 2;
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
			int pix3 = map ? map[src[xPos>>8]] : src[xPos>>8];
			xPos+=Xstep;
			int pix4 = map ? map[src[xPos>>8]] : src[xPos>>8];
			xPos+=Xstep;

			*dest = (mode1Mask[pix1]<<3) +(mode1Mask[pix2]<<2) + (mode1Mask[pix3]<<1) + mode1Mask[pix4];
			dest+=8;
		}
		yPos+=Ystep;
        line++;

	} while (line < bsScreenHeight);
}

void convert4Dither(unsigned char *map)
{
    unsigned char *src;
	unsigned char *dest;
	int x;
    int yPos=0;

    int w = bsScreenWidth * 2;
    int charW = bsScreenWidth >> 2;
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
			int pix3 = map ? map[src[xPos>>8]] : src[xPos>>8];
			xPos+=Xstep;
			int pix4 = map ? map[src[xPos>>8]] : src[xPos>>8];
			xPos+=Xstep;

			*dest = (y & 1) ? ((mode1Dither2[pix1]<<3) + (mode1Dither1[pix2]<<2) + (mode1Dither2[pix3]<<1) + mode1Dither1[pix4])
            : ((mode1Dither1[pix1]<<3) + (mode1Dither2[pix2]<<2) + (mode1Dither1[pix3]<<1) + mode1Dither2[pix4]);
			dest+=8;
		}
		yPos+=Ystep;
        line++;

	} while (line < bsScreenHeight);
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
	} 
    while (line < bsScreenHeight);
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
    int Xstep = 1;
    int Ystep = 1;
    int mx = (bsMouseX * bsScreenWidth / 1280);
    int my = bsScreenHeight - (bsMouseY * bsScreenHeight / 1024);
    void (*ptrFunc)(unsigned char *,int,int,int);
    int ptrColour[]={0,bsMouseColour,0};//{0, bsNula ? 15 : 7, 0};
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

#define ADD_BUFFER(value) *p=value; p+=8;

void addHdmiPal(unsigned char *beebBuffer)
{
    char *p=beebBuffer;
    ADD_BUFFER('H');
    ADD_BUFFER('D');
    ADD_BUFFER('M');
    ADD_BUFFER('I');
    ADD_BUFFER(2*bsHdmiCols);
    for(int i=0;i<bsHdmiCols;++i)
    {
        ADD_BUFFER(bsHdmiPal[i]&0xff);
        ADD_BUFFER(bsHdmiPal[i]>>8);
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
    bsMouseX = ((x & 0xff) + (y<<8));
    _swi(OS_Byte,_INR(0,1)|_OUTR(1,2),128,8,&x,&y);
    bsMouseY = ((x & 0xff) + (y<<8));
    _swi(OS_Byte,_INR(0,1)|_OUT(1),128,9,&bsMouseB);
    if (bsMouseX > 32768)
    {
        bsMouse = FALSE;
        bsMouseX = 640;
        bsMouseY = 512;
        bsMouseB = 0;
    }
}

void addPiCursor(unsigned char *buffer)
{
    int Xstep = 1;
    int Ystep = 1;
    int mx = (bsMouseX * bsScreenWidth / 1280);
    int my = bsScreenHeight - (bsMouseY * bsScreenHeight / 1024);

    int ptrColour[]={0,bsMouseColour,0};//{0, bsNula ? 15 : 7, 0};

    for(int y = my; y< bsScreenHeight & y < my+12; ++y)
    {
        unsigned char *ptr=&ptrData[(y-my)*8];

        for(int x = mx; x < bsScreenWidth & x < mx+8; ++x)
        {
            int c=*ptr++;
            if (c)
            {
                buffer[y*bsScreenWidth + x]=ptrColour[c];
            }
        }
    }    
}

void beebScreen_Flip()
{  
    if (bsPiVdu)
    {
        if (bsCallback)
        {
            bsCallback();
        }
        else
        {
            // If we've not got a callback then wait for the next VSync
            beebScreen_VSync();
        }
        static unsigned char c=0;
        // Copy to frame buffer
        // memset(piVduBuffer, c++, bsScreenWidth * bsScreenHeight);
        memcpy(piVduBuffer, bsBuffer, bsBufferSize);

        if (bsShowPointer)
        {
            addPiCursor(piVduBuffer);
        }
    }
    else
    {
        // TODO - Add flip screen code
        switch(bsMode)
        {
            case 0:
            case 4:
            case 3:
            case 6:
                // Only really makes sense to dither in mode 0,3,4 or 6
                convert2Dither(bsRemap);
                break;

            case 1:
            case 5:
                if (bsNula || bsRgb2Hdmi)
                    convert4Col(bsNulaRemap);
                else
                    convert4Dither(bsRemap);
                break;

            case 2:
                if (bsNula || bsRgb2Hdmi)
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
        if (bsRgb2Hdmi && bsMode == 2)
        {
            addHdmiPal(beebBuffer);
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
    }

    // Swap buffers if we're double buffering
    if (bsDoubleBuffer)
    {
        bsCurrentFrame=1-bsCurrentFrame;
        sendScreenbase(bsScreenBase[bsCurrentFrame]);
    }
}

void beebScreen_VSync()
{
    if (bsPiVdu)
    {
        int next = bsFrameCounter;
        while (next == bsFrameCounter) {
            int block[2];
            _swi(OS_Word,_INR(0,1),1,block);
            next = (block[0]>>1)&0xff;
        }
        bsFrameCounter = next;
    }
    else
    {
        _swi(OS_Byte,_IN(0),19);

        // Read frame counter from the beeb
        bsFrameCounter = ReadByteFromIo((void*)0x6d);
    }
}

void beebScreen_Quit()
{
    if (!bsPiVdu)
    {
        _VDU(BS_CMD_SEND_QUIT);
    }
    
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
        // Mouse coordinate space in 0-1280x0-1024 same as the beeb's graphics coordinate system
        *mx = 640;
        *my = 512;
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

void beebScreen_SetMouseColour(int colour)
{
    bsMouseColour = colour;   
}