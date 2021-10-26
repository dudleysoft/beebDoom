/**
 * @file beebScreen.h
 * @brief PiNativeArm and ARM7TDMI co-processor support library.
 * beebScreen - A PiNativeArm and ARM7TDMI co-processor support library.
 * This library provides a complete solution for taking an existing C code application or game
 * which uses a linear frame buffer for it's output and converting that output into a format
 * that can be displayed on the BBC Micro's own display.
 * 
 * To do this it provides functionality to convert the original video data into a format that
 * will display on the BBC's screen, including either dithering to the default colour palette
 * of the BBC Micro, or if a VideoNULA chip is installed to either a fixed or dynamic 16 colour
 * palette from the NULAs 12bit RGB palette.
 * 
 * Also supported in the reading of both keyboard and mouse (provided the mouse supports the standard
 * ADVAL 7,8 and 9 method of returning it's coordinates/buttons).
 * 
 * There is no sounds support in this library, althought it's still possible to call host sound 
 * functions via OS_Word to play sounds.
 * @author James Watson
 * @date 25/7/2021
 */

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
#define BS_CMD_SEND_USER1 (252) //!< User Command 1 send using _VDU(BS_CMD_SEND_USER1)
#define BS_CMD_SEND_USER2 (251) //!< User Command 2 send using _VDU(BS_CMD_SEND_USER2)
#define BS_CMD_SEND_QUIT (250)

// ---- Vector Indices ----
#define BS_VECTOR_USER1 0   //!< Host side code address to handle User Command 1
#define BS_VECTOR_USER2 1   //!< Host side code address to handle User Command 2
#define BS_VECTOR_VSYNC 2   //!< Host side code address for extra VSync code
#define BS_VECTOR_TIMER 3   //!< Host side code address for timer interupt handler

#ifndef TRUE
#define TRUE (1)
#define FALSE (0)
#endif

// ---- Definition of VDU (not included in normal export headers) ----
extern void _VDU(int c);

#define MAPRGB(r,g,b) (r)+(g<<12)+(b<<8) //!< Maps an 12bit RGB value to a NULA colour value

// ---- Colour extract functions, used with beebScreen_MakeNulaPal ----
extern void beebScreen_extractRGB444(int v,int *r,int *g,int *b);   //!< Extracts NULA RGB values from a 12bit RGB444 value
extern void beebScreen_extractRGB555(int v,int *r,int *g,int *b);   //!< Extracts NULA RGB values from a 15bit RGB555 value
extern void beebScreen_extractRGB565(int v,int *r,int *g,int *b);   //!< Extracts NULA RGB values from a 16bit RGB565 value
extern void beebScreen_extractRGB888(int v,int *r,int *g,int *b);   //!< Extracts NULA RGB values from a 24bit RGB888 value
extern void beebScreen_extractBGR444(int v,int *r,int *g,int *b);   //!< Extracts NULA RGB values from a 12bit BGR444 value
extern void beebScreen_extractBGR555(int v,int *r,int *g,int *b);   //!< Extracts NULA RGB values from a 15bit BGR555 value
extern void beebScreen_extractBGR565(int v,int *r,int *g,int *b);   //!< Extracts NULA RGB values from a 16bit BGR565 value
extern void beebScreen_extractBGR888(int v,int *r,int *g,int *b);   //!< Extracts NULA RGB values from a 24bit BGR888 value

// ---- Palette creation functions ----

/**
 * Finds the index of the closest matching colour in the palette we're remapping to
 * @param colour - source colour to find
 * @param remap - colours to remap to
 * @param total - total number of colours in remap
 * @returns remapped colour index
 */
unsigned char beebScreen_FindPalette(int colour, int *remap,int total);

/**
 * Creates a map of indices to colour values to remap to a smaller palette
 * @param source - source colours to remap
 * @param remap - colours to be remapped to
 * @param total - total number of colours to remap to
 * @param len - length of the source palette
 */
void beebScreen_CreateRemapColours(int *source, int *remap, int total, int len);

/**
 * Sets a set of remap values to remap indices to the current nula palette
 * Uses the set created with beebScreen_CreateRemapColours()
 */
void beebScreen_SetDefaultNulaRemapColours();

/**
 * Sets a set of remap values to remap indices to the current nula palette
 * Uses a custom set of remap values, this array should be large enough for all the colours used
 * in the current palette.
 * @param remap - set of remap values to use
 */
void beebScreen_SetNulaRemapColours(unsigned char *remap);

/**
 * Converts an RGB palette into a NULA compatible palette entry
 * @param value - RGB palette value
 * @param index - NULA palette index
 * @param extractor - extractor function to extract the RGB values from the palette value
 */
extern int beebScreen_MakeNulaPal(int value,int index,void (*extractor)(int v,int *r, int *g, int *b));

/**
 * Sets NULA palette values from RGB palette values
 * @param values - Array of palette entries
 * @param output - Array to hold NULA palettes
 * @param count - Number of entries
 * @param extractor - Function to extract RGB values
 */
