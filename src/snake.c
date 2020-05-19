/* Copyright (c) 2020, William TANG <galaxyking0419@gmail.com> */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <signal.h>
#include <assert.h>
#include <limits.h>

#ifndef _WIN32
#include <pthread.h>
#endif

#include "tui.h"
#include "queue.h"
#include "snake.h"

/* Mandatory requirements to have a sensible borad size */
static_assert(BOARD_WIDTH > 8 && BOARD_WIDTH <= SHRT_MAX,
	"BOARD_WIDTH is not valid in snake.h!");
static_assert(BOARD_HEIGHT > 8 && BOARD_HEIGHT <= SHRT_MAX,
	"BOARD_HEIGHT is not valid in snake.h!");
static_assert(WIN_SNAKE_SIZE > 3 && WIN_SNAKE_SIZE < (BOARD_HEIGHT - 2) * (BOARD_WIDTH - 2),
	"WIN_SNAKE_SIZE is not valid in snake.h!");

static queue_t snake;
static cord_t food;
static unsigned char snake_direction;
static unsigned char over_type = 0;

#ifdef _WIN32
static LARGE_INTEGER timer_freq;
static LARGE_INTEGER key_hit;
static HANDLE snake_move_mutex;
#else
static struct timespec key_hit;
static pthread_mutex_t snake_move_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

static const char initial_snake[4] = { SNAKE_HEAD, SNAKE_BODY, SNAKE_BODY, '\0' };
static const cord_t initial_snake_cords[3] = {
	{ BOARD_HEIGHT / 2, BOARD_WIDTH / 2 + 1 },
	{ BOARD_HEIGHT / 2, BOARD_WIDTH / 2 },
	{ BOARD_HEIGHT / 2, BOARD_WIDTH / 2 - 1 }
};
static const char opt_str[3][14] = {
	"  New Game  ",
	"    Help    ",
	"    Exit    " 
};

#ifdef _WIN32
always_inline long time_diff_ms(LARGE_INTEGER *const restrict before,
	LARGE_INTEGER *const restrict after)
{
	return (after->QuadPart - before->QuadPart) * 1000L / timer_freq.QuadPart;
}
#else
always_inline void Sleep(long msec)
{
	struct timespec time_to_sleep = { 0L, msec * 1000000L }, remain;
	nanosleep(&time_to_sleep, &remain);
}

always_inline long time_diff_ms(struct timespec *const restrict before,
	struct timespec *const restrict after)
{
	return (after->tv_sec - before->tv_sec) * 1000L +
		(after->tv_nsec - before->tv_nsec) / 1000000L;
}
#endif

void msg_box(short line_num, const char *restrict line, ...)
{
	short line_len = (short)strlen(line);
	short left_x = (BOARD_WIDTH - line_len) / 2 - 2;
	short top_y = (BOARD_HEIGHT - line_num) / 2;

	highlight_text();

	/* Upper line */
	gotoxy(top_y++, left_x);
	putchar('+');
	for (short i = 0; i < line_len + 4; ++i)
		putchar('-');
	putchar('+');

	/* Print the body text */
	va_list ap;
	va_start(ap, line);
	for (short i = 0; i < line_num; ++i) {
		gotoxy(top_y++, left_x);
		printf("|  %s  |", line);
		line = va_arg(ap, const char *);
	}
	va_end(ap);

	/* Bottom line */
	gotoxy(top_y++, left_x);
	putchar('+');
	for (short i = 0; i < line_len + 4; ++i)
		putchar('-');
	putchar('+');

	/* Flush stdout in case of buffering which may
	 * cause the box not showing on the screen
	 */
	fflush(stdout);
	cancel_highlight();

	while (getchar() != CONFIRM_KEY)
		;
}

always_inline cord_t gen_food(void)
{
	queue_t candidates;
	queue_init(&candidates, sizeof(cord_t), 512, 64, 512);

	cord_t candidate;
	for (short i = 2; i < BOARD_HEIGHT - 1; ++i) {
		candidate.y = i;
		for (short j = 2; j < BOARD_WIDTH - 1; ++j) {
			candidate.x = j;
			if (queue_find_the_first_of(&snake, (void *)&candidate) == NULL)
				enqueue(&candidates, (void *)&candidate);
		}
	}

	candidate =
		*(cord_t *)queue_get_item(&candidates, rand() % queue_len(&candidates));

	queue_destory(&candidates);

	return candidate;
}

always_inline void check_over(void)
{
	cord_t *snake_head = queue_back(&snake);

	if (queue_len(&snake) == WIN_SNAKE_SIZE) {
		over_type = 2;
		return;
	} else if (snake_head->x == 1 || snake_head->y == 1 ||
			snake_head->x == BOARD_WIDTH - 1 ||
			snake_head->y == BOARD_HEIGHT - 1) {
		over_type = 1;
		return;
	}

	for (size_t i = 0; i < queue_len(&snake) - 1; ++i) {
		if (memcmp(snake_head, queue_get_item(&snake, i), sizeof(cord_t)) == 0) {
			over_type = 1;
			return;
		}
	}

	over_type = 0;
}

