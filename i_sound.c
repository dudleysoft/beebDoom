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
//	System interface for sound.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: i_unix.c,v 1.5 1997/02/03 22:45:10 b1 Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <math.h>

#include <sys/time.h>
#include <sys/types.h>

#ifndef LINUX
//#include <sys/filio.h>
#endif

#include <fcntl.h>
#include <unistd.h>
//#include <sys/ioctl.h>

// Linux voxware output.
//#include <linux/soundcard.h>

// Timer stuff. Experimental.
#include <time.h>
#include <signal.h>

#include "z_zone.h"

#include "i_system.h"
#include "i_sound.h"
#include "m_argv.h"
#include "m_misc.h"
#include "w_wad.h"

#include "doomdef.h"

#include "audio.h"

// UNIX hack, to be removed.
#ifdef SNDSERV
// Separate sound server process.
FILE*	sndserver=0;
char*	sndserver_filename = "./sndserver ";
#elif SNDINTR

// Update all 30 millisecs, approx. 30fps synchronized.
// Linux resolution is allegedly 10 millisecs,
//  scale is microseconds.
#define SOUND_INTERVAL     500

// Get the interrupt. Set duration in millisecs.
int I_SoundSetTimer( int duration_of_tick );
void I_SoundDelTimer( void );
#else
// None?
#endif


// A quick hack to establish a protocol between
// synchronous mix buffer updates and asynchronous
// audio writes. Probably redundant with gametic.
static int flag = 0;

// The number of internal mixing channels,
//  the samples calculated for each mixing step,
//  the size of the 16bit, 2 hardware channel (stereo)
//  mixing buffer, and the samplerate of the raw data.


// Needed for calling the actual sound output.
#define SAMPLECOUNT		11025/10
#define NUM_CHANNELS		8
// It is 2 for 16bit, and 2 for two channels.
#define BUFMUL                  4
#define MIXBUFFERSIZE		(SAMPLECOUNT*BUFMUL)

#define SAMPLERATE		11025	// Hz
#define SAMPLESIZE		2   	// 16bit

// The actual lengths of all sound effects.
int 		lengths[NUMSFX];

// The actual output device.
int	audio_fd;

int sample_range;

// The global mixing buffer.
// Basically, samples from all active internal channels
//  are modifed and added, and stored in the buffer
//  that is submitted to the audio device.
signed short	mixbuffer[MIXBUFFERSIZE];


// The channel step amount...
unsigned int	channelstep[NUM_CHANNELS];
// ... and a 0.16 bit remainder of last step.
unsigned int	channelstepremainder[NUM_CHANNELS];


// The channel data pointers, start and end.
unsigned char*	channels[NUM_CHANNELS];
unsigned char*	channelsend[NUM_CHANNELS];


// Time/gametic that the channel started playing,
//  used to determine oldest, which automatically
//  has lowest priority.
// In case number of active sounds exceeds
//  available channels.
int		channelstart[NUM_CHANNELS];

// The sound in channel handles,
//  determined on registration,
//  might be used to unregister/stop/modify,
//  currently unused.
int 		channelhandles[NUM_CHANNELS];

// SFX id of the playing sound effect.
// Used to catch duplicates (like chainsaw).
int		channelids[NUM_CHANNELS];			

// Pitch to stepping lookup, unused.
int		steptable[256]={
  16384,16562,16742,16925,17109,17295,17484,17674,17866,18061,18258,18456,18657,18861,19066,19274,19483,19696,19910,20127,20346,20568,20792,21018,21247,21478,21712,21949,22188,22429,22673,22920,23170,23422,23677,23935,24196,24459,24726,24995,25267,25542,25820,26102,26386,26673,26964,27257,27554,27854,28157,28464,28774,29087,29404,29724,30048,30375,30706,31040,31378,31720,32065,32415,32768,33124,33485,33850,34218,34591,34968,35348,35733,36122,36516,36913,37315,37722,38132,38548,38967,39392,39821,40254,40693,41136,41584,42037,42494,42957,43425,43898,44376,44859,45347,45841,46340,46845,47355,47871,48392,48919,49452,49990,50535,51085,51641,52204,52772,53347,53928,54515,55108,55709,56315,56928,57548,58175,58809,59449,60096,60751,61412,62081,62757,63440,64131,64830,65536,66249,66971,67700,68437,69182,69936,70697,71467,72245,73032,73827,74631,75444,76265,77096,77935,78784,79642,80509,81386,82272,83168,84074,84989,85915,86850,87796,88752,89718,90695,91683,92681,93691,94711,95742,96785,97839,98904,99981,101070,102170,103283,104408,105545,106694,107856,109030,110217,111418,112631,113857,115097,116351,117618,118898,120193,121502,122825,124162,125514,126881,128263,129660,131072,132499,133942,135400,136875,138365,139872,141395,142935,144491,146064,147655,149263,150888,152531,154192,155871,157569,159284,161019,162772,164545,166337,168148,169979,171830,173701,175592,177504,179437,181391,183367,185363,187382,189422,191485,193570,195678,197809,199963,202140,204341,206566,208816,211090,213388,215712,218061,220435,222836,225262,227715,230195,232702,235236,237797,240387,243004,245650,248325,251029,253763,256526,259320
};

