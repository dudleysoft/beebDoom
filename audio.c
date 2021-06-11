#include "arm-start.h"
#include "systimer.h"
#include "audio.h"
#include "rpi.h"
#include "swis.h"
#include <stdio.h>

// #ifdef PI_AUDIO

struct bcm2708_dma_cb {
   uint32_t info;
   uint32_t src;
   uint32_t dst;
   uint32_t length;
   uint32_t stride;
   uint32_t next;
   uint32_t pad[2];
   uint32_t buffer[4096];
};

NOINIT_SECTION static __attribute__ ((aligned (0x20))) struct bcm2708_dma_cb dma_cb_data[2];

#include "gpio.h"

#define AUDIO_PIN    (12)

#if (__ARM_ARCH >= 7 )
    #define _data_memory_barrier() {asm volatile ("dmb");}
#else
    void _data_memory_barrier() {  asm volatile ("mcr p15, 0, %0, c7, c10, 5" :: "r" (0));}
#endif

void _clean_cache_area(void * start, unsigned int length)
{
// #if (__ARM_ARCH >= 7 )
//    uint32_t cachelinesize;
//    char * startptr = start;
//    char * endptr;
//    asm volatile ("mrc p15, 0, %0, c0, c0,  1" : "=r" (cachelinesize));
//    cachelinesize = (cachelinesize>> 16 ) & 0xF;
//    cachelinesize = 4<<cachelinesize;
//    endptr = startptr + length;

//    // round down start address
//    startptr = (char *)(((uint32_t)start) & ~(cachelinesize - 1));

//    do{
//       asm volatile ("mcr     p15, 0, %0, c7, c14, 1" : : "r" (startptr));
//       startptr = startptr + cachelinesize;
//    } while ( startptr  < endptr);
// #else
//    asm volatile("mcrr p15,0,%0,%1,c14"::"r" (((char *)start)+length), "r" (start));
//    _data_memory_barrier();
// #endif
}

void RPI_SetGpioPinFunction(rpi_gpio_pin_t gpio, rpi_gpio_alt_function_t func)
{
  rpi_reg_rw_t* fsel_reg = &RPI_GpioBase->GPFSEL[gpio / 10];

  uint32_t fsel_copy = *fsel_reg;
  fsel_copy &= ~(FS_MASK << ((gpio % 10) * 3));
  fsel_copy |= (func << ((gpio % 10) * 3));
  *fsel_reg = fsel_copy;
}

/*
buffer state dma

00 under run fill buffer other buffer
01 fill buffer other buffer
10 nop
11 clear buffer bit as DMA is using it.
*/

static uint32_t buffer_state;
static uint32_t *next_buffer;

size_t rpi_audio_buffer_free_space()
{
   if ((RPI_DMA5Base->SRC_ADR) < (((uint32_t)&dma_cb_data[1].buffer[0]) | GPU_BASE))
   {
      // DMA is in first buffer
      switch (buffer_state)
      {
      case 0://next_buffer = (uint32_t *) &dma_cb_data[1].buffer[0]; break;
      case 1:buffer_state = 0; next_buffer = (uint32_t *)&dma_cb_data[1].buffer[0]; break;
      case 2:return 0;
      case 3:buffer_state = 2; return 0; break;
      }
   } else
   {
      // DMA is in second buffer
      switch (buffer_state)
      {
      case 0://next_buffer = (uint32_t *)&dma_cb_data[0].buffer[0]; break;
      case 2:buffer_state = 0; next_buffer = (uint32_t *)&dma_cb_data[0].buffer[0]; break;
      case 1:return 0;
      case 3:buffer_state = 1; return 0;break;
      }
   }
   return (dma_cb_data->length/sizeof(uint32_t));
}

uint32_t * rpi_audio_buffer_pointer()
{
   return next_buffer;
}

void rpi_audio_samples_written(void)
{
   if (next_buffer<&dma_cb_data[1].buffer[0])
      buffer_state |= 1;
   else
      buffer_state |= 2;
   // make sure the buffer is written out of cache
   _swi(OS_MMUControl,_INR(0,2),2|(21<<8),next_buffer,next_buffer+sizeof(dma_cb_data[0].buffer));
   //_clean_cache_area(next_buffer, sizeof(dma_cb_data[0].buffer));
}