extern void beebScreen_SetNulaPal(int *values,int *output,int count, void (*extractor)(int v,int *r,int *g,int *b));

/**
 * Creates a remap array which maps a palette to the first occurance of each unique NULA palette entry, this is
 * most useful when used in conjunction with beebScreen_CreateDynamicPalette() to help keep the palette entries
 * to the minimum.
 * This only needs to be called after a call to beebScreen_MakeNulaPal() when a program changes it's internal palette.
 * @param pal - Palette to create map for
 * @param count - Number of colours in the palette
 * @param map - Array to write map into
 * @returns Number of unique colours found
 */
extern int beebScreen_CreatePalMap(int *pal,int count,unsigned char *map);

/**
 * Creates a dynamic palette from the current palette, also creates internal remap palette.
 * This routine will try to assign colours in the existing internal palette to the same place where
 * possible to minimise the changes between frames caused by the dynamic palette.
 * Works best when combined with beebScreen_CreatePalMap() to create a map to unique palette colours
 * in the current palette.
 * @param inPal - Source palette
 * @param palMap - Remap from original palette to unique NULA colours
 * @param colours - Number of colours in the original palette (not the remap)
 * @param outPal - Target palette
 * @param target - Number of target colours
 */
extern void beebScreen_CreateDynamicPalette(int* inPal,unsigned char *palMap,int colours,int *outPal,int target);

/**
 * Sends a set of NULA palette values to the host, also required when using non-NULA mode to set the colours
 * to map from for dithering in 2 colour and 16 colour modes.
 * @param pal - Array of palette entries
 * @param count - Number of palette entries
 */
extern void beebScreen_SendPal(int *pal,int count);

/**
 * Sets the current video mode for the host,
 * Currently all graphics modes are supported, technically modes 3 and 6 are supported
 * but since it won't remove the blank lines between character rows those will look a litte
 * strange. Mode 7 is not supported :'(
 * @param mode - BBC Micro video mode
 */
extern void beebScreen_SetMode(int mode);

// ---- Init Flags ----
#define BS_INIT_NORMAL (0)          //!< No special flags, normal single buffered mode
#define BS_INIT_NULA (1)            //!< Use VideoNULA for extended palette
#define BS_INIT_DOUBLE_BUFFER (2)   //!< Use double buffering
#define BS_INIT_MOUSE (4)           //!< Enable mouse reading
#define BS_INIT_ADFS (8)            //!< Enable ADFS memory on the host
#define BS_INIT_RGB2HDMI (16)       //!< Enabled RGB2HDMI video output mode
#define BS_INIT_PIVDU (32)          //!< Enabled PiVdu video frame buffer

#define BS_INIT_ALL (0xff)          //!< Initialise everything

// ---- Initialisation functions ----
/**
 * Initialises the beebScreen library ready for use.
 * Use the following flags to choose what to use:-
 * * BS_INIT_NORMAL - Single buffered, normal BBC palette.
 * * BS_INIT_NULA - Use the VideoNULA for palette.
 * * BS_INIT_DOUBLE_BUFFER - Create two frame buffers, note if there is not enough memory for both they will use the same memory space.
 * * BS_INIT_MOUSE - Enables mouse support, your mouse should support reporting it's state via ADVAL 7,8 and 9.
 * * BS_INIT_ADFS - Set if using ADFS to ensure that the frame buffers on the host don't overwrite ADFS workspace, generally
 *                  you shouldn't need to do this.
 * @param mode - BBC Micro Video Mode to use as the base
 * @param flags - Flags to initialise
 */
extern void beebScreen_Init(int mode, int flags);

/**
 * Injects 6502 code into the host's memory (can also inject data if required)
 * Code injected can be assembled using BeebAsm, or any other compatible 6502 assembler.
 * @param code - code/data to be injected
 * @param length - size of code/data
 * @param dest - destination address in the host's memory
 */
extern void beebScreen_InjectCode(unsigned char *code, int length,int dest);

/**
 * Sets one of the 4 user vectors to point at user code on the host
 * There are 4 vectors, USER1V, USER2V, VSYNCV and TIMERV
 * * BS_VECTOR_USER1 - is called when _VDU(BS_CMD_SEND_USER1) is called
 * * BS_VECTOR_USER2 - is called when _VDU(BS_CMD_SEND_USER2) is called
 * * BS_VECTOR_VSYNC - is called from the host's VSYNC interrupt routine
 * * BS_VECTOR_TIMER - is called by the host's TIMER1 interrupt routine
 * @param vector - vector to set (BS_VECTOR_*)
 * @param addr - Host address to set the vector to
 */
extern void beebScreen_SetUserVector(int vector,int addr);

/**
 * Sets the video mode size, can also set CRTC registers to reflect this geometry
 * @param w - Width in pixels (must be a multiple of the modes character block size)
 * @param h - Height in pixels (must be a multiple of 8)
 * @param setCrtc - Set the CRTC registers so that the screen is centered on the display
 */