// Volume lookups.
int		vol_lookup[128*256];

// Hardware left and right channel volume lookup.
int*		channelleftvol_lookup[NUM_CHANNELS];
int*		channelrightvol_lookup[NUM_CHANNELS];




//
// Safe ioctl, convenience.
//
void
myioctl
( int	fd,
  int	command,
  int*	arg )
{   
    int		rc;
    extern int	errno;
    
  //   rc = ioctl(fd, command, arg);  
  //   if (rc < 0)
  //   {
	// fprintf(stderr, "ioctl(dsp,%d,arg) failed\n", command);
	// fprintf(stderr, "errno=%d\n", errno);
	// exit(-1);
  //   }
}





//
// This function loads the sound data from the WAD lump,
//  for single sound.
//
void*
getsfx
( char*         sfxname,
  int*          len )
{
    unsigned char*      sfx;
    unsigned char*      paddedsfx;
    int                 i;
    int                 size;
    int                 paddedsize;
    char                name[20];
    int                 sfxlump;

    
    // Get the sound data from the WAD, allocate lump
    //  in zone memory.
    sprintf(name, "ds%s", sfxname);

    // Now, there is a severe problem with the
    //  sound handling, in it is not (yet/anymore)
    //  gamemode aware. That means, sounds from
    //  DOOM II will be requested even with DOOM
    //  shareware.
    // The sound list is wired into sounds.c,
    //  which sets the external variable.
    // I do not do runtime patches to that
    //  variable. Instead, we will use a
    //  default sound for replacement.
    if ( W_CheckNumForName(name) == -1 )
      sfxlump = W_GetNumForName("dspistol");
    else
      sfxlump = W_GetNumForName(name);
    
    size = W_LumpLength( sfxlump );

    // Debug.
    // fprintf( stderr, "." );
    //fprintf( stderr, " -loading  %s (lump %d, %d bytes)\n",
    //	     sfxname, sfxlump, size );
    //fflush( stderr );
    
    sfx = (unsigned char*)W_CacheLumpNum( sfxlump, PU_STATIC );

    // Pads the sound effect out to the mixing buffer size.
    // The original realloc would interfere with zone memory.
    paddedsize = ((size-8 + (SAMPLECOUNT-1)) / SAMPLECOUNT) * SAMPLECOUNT;

    // Allocate from zone memory.
    paddedsfx = (unsigned char*)Z_Malloc( paddedsize+8, PU_STATIC, 0 );
    // ddt: (unsigned char *) realloc(sfx, paddedsize+8);
    // This should interfere with zone memory handling,
    //  which does not kick in in the soundserver.

    // Now copy and pad.
    memcpy(  paddedsfx, sfx, size );
    for (i=size ; i<paddedsize+8 ; i++)
        paddedsfx[i] = 128;

    // Remove the cached lump.
    Z_Free( sfx );
    
    // Preserve padded length.
    *len = paddedsize;

    // Return allocated padded data.
    return (void *) (paddedsfx + 8);
}