always_inline short find_opposite(void)
{
	switch (snake_direction) {
		case UP_KEY:
			return DOWN_KEY;
		case DOWN_KEY:
			return UP_KEY;
		case LEFT_KEY:
			return RIGHT_KEY;
		case RIGHT_KEY:
			return LEFT_KEY;
		default:
			clrscr();
			restore_console();
			fputs("Unexpected ERROR!", stderr);
			exit(EXIT_UNKNOWN);
	}
}

always_inline void draw_board(short height, short width)
{
	gotoxy(1, 1);

	putchar('+');
	for (short i = 0; i < width - 2; ++i)
		putchar('-');
	putchar('+');
	putchar('\n');

	for (short i = 0; i < height - 2; ++i) {
		putchar('|');
		gotoxy(i + 2, width);
		putchar('|');
		putchar('\n');
	}

	putchar('+');
	for (short i = 0; i < width - 2; ++i)
		putchar('-');
	putchar('+');

	fflush(stdout);
}

/* If the snake ate the food, return true */
always_inline void move_and_draw_snake(void)
{
	cord_t move_offset = { 0, 0 };
	switch (snake_direction) {
		case UP_KEY:
			move_offset.y -= 1;
			break;
		case DOWN_KEY:
			move_offset.y += 1;
			break;
		case RIGHT_KEY:
			move_offset.x += 1;
			break;
		case LEFT_KEY:
			move_offset.x -= 1;
			break;
	}

	cord_t head_node = *(cord_t *)queue_back(&snake);
	gotoxy(head_node.y, head_node.x);
	putchar(SNAKE_BODY);

	head_node.y += move_offset.y;
	head_node.x += move_offset.x;

	/* Do nothing if the snake ate the food */
	if (memcmp(&food, &head_node, sizeof(cord_t)) == 0) {
		food = gen_food();
		gotoxy(food.y, food.x);
		putchar(FOOD);
		fflush(stdout);
	} else {
		cord_t *tail_node = dequeue(&snake);
		gotoxy(tail_node->y, tail_node->x);
		putchar(' ');
	}

	/* Print the new head */
	enqueue(&snake, &head_node);
	gotoxy(head_node.y, head_node.x);
	putchar(SNAKE_HEAD);

	fflush(stdout);
}

#ifdef _WIN32
DWORD WINAPI input_handler(void *dummy)
#else
void *input_handler(void *dummy)
#endif
{
	char ch = 0;
	while (over_type == 0) {
		ch = getchar();
		if ((ch == UP_KEY || ch == DOWN_KEY || ch == LEFT_KEY || ch == RIGHT_KEY) &&
				ch != find_opposite() && ch != snake_direction && over_type == 0) {
			snake_direction = ch;
#ifdef _WIN32
			WaitForSingleObject(snake_move_mutex, INFINITE);
#else
			pthread_mutex_lock(&snake_move_mutex);
#endif
			move_and_draw_snake();
			check_over();
#ifdef _WIN32
			ReleaseMutex(snake_move_mutex);
			QueryPerformanceCounter(&key_hit);
#else
			pthread_mutex_unlock(&snake_move_mutex);
			clock_gettime(CLOCK_MONOTONIC, &key_hit);
#endif
		}
	}

	ungetc(ch, stdin);

#ifdef _WIN32
	ExitThread(0);
#else
	pthread_exit(0);
#endif
}

always_inline void start_game(void)
{
	/* Game flags initialization */
	snake_direction = LEFT_KEY;

	srand((unsigned int)time(NULL));

	/* Initial setup of the game screen */
	clrscr();

	/* Board */
	draw_board(BOARD_HEIGHT, BOARD_WIDTH);

	/* Initial snake */
	queue_populate_init(&snake, sizeof(cord_t),
		(void *)initial_snake_cords, sizeof(initial_snake_cords), 16, 64);
	
	gotoxy(BOARD_HEIGHT / 2, BOARD_WIDTH / 2 - 1);
	puts(initial_snake);

	/* Food */
	food = gen_food();
	gotoxy(food.y, food.x);
	putchar(FOOD);

	fflush(stdout);

#ifdef _WIN32
	HANDLE thread_input_handler =
		CreateThread(NULL, 0, input_handler, NULL, 0, NULL);
	if (thread_input_handler == NULL) {
		clrscr();
		restore_console();
		perror("FATAL->Thread");
		exit(EXIT_THREAD_ERR);
	}

	if ((snake_move_mutex = CreateMutexA(NULL, FALSE, NULL)) == NULL) {
		clrscr();
		restore_console();
		perror("FATAL->Mutex");
		exit(EXIT_THREAD_ERR);
	}

	LARGE_INTEGER now;
	QueryPerformanceCounter(&key_hit);
#else
	pthread_t thread_input_handler;
	if (pthread_create(&thread_input_handler, NULL, input_handler, NULL) != 0) {
		perror("FATAL");
		exit(EXIT_THREAD_ERR);
	}

	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &key_hit);
