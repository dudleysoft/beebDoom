// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//	DOOM graphics stuff for X11, UNIX.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: i_x.c,v 1.6 1997/02/03 22:45:10 b1 Exp $";

#include <stdlib.h>
#include <unistd.h>
//#include <sys/ipc.h>
//#include <sys/shm.h>

//#include <X11/Xlib.h>
//#include <X11/Xutil.h>
//#include <X11/keysym.h>

//#include <X11/extensions/XShm.h>
// Had to dig up XShm.c for this one.
// It is in the libXext, but not in the XFree86 headers.
#ifdef LINUX
int XShmGetEventBase( Display* dpy ); // problems with g++?
#endif

#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
//#include <sys/socket.h>

//#include <netinet/in.h>
//#include <errnos.h>
#include <signal.h>

#include "doomstat.h"
#include "i_system.h"
#include "v_video.h"
#include "m_argv.h"
#include "d_main.h"

#include "doomdef.h"

#include "beebScreen/beebScreen.h"
#include "bbckeycodes.h"

#define POINTER_WARP_COUNTDOWN	1

//#define BEEB_DEBUG

// Display*	X_display=0;
// Window		X_mainWindow;
// Colormap	X_cmap;
// Visual*		X_visual;
// GC		X_gc;
// XEvent		X_event;
// int		X_screen;
// XVisualInfo	X_visualinfo;
// XImage*		image;
// int		X_width;
// int		X_height;

// // MIT SHared Memory extension.
// boolean		doShm;

// XShmSegmentInfo	X_shminfo;
// int		X_shmeventtype;

// // Fake mouse handling.
// // This cannot work properly w/o DGA.
// // Needs an invisible mouse cursor at least.
// boolean		grabMouse;
// int		doPointerWarp = POINTER_WARP_COUNTDOWN;

// // Blocky mode,
// // replace each 320x200 pixel with multiply*multiply pixels.
// // According to Dave Taylor, it still is a bonehead thing
// // to use ....
// static int	multiply=1;


// //
// //  Translates the key currently in X_event
// //

// int xlatekey(void)
// {

//     int rc;

//     switch(rc = XKeycodeToKeysym(X_display, X_event.xkey.keycode, 0))
//     {
//       case XK_Left:	rc = KEY_LEFTARROW;	break;
//       case XK_Right:	rc = KEY_RIGHTARROW;	break;
//       case XK_Down:	rc = KEY_DOWNARROW;	break;
//       case XK_Up:	rc = KEY_UPARROW;	break;
//       case XK_Escape:	rc = KEY_ESCAPE;	break;
//       case XK_Return:	rc = KEY_ENTER;		break;
//       case XK_Tab:	rc = KEY_TAB;		break;
//       case XK_F1:	rc = KEY_F1;		break;
//       case XK_F2:	rc = KEY_F2;		break;
//       case XK_F3:	rc = KEY_F3;		break;
//       case XK_F4:	rc = KEY_F4;		break;
//       case XK_F5:	rc = KEY_F5;		break;
//       case XK_F6:	rc = KEY_F6;		break;
//       case XK_F7:	rc = KEY_F7;		break;
//       case XK_F8:	rc = KEY_F8;		break;
//       case XK_F9:	rc = KEY_F9;		break;
//       case XK_F10:	rc = KEY_F10;		break;
//       case XK_F11:	rc = KEY_F11;		break;
//       case XK_F12:	rc = KEY_F12;		break;
	
//       case XK_BackSpace:
//       case XK_Delete:	rc = KEY_BACKSPACE;	break;

//       case XK_Pause:	rc = KEY_PAUSE;		break;

//       case XK_KP_Equal:
//       case XK_equal:	rc = KEY_EQUALS;	break;

//       case XK_KP_Subtract:
//       case XK_minus:	rc = KEY_MINUS;		break;

//       case XK_Shift_L:
//       case XK_Shift_R:
// 	rc = KEY_RSHIFT;
// 	break;
	