//
// This function adds a sound to the
//  list of currently active sounds,
//  which is maintained as a given number
//  (eight, usually) of internal channels.
// Returns a handle.
//
int
addsfx
( int		sfxid,
  int		volume,
  int		step,
  int		seperation )
{
    static unsigned short	handlenums = 0;
 
    int		i;
    int		rc = -1;
    
    int		oldest = gametic;
    int		oldestnum = 0;
    int		slot;

    int		rightvol;
    int		leftvol;

    // Chainsaw troubles.
    // Play these sound effects only one at a time.
    if ( sfxid == sfx_sawup
	 || sfxid == sfx_sawidl
	 || sfxid == sfx_sawful
	 || sfxid == sfx_sawhit
	 || sfxid == sfx_stnmov
	 || sfxid == sfx_pistol	 )
    {
      // Loop all channels, check.
      for (i=0 ; i<NUM_CHANNELS ; i++)
      {
          // Active, and using the same SFX?
          if ( (channels[i])
        && (channelids[i] == sfxid) )
          {
        // Reset.
        channels[i] = 0;
        // We are sure that iff,
        //  there will only be one.
        break;
          }
      }
    }

    // Loop all channels to find oldest SFX.
    for (i=0; (i<NUM_CHANNELS) && (channels[i]); i++)
    {
      if (channelstart[i] < oldest)
      {
          oldestnum = i;
          oldest = channelstart[i];
      }
    }

    // Tales from the cryptic.
    // If we found a channel, fine.
    // If not, we simply overwrite the first one, 0.
    // Probably only happens at startup.
    if (i == NUM_CHANNELS)
    	slot = oldestnum;
    else
    	slot = i;

    // Okay, in the less recent channel,
    //  we will handle the new SFX.
    // Set pointer to raw data.
    channels[slot] = (unsigned char *) S_sfx[sfxid].data;
    // Set pointer to end of raw data.
    channelsend[slot] = channels[slot] + lengths[sfxid];

    // Reset current handle number, limited to 0..100.
    if (!handlenums)
    	handlenums = 100;

    // Assign current handle number.
    // Preserved so sounds could be stopped (unused).
    channelhandles[slot] = rc = handlenums++;

    // Set stepping???
    // Kinda getting the impression this is never used.
    channelstep[slot] = step;
    // ???
    channelstepremainder[slot] = 0;
    // Should be gametic, I presume.
    channelstart[slot] = gametic;

    // Separation, that is, orientation/stereo.
    //  range is: 1 - 256
    seperation += 1;

    // Per left/right channel.
    //  x^2 seperation,
    //  adjust volume properly.
    leftvol = volume - ((volume*seperation*seperation) >> 16); ///(256*256);
    seperation = seperation - 257;
    rightvol = volume - ((volume*seperation*seperation) >> 16);	

    // Sanity check, clamp volume.
    if (rightvol < 0 || rightvol > 127)
	    I_Error("rightvol out of bounds");
    
    if (leftvol < 0 || leftvol > 127)
	    I_Error("leftvol out of bounds");
    
    // Get the proper lookup table piece
    //  for this volume level???
    channelleftvol_lookup[slot] = &vol_lookup[leftvol*256];
    channelrightvol_lookup[slot] = &vol_lookup[rightvol*256];

    // Preserve sound SFX id,
    //  e.g. for avoiding duplicates of chainsaw.
    channelids[slot] = sfxid;

    // You tell me.
    return rc;
}

//
// SFX API
// Note: this was called by S_Init.
// However, whatever they did in the
// old DPMS based DOS version, this
// were simply dummies in the Linux
// version.
// See soundserver initdata().
//
void I_SetChannels()
{
  // Init internal lookups (raw data, mixing buffer, channels).
  // This function sets up internal lookups used during
  //  the mixing process. 
  int		i;
  int		j;
    
  int*	steptablemid = steptable + 128;
  
  // Okay, reset internal mixing channels to zero.
  for (i=0; i<NUM_CHANNELS; i++)
  {
    channels[i] = 0;
  }

  // This table provides step widths for pitch parameters.
  // I fail to see that this is currently used.
  // for (i=-128 ; i<128 ; i++)
  //   steptablemid[i] = (int)(pow(2.0, (i/64.0))*65536.0);
  
  
  // Generates volume lookup tables
  //  which also turn the unsigned samples
  //  into signed samples.
  for (i=0 ; i<128 ; i++)
    for (j=0 ; j<256 ; j++)
      vol_lookup[i*256+j] = (i*(j-128)*256)/127;
}	

 
void I_SetSfxVolume(int volume)
{
  // Identical to DOS.
  // Basically, this should propagate
  //  the menu/config file setting
  //  to the state variable used in
  //  the mixing.
  snd_SfxVolume = volume;
}

// MUSIC API - dummy. Some code from DOS version.
void I_SetMusicVolume(int volume)
{
  // Internal state variable.
  snd_MusicVolume = volume;
  // Now set volume on output device.
  // Whatever( snd_MusciVolume );
}


//
// Retrieve the raw data lump index
//  for a given SFX name.
//
int I_GetSfxLumpNum(sfxinfo_t* sfx)
{
    char namebuf[9];
    sprintf(namebuf, "ds%s", sfx->name);
    return W_GetNumForName(namebuf);
}

