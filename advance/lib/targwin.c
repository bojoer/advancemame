/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003 Andrea Mazzoleni
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

#include "target.h"
#include "log.h"
#include "os.h"
#include "snstring.h"
#include "portable.h"

#include "SDL.h"

#include <windows.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <signal.h>

#define BUFFER_SIZE 8192

struct target_context {
	char buffer_out[BUFFER_SIZE];
	char buffer_err[BUFFER_SIZE];
};

static struct target_context TARGET;

/***************************************************************************/
/* Init */

adv_error target_init(void)
{
	memset(&TARGET, 0, sizeof(TARGET));
	return 0;
}

void target_done(void)
{
	target_flush();
}

/***************************************************************************/
/* Scheduling */

void target_yield(void)
{
	Sleep(0);
}

void target_idle(void)
{
	Sleep(1);
}

void target_usleep(unsigned us)
{
}

/***************************************************************************/
/* Clock */

target_clock_t TARGET_CLOCKS_PER_SEC = 1000;

target_clock_t target_clock(void)
{
	return SDL_GetTicks();
}

/***************************************************************************/
/* Hardware */

void target_port_set(unsigned addr, unsigned value)
{
}

unsigned target_port_get(unsigned addr)
{
	return 0;
}

void target_writeb(unsigned addr, unsigned char c)
{
}

unsigned char target_readb(unsigned addr)
{
	return 0;
}

/***************************************************************************/
/* Mode */

void target_mode_reset(void)
{
	/* nothing */
}

/***************************************************************************/
/* Sound */

void target_sound_error(void)
{
	/* MessageBeep(MB_ICONASTERISK); */
}

void target_sound_warn(void)
{
	/* MessageBeep(MB_ICONQUESTION); */
}

void target_sound_signal(void)
{
	/* MessageBeep(MB_OK); */
}

/***************************************************************************/
/* APM */

#define WIN2K_EWX_FORCEIFHUNG 0x00000010

adv_error target_apm_shutdown(void)
{
	OSVERSIONINFO VersionInformation;
	DWORD flags = EWX_POWEROFF;

#if 0
	/* force always the shutdown */
	flags |= EWX_FORCE;
#endif	

	VersionInformation.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (!GetVersionEx(&VersionInformation)) {
		return -1;
	}

	if (VersionInformation.dwPlatformId == VER_PLATFORM_WIN32_NT) {
		HANDLE hToken;
		TOKEN_PRIVILEGES tkp;

	 	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
			return -1;
 
		LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

		tkp.PrivilegeCount = 1;
		tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
 
		AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
 
		if (GetLastError() != ERROR_SUCCESS)
			return -1;

		if ((flags & EWX_FORCE) == 0) {
			if (VersionInformation.dwMajorVersion >= 5)
				flags |= WIN2K_EWX_FORCEIFHUNG;
		}
	}

	if (!ExitWindowsEx(flags, 0)) 
		return -1;

	return 0;	
}

adv_error target_apm_standby(void)
{
	/* nothing */
	return 0;
}

adv_error target_apm_wakeup(void)
{
	/* nothing */
	return 0;
}

/***************************************************************************/
/* Led */

void target_led_set(unsigned mask)
{
}

/***************************************************************************/
/* System */

static int exec(char* cmdline)
{
	DWORD exitcode;
	PROCESS_INFORMATION process;
	STARTUPINFO startup;

	log_std(("win: CreateProcess(%s)\n", cmdline));

	memset(&startup, 0, sizeof(startup));
	startup.cb = sizeof(startup);
	if (!CreateProcess(0, cmdline, 0, 0, FALSE, CREATE_NO_WINDOW, 0, 0, &startup, &process)) {
		log_std(("win: CreateProcess() failed %d\n", (unsigned)GetLastError()));
		return -1;
	}

	/* wait until the process terminate */
	while (1) {
		MSG msg;
		if (!GetExitCodeProcess(process.hProcess, &exitcode)) {
			log_std(("win: GetExitCodeProcess() failed %d\n", (unsigned)GetLastError()));
			exitcode = -1;
			break;
		}

		if (exitcode != STILL_ACTIVE) {
			break;
		}

		if (os_is_quit()) {
			log_std(("win: GetExitCodeProcess() aborted due TERM signal\n"));
			exitcode = -1;
			break;
		}

		/* flush the message queue, otherwise the window isn't updated */
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT)
				break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		Sleep(100);
	}

	log_std(("win: GetExitCodeProcess() return %d\n", (unsigned)exitcode));

	CloseHandle(process.hProcess);
	CloseHandle(process.hThread);

	return exitcode;
}