//       case XK_Control_L:
//       case XK_Control_R:
// 	rc = KEY_RCTRL;
// 	break;
	
//       case XK_Alt_L:
//       case XK_Meta_L:
//       case XK_Alt_R:
//       case XK_Meta_R:
// 	rc = KEY_RALT;
// 	break;
	
//       default:
// 	if (rc >= XK_space && rc <= XK_asciitilde)
// 	    rc = rc - XK_space + ' ';
// 	if (rc >= 'A' && rc <= 'Z')
// 	    rc = rc - 'A' + 'a';
// 	break;
//     }

//     return rc;

// }

void I_ShutdownGraphics(void)
{
//   // Detach from X server
//   if (!XShmDetach(X_display, &X_shminfo))
// 	    I_Error("XShmDetach() failed in I_ShutdownGraphics()");

//   // Release shared memory.
//   shmdt(X_shminfo.shmaddr);
//   shmctl(X_shminfo.shmid, IPC_RMID, 0);

//   // Paranoia.
//   image->data = NULL;
	beebScreen_Quit();
	_VDU(22);_VDU(7);
}



// //
// // I_StartFrame
// //
void I_StartFrame (void)
{
//     // er?

}

#define KEY_COUNT 23

int keyMap[] = {
	BBC_ESC,	// Menu
	BBC_a,		// Strafe Left
	BBC_d,		// Strafe Right
	BBC_w,		// Forward
	BBC_s,		// Backwards
	BBC_SPACE,	// Use
	BBC_SHIFT,	// Sprint
	BBC_RETURN,	// Fire
	BBC_TAB,	// Map
	BBC_1,		// First Weapon Select
	BBC_2,
	BBC_3,
	BBC_4,
	BBC_5,
	BBC_6,
	BBC_7,
	BBC_8,
	BBC_9,		// Last Weapon select
	BBC_f6,		// Quick Save
	BBC_f9,		// Quick Load
	BBC_LESS,	// Turn Left
	BBC_GREATER,// Turn Right
	BBC_COPY	// Pause
};

int keyEvent[] = 
{
	KEY_ESCAPE,
	KEY_LEFTARROW,
	KEY_RIGHTARROW,
	KEY_UPARROW,
	KEY_DOWNARROW,
	' ',
	KEY_RSHIFT,
	KEY_RCTRL,
	KEY_TAB,
	'1',
	'2',
	'3',
	'4',
	'5',
	'6',
	'7',
	'8',
	'9',
	KEY_F6,
	KEY_F9,
	',',
	'.',
	KEY_PAUSE
};

#define MENU_KEY_COUNT 47

int menuKeyMap[] = {
	BBC_ESC,
	BBC_CURL,
	BBC_CURR,
	BBC_CURU,
	BBC_CURD,
	BBC_RETURN,
	BBC_SPACE,
	BBC_SHIFT,
	BBC_CTRL,
	BBC_TAB,
	BBC_DELETE,
	BBC_1,
	BBC_2,
	BBC_3,
	BBC_4,
	BBC_5,
	BBC_6,
	BBC_7,
	BBC_8,
	BBC_9,
	BBC_0,
	BBC_a,
	BBC_b,
	BBC_c,
	BBC_d,
	BBC_e,
	BBC_f,
	BBC_g,
	BBC_h,
	BBC_i,
	BBC_j,
	BBC_k,
	BBC_l,
	BBC_m,
	BBC_n,
	BBC_o,
	BBC_p,
	BBC_q,
	BBC_r,
	BBC_s,
	BBC_t,
	BBC_u,
	BBC_v,
	BBC_w,
	BBC_x,
	BBC_y,
	BBC_z
};