extern void beebScreen_SetGeometry(int w,int h,int setCrtc);

/**
 * Sets the screen base address for front or back buffers
 * @param address - address of the screen buffer
 * @param secondBuffer - set to TRUE to set the back buffer's address
 */
extern void beebScreen_SetScreenBase(int address,int secondBuffer);

/**
 * Calculates the highest address that the current screen geometry can fit into, including space for double buffering if enabled
 * @param secondBuffer - Calculate for the back buffer, allowing two screens of memory
 */
extern int beebScreen_CalcScreenBase(int secondBuffer);

/**
 * Sets the screenbase addresses to the defaults as calculated by beebScreen_CalcScreenBase.
 * Calling this after calling beebScreen_SetGeometry() will setup the screen base addresses to sensible defaults based on
 * the new size of the screen, note if you are double buffering it may not always be possible for it to allocate
 * two buffers in the host's memory.
 * If double buffer is set the both addresses will be calculated
 */
extern void beebScreen_UseDefaultScreenBases();

/**
 * Clears all the memory for both screen buffers both locally and on the host.
 * Normally you would use this if the size of the two combined buffers is lower
 * than the normal screen base address since this memory isn't cleared by a mode change.
 * 
 * Call after calling beebScreen_SetGeometry() and beebScreen_UseDefaultScreenBases() to
 * clear screen memory that would normally not be cleared by changing modes
 * @param screenMemory - Clears all screen memory up to 0x8000 if true
 */
extern void beebScreen_ClearScreens(int screenMemory);

/**
 * Sets a callback function called in the flip function to perform any custom updates you require after flipping.
 * @param callback - pointer to callback function
 */
extern void beebScreen_FlipCallback(void (*callback)(void));

// ---- Buffer formats (currently only 8bpp supported) ----
#define BS_BUFFER_FORMAT_1BPP (1)   //!< 1 Bit per pixel buffer (not uet supported)
#define BS_BUFFER_FORMAT_2BPP (2)   //!< 2 Bits per pixel buffer (not yet supported)
#define BS_BUFFER_FORMAT_4BPP (4)   //!< 4 Bits per pixel buffer (not yet supported)
#define BS_BUFFER_FORMAT_8BPP (8)   //!< 8 Bits per pixel buffer

// Is the buffer order swapped from the beeb order (msb is left most pixel on beeb)
#define BS_BUFFER_LITTLE_ENDIAN  (128)  //!< For buffers less than 8bpp which order are pixels stored in the buffer, set for lsb on the left (not yet supported)

// ---- Setup the screen buffer data ----

/**
 * Sets the framebuffer data used for the non-beeb screen.
 * The buffer should be a linear frame buffer when byte 0 is top left and the last byte the bottom right,
 * running in rows from top to bottom, since this is the standard format for frame buffers used in
 * modern software this isn't a particularly difficult requirement to fulfill.
 * @param buffer - pointer to the buffer
 * @param format - format of the buffer (currently only BS_BUFFER_FORMAT_8BPP is supported)
 * @param w - width of buffer in pixels
 * @param h - height of buffer in pixels
 */
extern void beebScreen_SetBuffer(unsigned char *buffer,int format,int w,int h);

// ---- Flip buffers ----

/**
 * Compresses and transfers the current contents of the frame buffer to the BBC Micro's screen.
 * This does the work, it converts the buffer data into a beeb screen format, does the delta compression
 * with the previous frame (correctly works with double buffers) and sends the screen data to the beeb
 */
extern void beebScreen_Flip();

/**
 * Waits for the next VSync, causes the internal frame counter value to be updated from the host
 */
extern void beebScreen_VSync();

// ---- Shutdown ----

/**
 * Shuts down the system, removes vectors from the host side.
 * It is safe to run another beebScreen application after exiting using beebScreen_Quit()
 */
extern void beebScreen_Quit();

// ---- Virtual Mouse functions ----

/**
 * Gets the last read mouse coordinates and button status, returns the middle of the screen if mouse isn't initialised or does not exist.
 * @param x - address to hold X coordinate
 * @param y - address to hold Y coordinate
 * @param b - address to hold Buttons
 */
extern void beebScreen_GetMouse(int *x,int *y,int *b);

/**
 * Tells beebScreen if you want to show a virtual mouse pointer.
 * Useful for projects that rely on the host having a hardware mouse pointer since the beeb doesn't have one.
 * @param show - should we show the pointer or not
 */
extern void beebScreen_ShowPointer(int show);

/**
 * Returns the host side internal frame counter, this runs from 0 to 255
 * and is incremented every vsync, you need to call beebScreen_VSync() or beebScreen_Flip() for this value
 * to be updated from the host's memory.
 * @returns frame counter from 0 to 255
 */
extern unsigned char beebScreen_GetFrameCounter();

/**
 * Returns if the chosen key has been pressed, a full list of keys is available in bbckeycodes.h 
 * @param key - key code to scan for
 */
extern int beebScreen_ScanKey(int key);

#ifdef __cplusplus
}
#endif

#endif