//
// Starting a sound means adding it
//  to the current list of active sounds
//  in the internal channels.
// As the SFX info struct contains
//  e.g. a pointer to the raw data,
//  it is ignored.
// As our sound handling does not handle
//  priority, it is ignored.
// Pitching (that is, increased speed of playback)
//  is set, but currently not used by mixing.
//
int
I_StartSound
( int		id,
  int		vol,
  int		sep,
  int		pitch,
  int		priority )
{

  // UNUSED
  priority = 0;
}



void I_StopSound (int handle)
{
  // You need the handle returned by StartSound.
  // Would be looping all channels,
  //  tracking down the handle,
  //  an setting the channel to zero.
  
  // UNUSED.
  handle = 0;
}


int I_SoundIsPlaying(int handle)
{
    // Ouch.
    return gametic < handle;
}




//
// This function loops all active (internal) sound
//  channels, retrieves a given number of samples
//  from the raw sound data, modifies it according
//  to the current (internal) channel parameters,
//  mixes the per channel samples into the global
//  mixbuffer, clamping it to the allowed range,
//  and sets up everything for transferring the
//  contents of the mixbuffer to the (two)
//  hardware channels (left and right, that is).
//
// This function currently supports only 16bit.
//
void I_UpdateSound( void )
{
#ifdef SNDINTR
  // Debug. Count buffer misses with interrupt.
  static int misses = 0;
#endif

  if (sample_range == 0)
  {
    return;
  }

  
  // Mix current sound data.
  // Data, from raw sound, for right and left.
  register unsigned int	sample;
  register int		dl;
  register int		dr;
  
  // Pointers in global mixbuffer, left, right, end.
  signed short*		leftout;
  signed short*		rightout;
  signed short*		leftend;
  // Step in mixbuffer, left and right, thus two.
  int				step;

  // Mixing channel index.
  int				chan;
    
  // Left and right channel
  //  are in global mixbuffer, alternating.
  leftout = mixbuffer;
  rightout = mixbuffer+1;
  step = 2;

  // Determine end, for left channel only
  //  (right channel is implicit).
  leftend = mixbuffer + SAMPLECOUNT*step;

  // Mix sounds into the mixing buffer.
  // Loop over step*SAMPLECOUNT,
  //  that is 512 values for two channels.
  while (leftout != leftend)
  {
    // Reset left/right value. 
    dl = 0;
    dr = 0;

    // Love thy L2 chache - made this a loop.
    // Now more channels could be set at compile time
    //  as well. Thus loop those  channels.
    for ( chan = 0; chan < NUM_CHANNELS; chan++ )
    {
        // Check channel, if active.
        if (channels[ chan ])
        {
          // Get the raw data from the channel. 
          sample = *channels[ chan ];
          // Add left and right part
          //  for this channel (sound)
          //  to the current data.
          // Adjust volume accordingly.
          dl += channelleftvol_lookup[ chan ][sample];
          dr += channelrightvol_lookup[ chan ][sample];
          // Increment index ???
          channelstepremainder[ chan ] += channelstep[ chan ];
          // MSB is next sample???
          channels[ chan ] += channelstepremainder[ chan ] >> 16;
          // Limit to LSB???
          channelstepremainder[ chan ] &= 65536-1;

          // Check whether we are done.
          if (channels[ chan ] >= channelsend[ chan ])
              channels[ chan ] = 0;
        }
    }
    
    // Clamp to range. Left hardware channel.
    // Has been char instead of short.
    // if (dl > 127) *leftout = 127;
    // else if (dl < -128) *leftout = -128;
    // else *leftout = dl;

    if (dl > 0x7fff)
        *leftout = 0x7fff;
    else if (dl < -0x8000)
        *leftout = -0x8000;
    else
        *leftout = dl;

    // Same for right hardware channel.
    if (dr > 0x7fff)
        *rightout = 0x7fff;
    else if (dr < -0x8000)
        *rightout = -0x8000;
    else
        *rightout = dr;

    // Increment current pointers in mixbuffer.
    leftout += step;
    rightout += step;
  }

  if (sample_range > 0)
  {
    if (rpi_audio_buffer_free_space())
    {
      unsigned int *piPtr = (unsigned int *)rpi_audio_buffer_pointer();
      // TODO - Convert the contents of the mix buffer into the audio buffer
      signed short *mptr = mixbuffer;
      for(int i = 0;i < SAMPLECOUNT * 2; i++)
      {
        *piPtr++ = (*mptr++ * sample_range) >> 16;
      }
      rpi_audio_samples_written();
    }
  }

}


