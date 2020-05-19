/* Copyright (c) 2020, William TANG <galaxyking0419@gmail.com> */
#ifndef __SNAKE_TUI_H__
#define __SNAKE_TUI_H__

#if !defined(_WIN32) && !defined(__unix__) && !defined(__unix) &&              \
	!defined(__APPLE__)
#error "The Program only support on Windows and POSIX compatible Systems"
#endif

#include "common-def.h"

#ifdef _WIN32
#include <windows.h>

extern void console_setup(void);
extern void restore_console(void);
extern void clrscr(void);

always_inline void gotoxy(short row, short column)
{
	COORD cord = { column - 1, row - 1 };
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), cord);
}

always_inline void highlight_text(void)
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
							FOREGROUND_INTENSITY | BACKGROUND_RED |
								BACKGROUND_GREEN | BACKGROUND_BLUE |
								BACKGROUND_INTENSITY);
}

always_inline void cancel_highlight(void)
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
							FOREGROUND_RED | FOREGROUND_GREEN |
								FOREGROUND_BLUE);
}
#else
#include <stdio.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>

extern void console_setup(void);
extern void restore_console(void);

always_inline void gotoxy(short row, short column)
{
	/* The format "\e[<x>;<y>f" will move
	 * console cursor to x row and y column
	 * Check http://www.termsys.demon.co.uk/vtansi.htm for more info
	 */
	printf("\e[%d;%df", row, column);
}

always_inline void clrscr(void)
{
	/* Clear the terminal screen */
	write(STDOUT_FILENO, "\e[1;1H\e[2J", 11);
}

always_inline void highlight_text(void)
{
	write(STDOUT_FILENO, "\e[30m\e[47m", 10);
}

always_inline void cancel_highlight(void)
{
	write(STDOUT_FILENO, "\e[39m\e[49m", 10);
}
#endif

#endif