adv_error target_system(const char* cmd)
{
	char cmdline[TARGET_MAXCMD];
	char* comspec;

	comspec = getenv("COMSPEC");
	if (!comspec) {
		log_std(("win: getenv(COMSPEC) failed\n"));
		return -1;
	}

	sncpy(cmdline, TARGET_MAXCMD, comspec);
	sncat(cmdline, TARGET_MAXCMD, " /C ");
	sncat(cmdline, TARGET_MAXCMD, cmd);

	return exec(cmdline);
}

adv_error target_spawn(const char* file, const char** argv)
{
	char cmdline[TARGET_MAXCMD];
	unsigned i;

	*cmdline = 0;
	for(i=0;argv[i];++i) {
		if (i) {
			sncat(cmdline, TARGET_MAXCMD, " ");
		}
		if (strchr(argv[i], ' ') != 0) {
			sncat(cmdline, TARGET_MAXCMD, "\"");
			sncat(cmdline, TARGET_MAXCMD, argv[i]);
			sncat(cmdline, TARGET_MAXCMD, "\"");
		} else {
			sncat(cmdline, TARGET_MAXCMD, argv[i]);
		}
	}

	return exec(cmdline);
}

adv_error target_mkdir(const char* file)
{
	return mkdir(file);
}

void target_sync(void)
{
	/* nothing */
}

adv_error target_search(char* path, unsigned path_size, const char* file)
{
	char* part;
	DWORD len;

	log_std(("win: target_search(%s)\n", file));

	len = SearchPath(0, file, ".exe", path_size, path, &part);

	if (len > path_size) {
		log_std(("win: SearchPath() failed due buffer too small\n"));
		return -1;
	}

	if (!len) {
		log_std(("win: SearchPath() failed %d\n", (unsigned)GetLastError()));
		return -1;
	}

	log_std(("win: target_search() return %s\n", path));
	return 0;
}

/***************************************************************************/
/* Stream */

void target_out_va(const char* text, va_list arg)
{
	unsigned len = strlen(TARGET.buffer_out);
	if (len < BUFFER_SIZE)
		vsnprintf(TARGET.buffer_out + len, BUFFER_SIZE - len, text, arg);
}

void target_err_va(const char *text, va_list arg)
{
	unsigned len = strlen(TARGET.buffer_err);
	if (len < BUFFER_SIZE)
		vsnprintf(TARGET.buffer_err + len, BUFFER_SIZE - len, text, arg);
}

void target_nfo_va(const char *text, va_list arg)
{
	/* nothing */
}

void target_out(const char *text, ...)
{
	va_list arg;
	va_start(arg, text);
	target_out_va(text, arg);
	va_end(arg);
}

void target_err(const char *text, ...)
{
	va_list arg;
	va_start(arg, text);
	target_err_va(text, arg);
	va_end(arg);
}

void target_nfo(const char *text, ...)
{
	va_list arg;
	va_start(arg, text);
	target_nfo_va(text, arg);
	va_end(arg);
}

void target_flush(void)
{
	MSG msg;

	/* flush the message queue, otherwise the MessageBox may be not displayed */
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) 
			break;
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

	if (*TARGET.buffer_err) {
		MessageBox(NULL, TARGET.buffer_err, "Advance Error", MB_ICONERROR);
		*TARGET.buffer_err = 0;
	}
	
	if (*TARGET.buffer_out) {
		MessageBox(NULL, TARGET.buffer_out, "Advance Message", MB_ICONINFORMATION);
		*TARGET.buffer_out = 0;
	}
}

static void target_backtrace(void)
{
}

void target_signal(int signum)
{
	if (signum == SIGINT) {
		fprintf(stderr, "Break pressed\n\r");
		exit(EXIT_FAILURE);
	} else {
		fprintf(stderr, "Signal %d.\n", signum);
		fprintf(stderr, "%s, %s\n\r", __DATE__, __TIME__);

		if (signum == SIGILL) {
			fprintf(stderr, "Are you using the correct binary ?\n");
		}

		_exit(EXIT_FAILURE);
	}
}

void target_crash(void)
{
	unsigned* i = (unsigned*)0;
	++*i;
	abort();
}

const char* target_option_extract(const char* arg)
{
	if (arg[0] != '-' && arg[0] != '/')
		return 0;
	return arg + 1;
}

adv_bool target_option_compare(const char* arg, const char* opt)
{
	const char* name = target_option_extract(arg);
	return name!=0 && strcasecmp(name, opt) == 0;
}