// 
// This would be used to write out the mixbuffer
//  during each game loop update.
// Updates sound buffer and audio device at runtime. 
// It is called during Timer interrupt with SNDINTR.
// Mixing now done synchronous, and
//  only output be done asynchronous?
//
void
I_SubmitSound(void)
{
  // Write it to DSP device.
  // write(audio_fd, mixbuffer, SAMPLECOUNT*BUFMUL);
}



void
I_UpdateSoundParams
( int	handle,
  int	vol,
  int	sep,
  int	pitch)
{
  // I fail too see that this is used.
  // Would be using the handle to identify
  //  on which channel the sound might be active,
  //  and resetting the channel parameters.

  // UNUSED.
  handle = vol = sep = pitch = 0;
}




void I_ShutdownSound(void)
{    
  if (sample_range > 0)
  {
    rpi_audio_quit();
  }
  // Done.
  return;
}

void
I_InitSound()
{ 
  sample_range = 0;//rpi_audio_init(SAMPLERATE, 10);
}




//
// MUSIC API.
// Still no music done.
// Remains. Dummies.
//
void I_InitMusic(void)		{ }
void I_ShutdownMusic(void)	{ }

static int	looping=0;
static int	musicdies=-1;

void I_PlaySong(int handle, int looping)
{
  // UNUSED.
  handle = looping = 0;
  musicdies = gametic + TICRATE*30;
}

void I_PauseSong (int handle)
{
  // UNUSED.
  handle = 0;
}

void I_ResumeSong (int handle)
{
  // UNUSED.
  handle = 0;
}

void I_StopSong(int handle)
{
  // UNUSED.
  handle = 0;
  
  looping = 0;
  musicdies = 0;
}

void I_UnRegisterSong(int handle)
{
  // UNUSED.
  handle = 0;
}

int I_RegisterSong(void* data)
{
  // UNUSED.
  data = NULL;
  
  return 1;
}

// Is the song playing?
int I_QrySongPlaying(int handle)
{
  // UNUSED.
  handle = 0;
  return looping || musicdies > gametic;
}



//
// Experimental stuff.
// A Linux timer interrupt, for asynchronous
//  sound output.
// I ripped this out of the Timer class in
//  our Difference Engine, including a few
//  SUN remains...
//  
#ifdef sun
    typedef     sigset_t        tSigSet;
#else    
    typedef     int             tSigSet;
#endif


// We might use SIGVTALRM and ITIMER_VIRTUAL, if the process
//  time independend timer happens to get lost due to heavy load.
// SIGALRM and ITIMER_REAL doesn't really work well.
// There are issues with profiling as well.
static int /*__itimer_which*/  itimer = ITIMER_REAL;

static int sig = SIGALRM;

// Interrupt handler.
void I_HandleSoundTimer( int ignore )
{
  // Debug.
  //fprintf( stderr, "%c", '+' ); fflush( stderr );
  
  // Feed sound device if necesary.
  // if ( flag )
  // {
  //   // See I_SubmitSound().
  //   // Write it to DSP device.
  //   write(audio_fd, mixbuffer, SAMPLECOUNT*BUFMUL);

  //   // Reset flag counter.
  //   flag = 0;
  // }
  // else
  //   return;
  
  // // UNUSED, but required.
  // ignore = 0;
  return;
}

// Get the interrupt. Set duration in millisecs.
int I_SoundSetTimer( int duration_of_tick )
{
//   // Needed for gametick clockwork.
//   struct itimerval    value;
//   struct itimerval    ovalue;
//   struct sigaction    act;
//   struct sigaction    oact;

//   int res;
  
//   // This sets to SA_ONESHOT and SA_NOMASK, thus we can not use it.
//   //     signal( _sig, handle_SIG_TICK );
  
//   // Now we have to change this attribute for repeated calls.
//   act.sa_handler = I_HandleSoundTimer;
// #ifndef sun    
//   //ac	t.sa_mask = _sig;
// #endif
//   act.sa_flags = SA_RESTART;
  
//   sigaction( sig, &act, &oact );

//   value.it_interval.tv_sec    = 0;
//   value.it_interval.tv_usec   = duration_of_tick;
//   value.it_value.tv_sec       = 0;
//   value.it_value.tv_usec      = duration_of_tick;

//   // Error is -1.
//   res = setitimer( itimer, &value, &ovalue );

//   // Debug.
//   if ( res == -1 )
//     fprintf( stderr, "I_SoundSetTimer: interrupt n.a.\n");
  
  return 0;
}


// Remove the interrupt. Set duration to zero.
void I_SoundDelTimer()
{
  // Debug.
  if ( I_SoundSetTimer( 0 ) == -1)
    fprintf( stderr, "I_SoundDelTimer: failed to remove interrupt. Doh!\n");
}