int menuKeyEvent[] = {
	KEY_ESCAPE,
	KEY_LEFTARROW,
	KEY_RIGHTARROW,
	KEY_UPARROW,
	KEY_DOWNARROW,
	KEY_ENTER,
	' ',
	KEY_RSHIFT,
	KEY_RCTRL,
	KEY_TAB,
	KEY_BACKSPACE,
	'1',
	'2',
	'3',
	'4',
	'5',
	'6',
	'7',
	'8',
	'9',
	'0',
	'a',
	'b',
	'c',
	'd',
	'e',
	'f',
	'g',
	'h',
	'i',
	'j',
	'k',
	'l',
	'm',
	'n',
	'o',
	'p',
	'q',
	'r',
	's',
	't',
	'u',
	'v',
	'w',
	'x',
	'y',
	'z'
};

int keyDown[]={
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0
};

boolean oldstate = false;
int *oldEvents = NULL;
int oldCount = 0;

// static int	lastmousex = 0;
// static int	lastmousey = 0;
// boolean		mousemoved = false;
// boolean		shmFinished;
void scanKeys(int count, int *keys,int *events)
{
	event_t event;
	oldCount = count;
	oldEvents = events;
	for(int i=0;i<count; ++i)
	{
		if (beebScreen_ScanKey(keys[i]))
		{
			if (!keyDown[i])
			{
				event.type = ev_keydown;
				event.data1 = events[i];
				D_PostEvent(&event);
				keyDown[i]=TRUE;
			}
		}
		else
		{
			if (keyDown[i])
			{
				event.type = ev_keyup;
				event.data1 = events[i];
				D_PostEvent(&event);
				keyDown[i]=FALSE;
			}
		}
	}
}

void releaseOldKeys()
{
	event_t event;

	for(int i=1;i<oldCount;++i)
	{
		if (keyDown[i])
		{
			keyDown[i]=FALSE;
			event.type = ev_keyup;
			event.data1 = oldEvents[i];
			D_PostEvent(&event);
		}
	}
}

void I_GetEvent(void)
{
	if (oldstate != menuactive)
	{
		releaseOldKeys();
		oldstate = menuactive;
	}

	if (!menuactive)
	{
		scanKeys(KEY_COUNT,keyMap,keyEvent);
	}
	else
	{
		scanKeys(MENU_KEY_COUNT,menuKeyMap,menuKeyEvent);
	}
}

// //
// // I_StartTic
// //
void I_StartTic (void)
{

//     if (!X_display)
// 	return;

//     while (XPending(X_display))
 	I_GetEvent();

}


// //
// // I_UpdateNoBlit
// //
void I_UpdateNoBlit (void)
{
//     // what is this?
}

void SetupPalette();

boolean hdmi_pal = false;
boolean nula_pal = false;
boolean grey_pal = false;
boolean pi_pal = false;

int beebPal[256];
int nulaPal[16];
int palMap[256];

// //
// // I_FinishUpdate
// //
void I_FinishUpdate (void)
{
#ifndef BEEB_DEBUG
	// static int col = 0;
	// screens[0][0] = ++col;
	if (nula_pal)
	{
		if (!grey_pal)
			SetupPalette();
	}
	beebScreen_Flip();
	if (nula_pal)
	{
		if (!grey_pal)
		{
			beebScreen_SendPal(nulaPal,16);
		}
	}
	// printf("beebScreen_Flip\n");
#else
	printf("I_FinishUpdate\n");
#endif
}

