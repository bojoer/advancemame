/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999-2002 Andrea Mazzoleni
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * In addition, as a special exception, Andrea Mazzoleni
 * gives permission to link the code of this program with
 * the MAME library (or with modified versions of MAME that use the
 * same license as MAME), and distribute linked combinations including
 * the two.  You must obey the GNU General Public License in all
 * respects for all of the code used other than MAME.  If you modify
 * this file, you may extend this exception to your version of the
 * file, but you are not obligated to do so.  If you do not wish to
 * do so, delete this exception statement from your version.
 */

#include "itty.h"
#include "log.h"
#include "target.h"
#include "oslinux.h"
#include "error.h"

#ifdef USE_VIDEO_SDL
#include "SDL.h"
#endif

#include <sys/select.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <termios.h>

struct inputb_tty_context {
	unsigned last;
};

static struct inputb_tty_context tty_state;

static adv_device DEVICE[] = {
{ "auto", -1, "Terminal input" },
{ 0, 0, 0 }
};

adv_error inputb_tty_init(int inputb_id)
{
	struct termios adjust;

	log_std(("inputb:tty: inputb_tty_init(id:%d)\n", inputb_id));

#ifdef USE_VIDEO_SDL
	/* If the SDL video driver is used, also the SDL */
	/* keyboard input must be used. */
	if (SDL_WasInit(SDL_INIT_VIDEO)) {
		log_std(("inputb:tty: Incompatible with the SDL video driver\n"));
		error_nolog_cat("tty: Incompatible with the SDL video driver.\n");
		return -1; 
	}
#endif

	/* no buffer */
	setvbuf(stdin, 0, _IONBF, 0);

	/* not canonical input */
	tcgetattr(fileno(stdin), &adjust);
	adjust.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(fileno(stdin), TCSANOW, &adjust);

	tty_state.last = 0;

	return 0;
}

void inputb_tty_done(void)
{
	struct termios adjust;

	log_std(("inputb:tty: inputb_tty_done()\n"));

	/* restore term */
	tcgetattr(fileno(stdin), &adjust);
	adjust.c_lflag |= ICANON | ECHO;
	tcsetattr(fileno(stdin), TCSANOW, &adjust);
}

static int tty_getkey(void)
{
	struct timeval tv;
	fd_set fds;
	int fd = fileno(stdin);
	unsigned char c;

	tv.tv_sec = 0;
	tv.tv_usec = 0;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	if (select(fd + 1, &fds, 0, 0, &tv) > 0) {

		if (read(fd, &c, 1) != 1) {
			return 0;
		}

		return c;
	}

	return 0;
}

adv_bool inputb_tty_hit(void)
{
	log_debug(("inputb:tty: inputb_tty_count_get()\n"));

	if (tty_state.last != 0)
		return 1;

	tty_state.last = tty_getkey();

	return tty_state.last != 0;
}

unsigned inputb_tty_get(void)
{
	const unsigned max = 32;
	char map[max+1];
	unsigned mac;

	log_debug(("inputb:tty: inputb_tty_button_count_get()\n"));

	mac = 0;
	while (mac<max && (mac==0 || tty_state.last)) {

		if (tty_state.last) {
			map[mac] = tty_state.last;
			if (mac > 0 && map[mac] == 27) {
				break;
			}
			++mac;
			tty_state.last = 0;
		} else {
			target_idle();
		}

		tty_state.last = tty_getkey();
	}
	map[mac] = 0;

	if (strcmp(map, "\033[A")==0)
		return INPUTB_UP;
	if (strcmp(map, "\033[B")==0)
		return INPUTB_DOWN;
	if (strcmp(map, "\033[D")==0)
		return INPUTB_LEFT;
	if (strcmp(map, "\033[C")==0)
		return INPUTB_RIGHT;
	if (strcmp(map, "\033[1~")==0)
		return INPUTB_HOME;
	if (strcmp(map, "\033[4~")==0)
		return INPUTB_END;
	if (strcmp(map, "\033[5~")==0)
		return INPUTB_PGUP;
	if (strcmp(map, "\033[6~")==0)
		return INPUTB_PGDN;
	if (strcmp(map, "\033[[A")==0)
		return INPUTB_F1;
	if (strcmp(map, "\033[[B")==0)
		return INPUTB_F2;
	if (strcmp(map, "\033[[C")==0)
		return INPUTB_F3;
	if (strcmp(map, "\033[[D")==0)
		return INPUTB_F4;
	if (strcmp(map, "\033[[E")==0)
		return INPUTB_F5;
	if (strcmp(map, "\033[17~")==0)
		return INPUTB_F6;
	if (strcmp(map, "\033[18~")==0)
		return INPUTB_F7;
	if (strcmp(map, "\033[19~")==0)
		return INPUTB_F8;
	if (strcmp(map, "\033[20~")==0)
		return INPUTB_F9;
	if (strcmp(map, "\033[21~")==0)
		return INPUTB_F10;
	if (strcmp(map, "\r")==0 || strcmp(map, "\n")==0)
		return INPUTB_ENTER;
	if (strcmp(map, "\x7F")==0)
		return INPUTB_BACKSPACE;

	if (mac != 1)
		return 0;
	else
		return (unsigned char)map[0];

	return 0;
}

unsigned inputb_tty_flags(void)
{
	return 0;
}

adv_error inputb_tty_load(adv_conf* context)
{
	return 0;
}

void inputb_tty_reg(adv_conf* context)
{
}

/***************************************************************************/
/* Driver */

inputb_driver inputb_tty_driver = {
	"tty",
	DEVICE,
	inputb_tty_load,
	inputb_tty_reg,
	inputb_tty_init,
	inputb_tty_done,
	inputb_tty_flags,
	inputb_tty_hit,
	inputb_tty_get
};
