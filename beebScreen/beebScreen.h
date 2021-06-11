#ifndef _BEEBSCREEN_H
#define _BEEBSCREEN_H

#include <stdio.h>
#include "../tube-env.h"
#include "../tube-defs.h"
#include "../armcopro.h"
#include "../swis.h"

#ifdef __cplusplus
extern "C"
{
#endif
// ---- BeebScreen Commands ----

#define BS_CMD_SEND_SCREEN (255)
#define BS_CMD_SEND_PAL (254)
#define BS_CMD_SEND_CRTC (253)
#define BS_CMD_SEND_USER1 (252)
#define BS_CMD_SEND_USER2 (251)
#define BS_CMD_SEND_QUIT (250)

// ---- Vector Indices ----
#define BS_VECTOR_USER1 0
#define BS_VECTOR_USER2 1
#define BS_VECTOR_VSYNC 2
#define BS_VECTOR_TIMER 3

#ifndef TRUE
#define TRUE (1)
#define FALSE (0)
#endif

// ---- Definition of VDU (not included in normal export headers) ----
extern void _VDU(int c);

#define MAPRGB(r,g,b) (r)+(g<<12)+(b<<8)

// ---- Colour extract functions, used with beebScreen_MakeNulaPal ----
extern void beebScreen_extractRGB444(int v,int *r,int *g,int *b);
extern void beebScreen_extractRGB555(int v,int *r,int *g,int *b);
extern void beebScreen_extractRGB565(int v,int *r,int *g,int *b);
extern void beebScreen_extractRGB888(int v,int *r,int *g,int *b);
extern void beebScreen_extractBGR444(int v,int *r,int *g,int *b);
extern void beebScreen_extractBGR555(int v,int *r,int *g,int *b);
extern void beebScreen_extractBGR565(int v,int *r,int *g,int *b);
extern void beebScreen_extractBGR888(int v,int *r,int *g,int *b);

// ---- Palette creation functions ----

/**
 * beebScreen_FindPalette
 * Finds the index of the closest matching colour in the palette we're remapping to
 * @param colour - source colour to find
 * @param remap - colours to remap to
 * @param total - total number of colours to remap to
 * @returns remapped colour index
 */
unsigned char beebScreen_FindPalette(int colour, int *remap,int total);

/**
 * beebScreen_CreateRemapColours
 * Creates a map of indices to colour values to remap to a smaller palette
 * @param source - source colours to remap
 * @param remap - colours to be remapped to
 * @param total - total number of colours to remap to
 * @param len - length of the source palette
 */
void beebScreen_CreateRemapColours(int *source, int *remap, int total, int len);

/**
 * beebScreen_SetDefaultNulaRemapColours
 * Sets a set of remap values to remap indices to the current nula palette
 * Uses the set created with beebScreen_CreateRemapColours
 */
void beebScreen_SetDefaultNulaRemapColours();

/**
 * beebScreen_SetNulaRemapColours
 * Sets a set of remap values to remap indices to the current nula palette
 * Uses the set created with beebScreen_CreateRemapColours
 */
void beebScreen_SetNulaRemapColours(unsigned char *remap);

/**
 * beebScreen_MakeNulaPal
 * Converts an RGB palette into a NULA compatible palette entry
 * @param value - RGB palette value
 * @param index - NULA palette index
 * @param extractor - extractor function to extract the RGB values from the palette value
 */
extern int beebScreen_MakeNulaPal(int value,int index,void (*extractor)(int v,int *r, int *g, int *b));

/**
 * beebScreen_SetNulaPal
 * Sets NULA palette values from RGB palette values
 * @param values - Array of palette entries
 * @param output - Array to hold NULA palettes
 * @param count - Number of entries
 * @param extractor - Function to extract RGB values
 */
extern void beebScreen_SetNulaPal(int *values,int *output,int count, void (*extractor)(int v,int *r,int *g,int *b));

/**
 * beebScreen_SendPal
 * Sends a set of NULA palette values to the host
 * @param pal - Array of palette entries
 * @param count - Number of palette entries
 */
extern void beebScreen_SendPal(int *pal,int count);

// ---- Init Flags ----
#define BS_INIT_NULA (1)            // Use VideoNULA for extended palette
#define BS_INIT_DOUBLE_BUFFER (2)   // Use double buffering
#define BS_INIT_MOUSE (4)           // Enable mouse reading
#define BS_INIT_ADFS (8)            // Enable ADFS memory on the host

#define BS_INIT_ALL (0xff)

// ---- Initialisation functions ----
/**
 * beebScreen_Init
 * Initialises the beebScreen library ready for use
 * @param mode - BBC Micro Video Mode to use as the base
 * @param flags - Flags to initialise
 */
extern void beebScreen_Init(int mode, int flags);

/**
 * beebScreen_InjectCode
 * Injects 6502 code into the host's memory (can also inject data if required)
 * @param code - code/data to be injected
 * @param length - size of code/data
 * @param dest - destination address in the host's memory
 */
extern void beebScreen_InjectCode(unsigned char *code, int length,int dest);

/**
 * beebScreen_SetUserVector
 * Sets one of the 4 user vectors to point at user code on the host
 * There are 4 vectors, USER1V, USER2V, VSYNCV and TIMERV
 *      USER1V is called when _VDU(BS_CMD_SEND_USER1) is called
 *      USER2V is called when _VDU(BS_CMD_SEND_USER2) is called
 *      VSYNCV is called from the host's VSYNC interrupt routine
 *      TIMERV is called by the host's TIMER1 interrupt routine
 * @param vector - vector to set (BS_VECTOR_*)
 * @param addr - Host address to set the vector to
 */
extern void beebScreen_SetUserVector(int vector,int addr);

/**
 * beebScreen_SetGeometry
 * Sets the video mode size, can also set CRTC registers to reflect this geometry
 * @param w - Width in pixels (must be a multiple of the modes character block size)
 * @param h - Height in pixels (must be a multiple of 8)
 * @param setCrtc - Set the CRTC registers so that the screen is centered on the display
 */
extern void beebScreen_SetGeometry(int w,int h,int setCrtc);

/**
 * beebScreen_SetScreenBase
 * Sets the screen base address for front or back buffers
 * @param address - address of the screen buffer
 * @param secondBuffer - set to TRUE to set the back buffer's address
 */
extern void beebScreen_SetScreenBase(int address,int secondBuffer);

/**
 * beebScreen_CalcScreenBase
 * Calculates the highest address that the current screen geometry can fit into, wrapping at &8000
 * @param secondBuffer - Calculate for the back buffer, allowing two screens of memory
 */
extern int beebScreen_CalcScreenBase(int secondBuffer);

/**
 * beebScreen_UseDefaultScreenBases
 * Sets the screenbase addresses to the defaults as calculated by beebScreen_CalcScreenBase
 * If double buffer is set the both addresses will be calculated
 */
extern void beebScreen_UseDefaultScreenBases();

/**
 * beebScreen_FlipCallback
 * Sets a callback function called in the flip function to perform any custom updates you require before flipping
 */
extern void beebScreen_FlipCallback(void (*callback)(void));

// ---- Buffer formats (currently only 8bpp supported) ----
#define BS_BUFFER_FORMAT_1BPP (1)
#define BS_BUFFER_FORMAT_2BPP (2)
#define BS_BUFFER_FORMAT_4BPP (4)
#define BS_BUFFER_FORMAT_8BPP (8)

// Is the buffer order swapped from the beeb order (msb is left most pixel on beeb)
#define BS_BUFFER_LITTLE_ENDIAN  (128)

// ---- Setup the screen buffer data ----

/**
 * beebScreen_SetBuffer
 * Sets the framebuffer data used for the non-beeb screen
 * @param buffer - pointer to the buffer
 * @param format - format of the buffer (currently only BS_BUFFER_FORMAT_8BPP is supported)
 * @param w - width of buffer in pixels
 * @param h - height of buffer in pixels
 */
extern void beebScreen_SetBuffer(unsigned char *buffer,int format,int w,int h);

// ---- Flip buffers ----

/**
 * beebScreen_Flip
 * This does the work, it converts the buffer data into a beeb screen format, does the delta compression
 * with the previous frame (correctly works with double buffers) and sends the screen data to the beeb
 */
extern void beebScreen_Flip();

/**
 * beebScreen_VSync
 * Waits for the next VSync
 */
extern void beebScreen_VSync();

// ---- Shutdown ----

/**
 * beebScreen_Quit
 * Shuts down the system, removes vectors from the host side.
 */
extern void beebScreen_Quit();

// ---- Virtual Mouse functions ----

/**
 * beebScreen_GetMouse
 * Gets the last read mouse coordinates and button status
 * @param x - address to hold X coordinate
 * @param y - address to hold Y coordinate
 * @param b - address to hold Buttons
 */
extern void beebScreen_GetMouse(int *x,int *y,int *b);

/**
 * beebScreen_ShowPointer
 * Tells beebScreen if you want to show a virtual mouse pointer.
 * Useful for projects that rely on the host having a hardware mouse pointer
 * @param show - should we show the pointer or not
 */
extern void beebScreen_ShowPointer(int show);

/**
 * beebScreen_GetFrameCounter
 * Returns the host side internal frame counter, this runs from 0 to 255
 * and is incremented every vsync.
 * @returns frame counter from 0 to 255
 */
extern unsigned char beebScreen_GetFrameCounter();

extern int beebScreen_ScanKey(int key);

#ifdef __cplusplus
}
#endif

#endif