// //
// // I_ReadScreen
// //
void I_ReadScreen (byte* scr)
{
    memcpy (scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
}

int createPalMap()
{
	int count = 256;
	palMap[0] = 0;
	for(int i = 1; i < 256; i++)
	{
		palMap[i] = i;
		for(int j = 0; j < i; j++)
		{
			if ((beebPal[j] & 0xff0f) == (beebPal[i] & 0xff0f))
			{
				palMap[j] = i;
				j = i;
				count--;
			}
		}
	}
	return count;
}

boolean palSent = false;

void extractBGR888x64(int v,int *r,int *g,int *b)
{
    *b = (v >> 4) & 0x0c;
    *g = (v >> 12) & 0x0c;
    *r = (v >> 20) & 0x0c;
}

int greyPal[]= {0x0000,0x1111,0x2222,0x3333,0x4444,0x5555,0x6666,0x7777,0x8888,0x9999,0xaaaa,0xbbbb,0xcccc,0xdddd,0xeeee,0xffff};

// //
// // I_SetPalette
// //
void I_SetPalette (byte* palette)
{
#ifndef BEEB_DEBUG
	 int newPal[256];
	 for(int i=0;i<256;++i)
	 {
		 newPal[i]=(gammatable[usegamma][palette[0]]<<16)+(gammatable[usegamma][palette[1]]<<8)+(gammatable[usegamma][palette[2]]);
		 palette+=3;
	 }

	if (nula_pal)
	{
		beebScreen_SetNulaPal(newPal,beebPal,256,beebScreen_extractBGR888);
		int count = createPalMap();
		palSent = true;
		if (grey_pal)
		{
			beebScreen_SendPal(greyPal,16);
			beebScreen_CreateRemapColours(beebPal,greyPal,16,256);
		}
	}
	else
	{
		beebScreen_SetNulaPal(newPal,beebPal,256,beebScreen_extractBGR888);
		beebScreen_SendPal(beebPal,256);
	}
#endif
}

//extern int viewheight;
int _lastPalIndices[] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};

void SetupPalette()
{
	int counts[256];
	int most[16];

	// Don't do anything until the palette has been sent once
	if (!palSent)
	{
		return;
	}

	// clear counts
	memset(counts, 0, sizeof(counts));

	// count unique palette colours, note that non-unique palette entries will
	// count all different values, luckily remap will remap them back to the first reference
	byte *p=screens[0];
	for(int i = 0; i < SCREENWIDTH * SCREENHEIGHT; ++i)
	{
		counts[palMap[*p++]]++;
	}

	if (gamestate == GS_LEVEL && viewheight != 200)
	{
		// Add extra for the status bar
		for(int i=168 * SCREENWIDTH; i < SCREENWIDTH * SCREENHEIGHT; ++i)
		{
			counts[palMap[*p++]] += 4;
		}
	}

	int used[16];
	int allocated[16];

	// Count the 16 most populous colours
	for(int i = 0; i < 16; ++i)
	{
		int found = 0;
		int max = counts[0];

		for(int j = 1; j < 256; ++j)
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
	for(int i = 0; i < 16; ++i)
	{
		for(int j = 0; j < 16; ++j)
		{
			if (_lastPalIndices[j] == most[i])
			{
				allocated[i] = TRUE;
				used[j] = TRUE;
				// Update palette (makes pal change work without distrupting the display)
				nulaPal[j] = (beebPal[most[i]] & 0xff0f) | (j<<4);
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
			nulaPal[j] = (beebPal[most[i]] & 0xff0f) | (j<<4);
			_lastPalIndices[j] = most[i];
			used[j] = TRUE;
		}
	}
	beebScreen_CreateRemapColours(beebPal,nulaPal,16,256);
}

void I_InitGraphics(void)
{
#ifndef BEEB_DEBUG
	 if (hdmi_pal)
	 	beebScreen_Init(2,BS_INIT_DOUBLE_BUFFER | BS_INIT_RGB2HDMI);
	 else if (pi_pal)
	 	beebScreen_Init(13, BS_INIT_PIVDU);
	 else
	 	beebScreen_Init(2,BS_INIT_DOUBLE_BUFFER | (nula_pal ? BS_INIT_NULA : 0));

	 if (pi_pal)
	 	beebScreen_SetGeometry(SCREENWIDTH, SCREENHEIGHT, TRUE);
	 else
	 {
	 	beebScreen_SetGeometry(128,192,TRUE);
	 	beebScreen_UseDefaultScreenBases();
	 }
	 if (nula_pal)
	 {
		 beebScreen_SetDefaultNulaRemapColours();
	 }

	 beebScreen_SetBuffer(screens[0], BS_BUFFER_FORMAT_8BPP, SCREENWIDTH, SCREENHEIGHT);
#endif
}


unsigned	exptable[256];

void InitExpand (void)
{
}

double		exptable2[256*256];

void InitExpand2 (void)
{
}

int	inited;