#endif

	/* Game loop */
	long diff_ms;
	while (over_type == 0) {
#ifdef _WIN32
		QueryPerformanceCounter(&now);
#else
		clock_gettime(CLOCK_MONOTONIC, &now);
#endif
		if ((diff_ms = time_diff_ms(&key_hit, &now)) < 200L) {
			Sleep(GAME_SPEED_MS - diff_ms);
		} else {
#ifdef _WIN32
			WaitForSingleObject(snake_move_mutex, INFINITE);
#else
			pthread_mutex_lock(&snake_move_mutex);
#endif
			move_and_draw_snake();
			check_over();
#ifdef _WIN32
			ReleaseMutex(snake_move_mutex);
#else
			pthread_mutex_unlock(&snake_move_mutex);
#endif
			Sleep(GAME_SPEED_MS);
		}
	}

	if (over_type == 2) {
		msg_box(5,
				"            You Win            ",
				"-------------------------------",
				"Wow, Are you the snake Master?!",
				"                               ",
				"    Press SPACE to continue    ");
	} else {
		msg_box(5,
				"            Game Over          ",
				"-------------------------------",
				"The snake died miserably (x_x) ",
				"                               ",
				"    Press SPACE to continue    ");
	}

#ifdef _WIN32
	CloseHandle(thread_input_handler);
	CloseHandle(snake_move_mutex);
#endif

	/* Cleanups */
	over_type = 0;
	queue_destory(&snake);
}

always_inline int menu(void)
{
	int cur_opt = OPT_START;
	cord_t cur_pos = { 10, 27 };

	clrscr();
	gotoxy(1, 1);

	puts("+---------------------------------------------------------------+\n"
		 "| Author: TIANCHEN TANG                            Version 1.0  |\n"
		 "|                                                               |\n"
		 "|                                                               |\n"
		 "|                         Greedy Snake                          |\n"
		 "|       #                                                       |\n"
		 "|       #                                         #             |\n"
		 "|       ##########@        $                      #             |\n"
		 "|                                                 #             |\n"
		 "|                           New Game              #             |\n"
		 "|        $                    Help         @#######             |\n"
		 "|                             Exit                              |\n"
		 "|                                                       $       |\n"
		 "|                Use w and s to move up and down                |\n"
		 "|                     Press SPACE to select                     |\n"
		 "+---------------------------------------------------------------+");

	/* Highlight "Start game" button */
	gotoxy(cur_pos.y, cur_pos.x);
	highlight_text();
	puts(opt_str[cur_opt]);
	cancel_highlight();

	char ch;
	while (1) {
		if ((ch = getchar())) {
			short offset = 0;

			if (ch == UP_KEY && cur_opt != OPT_START)
				offset = -1;
			else if (ch == DOWN_KEY && cur_opt != OPT_EXIT)
				offset = 1;
			else if (ch == CONFIRM_KEY)
				return cur_opt;

			/* De highlight the previous selection */
			gotoxy(cur_pos.y, cur_pos.x);
			cur_pos.y += offset;
			cancel_highlight();
			puts(opt_str[cur_opt]);
			cur_opt += offset;

			/* Highlight the current selection */
			gotoxy(cur_pos.y, cur_pos.x);
			highlight_text();
			puts(opt_str[cur_opt]);
			cancel_highlight();
			fflush(stdout);
		}
	}
}

void signal_handler(int sig_num)
{
	clrscr();
	cancel_highlight();

	switch (sig_num) {
		case SIGINT:
			puts("SIGINT recieved, exiting...");
			_Exit(EXIT_SIG_INT);
		case SIGTERM:
			puts("SIGTERM recieved, exiting...");
			_Exit(EXIT_SIG_TERM);
		case SIGSEGV:
			puts(
				"SIGSEGV recieved, if you want to improve this game, "
				"please create an issue on GitHub(https://github.com/Galaxy0419/CSnake/issues/new)");
			_Exit(EXIT_SIG_SEGV);
	}

	restore_console();
}

int main(void)
{
	/* Signal handler for control + C, segmentation fault, and termination */
	signal(SIGINT, signal_handler);
	signal(SIGSEGV, signal_handler);
	signal(SIGTERM, signal_handler);

	console_setup();

#ifdef _WIN32
	QueryPerformanceFrequency(&timer_freq);
#endif

	int opt_selcted = OPT_START;
	while (opt_selcted != OPT_EXIT) {
		switch (opt_selcted = menu()) {
			case OPT_START:
				start_game();
				break;
			case OPT_HELP:
				msg_box(4,
					"              Nani            ",
					"------------------------------",
					"What? You don't event know how",
					"to play the snake game???     ");
				break;
		}
	}

	/* Exit the game */
	msg_box(6,
		"           Goodbye          ",
		"----------------------------",
		"Thanks for playing the game!",
		"Have a nice day (^v^)       ",
		"                            ",
		"   Press SPACE to continue  ");

	clrscr();
	restore_console();

	return 0;
}
