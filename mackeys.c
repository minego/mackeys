#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include <unistd.h>
#include <linux/input.h>


#ifndef TRUE
# define TRUE 1
#endif
#ifndef FALSE
# define FALSE 0
#endif
typedef int BOOL;

typedef enum {
	KEY_STROKE_UP			= 0,
	KEY_STROKE_DOWN			= 1,
	KEY_STROKE_REPEAT		= 2,
} KEY_STROKE_ENUM;

typedef struct input_event input_event;

const struct input_event	syn	= { .type = EV_SYN, .code = SYN_REPORT, .value = 0 };

/* Global options */
int delay = 20000;
BOOL swap = FALSE;

static void usage(const char *program)
{
    fprintf(stderr,
			"%s - Use macOS style copy and paste shortcuts on Linux\n"
			"\n"
			"usage %s [-h | [-d delay]]\n"
			"\n"
			"options:\n"
			"   -h          Show this message and exit\n"
			"   -d          Delay used for key sequences (default: 20000 microseconds)\n"
			"   -s          Swap alt and super keys to more closely match the layout on a mac\n"

			, program, program);
}

static int read_event(struct input_event *event)
{
    return fread(event, sizeof(struct input_event), 1, stdin) == 1;
}

static void write_event(const struct input_event *event)
{
    if (fwrite(event, sizeof(struct input_event), 1, stdout) != 1)
        exit(EXIT_FAILURE);
}

static void fake_event(unsigned short code, KEY_STROKE_ENUM value)
{
	struct input_event	fake;

	fake.type		= EV_KEY;
	fake.code		= code;
	fake.value		= value;

	write_event(&fake);
}

int main(int argc, char **argv)
{
	char				o;
	input_event			event;
	BOOL				left_super_held		= FALSE;
	BOOL				right_super_held	= FALSE;
	BOOL				sent_super_down		= FALSE;
	BOOL				did_modify			= FALSE;
	BOOL				*current;

	while (-1 != (o = getopt(argc, argv, "htTsk:c:"))) {
        switch (o) {
            case 'h':
                usage(argv[0]);
				return EXIT_SUCCESS;

			case 's':
				swap = TRUE;
                break;

			case 'd':
				delay = atoi(optarg);
				break;
        }
    }


    setbuf(stdin,  NULL);
	setbuf(stdout, NULL);

    while (read_event(&event)) {
        if (event.type == EV_MSC && event.code == MSC_SCAN) {
            continue;
		}

        if (event.type != EV_KEY) {
            write_event(&event);
            continue;
        }

		if (swap) {
			switch (event.code) {
				case KEY_LEFTMETA:	event.code = KEY_LEFTALT;	break;
				case KEY_LEFTALT:	event.code = KEY_LEFTMETA;	break;
				case KEY_RIGHTMETA:	event.code = KEY_RIGHTALT;	break;
				case KEY_RIGHTALT:	event.code = KEY_RIGHTMETA;	break;

				default:
					break;
			}
		}

		switch (event.code) {
			case KEY_LEFTMETA:
				current = &left_super_held;
				break;

			case KEY_RIGHTMETA:
				current = &right_super_held;
				break;

			default:
				current = NULL;
				break;
		}

		if (current != NULL) {
			switch (event.value) {
				case KEY_STROKE_UP:
					*current = FALSE;

					if (!did_modify && !sent_super_down) {
						/*
							It looks like the user just tapped the key which
							is useful and should be let through.
						*/
						sent_super_down = TRUE;

						if (left_super_held) {
							fake_event(KEY_LEFTMETA, KEY_STROKE_DOWN);
						}
						if (right_super_held) {
							fake_event(KEY_RIGHTMETA, KEY_STROKE_DOWN);
						}

						write_event(&syn);
						usleep(delay);
					}

					if (!sent_super_down) {
						/* We never sent the down event so don't send the up */
						continue;
					}

					/* Reset our state */
					did_modify = FALSE;
					sent_super_down = FALSE;
					break;

				case KEY_STROKE_DOWN:
					*current = TRUE;

					/* Eat the event for now */
					sent_super_down = FALSE;
					did_modify = FALSE;
					continue;
			}
		}

		if (left_super_held || right_super_held) {
			switch (event.code) {
				default:
					break;

				case KEY_C:
				case KEY_V:
					if (event.value != KEY_STROKE_DOWN) {
						/* Eat this event, we're acting on the down only */
						continue;
					}

					/*
						Trigger events to release the super key(s) and then send
						ctrl+insert or shift+insert.

						Trigger a ctrl+insert or a shift+insert

						Send an event to release the super key(s) and then to press
						the desired modifiers before passing through the real key.
					*/
					if (left_super_held) {
						fake_event(KEY_LEFTMETA, KEY_STROKE_UP);
					}
					if (right_super_held) {
						fake_event(KEY_RIGHTMETA, KEY_STROKE_UP);
					}

					switch (event.code) {
						case KEY_C:
							fake_event(KEY_LEFTCTRL, KEY_STROKE_DOWN);
							break;
						case KEY_V:
							fake_event(KEY_LEFTSHIFT, KEY_STROKE_DOWN);
							break;
					}

					write_event(&syn);
					usleep(delay);

					fake_event(KEY_INSERT, KEY_STROKE_DOWN);
					write_event(&syn);
					usleep(delay);

					/* Undo everything we just did */

					fake_event(KEY_INSERT, KEY_STROKE_UP);
					usleep(delay);

					switch (event.code) {
						case KEY_C:
							fake_event(KEY_LEFTCTRL, KEY_STROKE_UP);
							break;
						case KEY_V:
							fake_event(KEY_LEFTSHIFT, KEY_STROKE_UP);
							break;
					}

					write_event(&syn);
					usleep(delay);

					if (left_super_held) {
						fake_event(KEY_LEFTMETA, KEY_STROKE_DOWN);
					}
					if (right_super_held) {
						fake_event(KEY_RIGHTMETA, KEY_STROKE_DOWN);
					}

					write_event(&syn);
					usleep(delay);

					continue;
			}
		}

		write_event(&event);
    }
}

