################################################################
#
# $Id:$
#
# $Log:$
#
CC = arm-none-eabi-gcc
LD = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
RM = rm

#CFLAGS=-g -Wall -DNORMALUNIX -DLINUX # -DUSEASM 
#LDFLAGS=-L/usr/X11R6/lib
#LIBS=-lXext -lX11 -lnsl -lm
#CFLAGS := -DVFP -DPI_AUDIO -nostartfiles -O2 -mfloat-abi=hard -mfpu=vfp -march=armv6zk -mtune=arm1176jzf-s -fno-delete-null-pointer-checks -fdata-sections -ffunction-sections --specs=nano.specs --specs=nosys.specs -u _printf_float
CFLAGS := -DVIDULA -nostartfiles -O2 -mcpu=arm7tdmi -mtune=arm7tdmi -fno-delete-null-pointer-checks -fdata-sections -ffunction-sections --specs=nano.specs --specs=nosys.specs -u _printf_float
LDFLAGS := -Wl,--gc-sections -Wl,--no-print-gc-sections -Wl,-T,rpi.X -Wl,-lm

CFLAGS += -Dbagi_SDLENV

# not too sophisticated dependency
OBJS=				\
		doomdef.o		\
		doomstat.o		\
		dstrings.o		\
		i_system.o		\
		i_sound.o		\
		i_video.o		\
		i_net.o			\
		tables.o			\
		f_finale.o		\
		f_wipe.o 		\
		d_main.o			\
		d_net.o			\
		d_items.o		\
		g_game.o			\
		m_menu.o			\
		m_misc.o			\
		m_argv.o  		\
		m_bbox.o			\
		m_fixed.o		\
		m_swap.o			\
		m_cheat.o		\
		m_random.o		\
		am_map.o			\
		p_ceilng.o		\
		p_doors.o		\
		p_enemy.o		\
		p_floor.o		\
		p_inter.o		\
		p_lights.o		\
		p_map.o			\
		p_maputl.o		\
		p_plats.o		\
		p_pspr.o			\
		p_setup.o		\
		p_sight.o		\
		p_spec.o			\
		p_switch.o		\
		p_mobj.o			\
		p_telept.o		\
		p_tick.o			\
		p_saveg.o		\
		p_user.o			\
		r_bsp.o			\
		r_data.o			\
		r_draw.o			\
		r_main.o			\
		r_plane.o		\
		r_segs.o			\
		r_sky.o			\
		r_things.o		\
		w_wad.o			\
		wi_stuff.o		\
		v_video.o		\
		st_lib.o			\
		st_stuff.o		\
		hu_stuff.o		\
		hu_lib.o			\
		s_sound.o		\
		z_zone.o			\
		info.o				\
		sounds.o			\
		systimer.o

OBJS +=  armc-start.o armtubeio.o armtubeswis.o beebScreen/beebScreen.o audio.o

all:	 doom

clean:
	$(RM) -f *.o *~ *.flc beebCode/*.o beebScreen/*.bin
	$(RM) -f doom

doom:	$(OBJS) i_main.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) i_main.o -o temp
	$(OBJCOPY) -O binary temp $@
	$(RM) temp

%.o:	%.c
	$(CC) $(CFLAGS) -c $< -o $@

beebScreen/beebScreen.o: beebScreen/beebScreen.c beebScreen/beebCode.c beebScreen/beebScreen.h
	$(CC) $(CFLAGS) -c beebScreen/beebScreen.c -o beebScreen/beebScreen.o

beebScreen/beebCode.c: beebScreen/beebCode.bin
	cd beebScreen; ./mkasm.bat

beebScreen/bagiCode.c: beebScreen/beebCode.asm beebScreen/bagiCode.bin
	cd beebScreen; ./mkasm.bat

beebScreen/beebCode.bin: beebScreen/beebCode.asm

beebScreen/bagiCode.bin: beebScreen/beebCode.asm

%.o: %.s
	$(CC) $(CFLAGS) -c $< -o $@

#############################################################
#
#############################################################