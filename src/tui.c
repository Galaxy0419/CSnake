/* Copyright (c) 2020, William TANG <galaxyking0419@gmail.com> */
#include "tui.h"

#ifdef _WIN32
static CONSOLE_FONT_INFOEX cfi_old;

void console_setup(void)
{
	CONSOLE_CURSOR_INFO cursor_info = { 25, FALSE };
	HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleCursorInfo(stdout_handle, &cursor_info);

	DWORD mode = 0;
	HANDLE stdin_handle = GetStdHandle(STD_INPUT_HANDLE);
	GetConsoleMode(stdin_handle, &mode);
	SetConsoleMode(stdin_handle,
				   mode & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT));

	cfi_old.cbSize = sizeof(CONSOLE_FONT_INFOEX);
	GetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi_old);

	CONSOLE_FONT_INFOEX cfi = {
		sizeof(CONSOLE_FONT_INFOEX), 0, { 0, 20 }, 54, FW_NORMAL, L"Terminal"
	};
	SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);
}

void restore_console(void)
{
	CONSOLE_CURSOR_INFO cursor_info = { 25, TRUE };
	HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleCursorInfo(stdout_handle, &cursor_info);

	DWORD mode = 0;
	HANDLE stdin_handle = GetStdHandle(STD_INPUT_HANDLE);
	GetConsoleMode(stdin_handle, &mode);
	SetConsoleMode(stdin_handle, mode | ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);

	SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi_old);
}

void clrscr(void)
{
	HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(stdout_handle, &csbi);

	DWORD buf_size = csbi.dwSize.X * csbi.dwSize.Y;

	DWORD chars_written;
	COORD init_cord = { 0, 0 };
	FillConsoleOutputCharacterA(
		stdout_handle, ' ', buf_size, init_cord, &chars_written);
	FillConsoleOutputAttribute(
		stdout_handle, csbi.wAttributes, buf_size, init_cord, &chars_written);

	SetConsoleCursorPosition(stdout_handle, init_cord);
}
#else
void console_setup(void)
{
	struct termios tmp_config;

	/* Get the current terminal settings */
	tcgetattr(STDIN_FILENO, &tmp_config);

	/* Set the new terminal config to non-canonical and non-echo mode */
	/* (The terminal will not buffer and echo the input) */
	tmp_config.c_lflag &= ~(ICANON | ECHO);

	/* Set the terminal to non blocking mode */
	tcsetattr(STDIN_FILENO, TCSANOW, &tmp_config);

	/* Hide console cursor */
	write(STDOUT_FILENO, "\e[?25l", 6);
}

void restore_console(void)
{
	struct termios tmp_config;

	/* Reset terminal config */
	tcgetattr(STDIN_FILENO, &tmp_config);
	tmp_config.c_lflag |= ICANON | ECHO;
	tcsetattr(STDIN_FILENO, TCSANOW, &tmp_config);
	write(STDOUT_FILENO, "\e[?25h", 6);
}
#endif