static void init_dma_buffer(size_t buf, uint32_t buffer_init,size_t samples)
{
   dma_cb_data[buf].info = BCM2708_DMA_PER_MAP(5) | BCM2708_DMA_S_WIDTH | BCM2708_DMA_S_INC | BCM2708_DMA_D_DREQ | BCM2708_DMA_WAIT_RESP;
   dma_cb_data[buf].src = ((uint32_t)&dma_cb_data[buf].buffer[0]) | GPU_BASE ;
   dma_cb_data[buf].dst = ((uint32_t)(&RPI_PWMBase->PWM_FIFO) & 0x00ffffff) | PERIPHERAL_BASE_GPU; // physical address of fifo
   dma_cb_data[buf].length = samples * 2 * sizeof(uint32_t);//sizeof(dma_cb_data[buf].buffer);
   dma_cb_data[buf].stride = 0;
   dma_cb_data[buf].next = (uint32_t)&dma_cb_data[(buf+1)%2].info | GPU_BASE;
   dma_cb_data[buf].pad[0] = 0;
   dma_cb_data[buf].pad[1] = 0;
   for (size_t i=0; i<samples * 2; i++)
       dma_cb_data[buf].buffer[i] = buffer_init;
   buffer_state |= 1<<buf;
   //_clean_cache_area(&dma_cb_data[buf],sizeof(dma_cb_data));
   _swi(OS_MMUControl,_INR(0,2),2|(21<<8),&dma_cb_data[buf], &dma_cb_data[buf]+sizeof(dma_cb_data));
}

int pi_detect()
{
   int first = RPI_GetSystemTimer()->counter_lo;
   // Wait for VSync
   _swi(OS_Byte,_IN(0),19);

   if (RPI_GetSystemTimer()->counter_lo == first)
   {
      return 0;
   }
   return 1;
}
// return the sample range
uint32_t rpi_audio_init(uint32_t samplerate,float frameRate)
{
   if (!pi_detect())
      return 0;

   printf("PI_AudioInit\n");

   uint32_t audio_range = 500000000 / (2 * samplerate);
   //uint32_t audio_range = (19200000) / (samplerate);

   uint32_t sampleCount=(int)((float)samplerate / frameRate);

   RPI_SetGpioPinFunction(AUDIO_PIN, FS_ALT0); // PWM1
   RPI_SetGpioPinFunction(AUDIO_PIN+1, FS_ALT0); // PWM1

   RPI_CLKBase->PWM_CTL = PM_PASSWORD | BCM2835_PWMCLK_CNTL_KILL;
   RPI_PWMBase->PWM_CONTROL = 0;

   // samplerate = 500000000 / 2 / range

   // Bits 0..11 Fractional Part Of Divisor = 0, Bits 12..23 Integer Part Of Divisor = 2

   RPI_CLKBase->PWM_DIV = PM_PASSWORD | (0x2000);
   RPI_CLKBase->PWM_CTL = PM_PASSWORD | BCM2835_PWMCLK_CNTL_ENABLE | BCM2835_PWMCLK_CNTL_PLLD ;

   usleep(1);

   RPI_PWMBase->PWM0_RANGE = audio_range;
   RPI_PWMBase->PWM1_RANGE = audio_range;

   init_dma_buffer(0,audio_range>>1,sampleCount);
   init_dma_buffer(1,audio_range>>1,sampleCount);

   usleep(1);

   RPI_PWMBase->PWM_DMAC = PWMDMAC_ENAB | PWMDMAC_THRSHLD;

   // it feals as that we should have | BCM2835_PWM0_REPEATFF | BCM2835_PWM1_REPEATFF  enabled
   //but this appears to half the output frequency.  May be we need to set up PWMDMAC_THRSHLD differently
   RPI_PWMBase->PWM_CONTROL = BCM2835_PWM1_USEFIFO | BCM2835_PWM1_ENABLE |
                              BCM2835_PWM0_USEFIFO | BCM2835_PWM0_ENABLE | BCM2735_PWMx_CLRF ;

   RPI_DMABase->Enable = 1<<5; // enable DMA 5

   RPI_DMA5Base->CS = BCM2708_DMA_RESET;
   usleep(10);
   RPI_DMA5Base->CS = BCM2708_DMA_INT | BCM2708_DMA_END;
   RPI_DMA5Base->ADDR = (uint32_t)&dma_cb_data[0].info | GPU_BASE;
   RPI_DMA5Base->Debug = 7; // clear debug error flags
   usleep(10);
   RPI_DMA5Base->CS = 0x10880000 | BCM2708_DMA_ACTIVE;  // go, mid priority, wait for outstanding writes

   return audio_range;
}

void rpi_audio_quit()
{
   // Disable the PWM channels
   RPI_PWMBase->PWM_CONTROL = BCM2835_PWM0_OFFSTATE | BCM2835_PWM1_OFFSTATE;

   RPI_DMABase->Enable = 0; // disable DMA 5

   // Reset the DMA, hopefully this will stop it reading anything else from the channel
   RPI_DMA5Base->CS = BCM2708_DMA_RESET;
   usleep(10);

}

// #endif