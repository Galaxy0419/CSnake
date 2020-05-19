/* Copyright (c) 2020, William TANG <galaxyking0419@gmail.com> */
#ifndef _SNAKE_H_
#define _SNAKE_H_

#if !defined(_WIN32) && !defined(__unix__) && !defined(__unix) &&              \
	!defined(__APPLE__)
#error "The Program only support on Windows and POSIX compatible Systems"
#endif

#define GAME_SPEED_MS 200L
#define WIN_SNAKE_SIZE 32

#define BOARD_WIDTH 64
#define BOARD_HEIGHT 16

#define SNAKE_HEAD '@'
#define SNAKE_BODY '#'
#define FOOD '$'

#define UP_KEY 'w'
#define DOWN_KEY 's'
#define LEFT_KEY 'a'
#define RIGHT_KEY 'd'
#define CONFIRM_KEY ' '

enum { OPT_START, OPT_HELP, OPT_EXIT };

enum {
	EXIT_CLEAN,
	EXIT_THREAD_ERR,
	EXIT_SIG_INT,
	EXIT_SIG_TERM,
	EXIT_SIG_SEGV,
	EXIT_UNKNOWN
};

typedef struct {
	short y;
	short x;
} cord_t;

#endif