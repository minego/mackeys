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
int isTerm = -1;
char *cmd = NULL;
char *keys = "cvx";
int delay = 20000;
BOOL swap = FALSE;

static void usage(const char *program)
{
    fprintf(stderr,
			"%s - Use macOS style copy and paste shortcuts on Linux\n"
			"\n"
			"usage %s [-h | [-t | -T | [-c cmd]] [-k keys] [-d delay]]\n"
			"\n"
			"options:\n"
			"   -h          Show this message and exit\n"
			"   -t          Assume terminal mode, send ctrl+shift+<key>\n"
			"   -T          Assume NOT terminal mode, send ctrl+<key>\n"
			"   -d          Delay used for key sequences (default: 20000 microseconds)\n"
			"   -c cmd      Command to run to determine mode. An exit code of 0 is used to\n"
			"               indicate terminal mode.\n"
			"   -k keys     Specify a list of keys that should be converted. Default: cvx\n"
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

static int check_for_terminal(void)
{
	FILE		*f;

	switch (isTerm) {
		case 1:
		case 0:
			return isTerm;

		default:
			if (cmd != NULL) {
				f = popen(cmd, "r");
				if (f != NULL && pclose(f) == 0) {
					return 1;
				}
			}
			return 0;
	}
}

static char key2char(unsigned short k)
{
	switch (k) {
		case KEY_A:	return 'a';
		case KEY_B:	return 'b';
		case KEY_C: return 'c';
		case KEY_D: return 'd';
		case KEY_E: return 'e';
		case KEY_F: return 'f';
		case KEY_G: return 'g';
		case KEY_H: return 'h';
		case KEY_I: return 'i';
		case KEY_J: return 'j';
		case KEY_K: return 'k';
		case KEY_L: return 'l';
		case KEY_M: return 'm';
		case KEY_N: return 'n';
		case KEY_O: return 'o';
		case KEY_P: return 'p';
		case KEY_Q: return 'q';
		case KEY_R: return 'r';
		case KEY_S: return 's';
		case KEY_T: return 't';
		case KEY_U: return 'u';
		case KEY_V: return 'v';
		case KEY_W: return 'w';
		case KEY_X: return 'x';
		case KEY_Y: return 'y';
		case KEY_Z: return 'z';

		default:	return '\0';
	}
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
	BOOL				terminal_state;

	while (-1 != (o = getopt(argc, argv, "htTsk:c:"))) {
        switch (o) {
            case 'h':
                usage(argv[0]);
				return EXIT_SUCCESS;

			case 't':
				isTerm = 1;
                break;

            case 'T':
				isTerm = 0;
				break;

			case 's':
				swap = TRUE;
                break;

			case 'c':
				cmd = optarg;
				break;

			case 'd':
				delay = atoi(optarg);
				break;

			case 'k':
				keys = optarg;
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
			o = key2char(event.code);
			if (o != '\0' && strchr(keys, o)) {
				/*
					This key needs to be modified

					Send an event to release the super key(s) and then to press
					the desired modifiers before passing through the real key.
				*/
				terminal_state = check_for_terminal();

				if (sent_super_down) {
					if (left_super_held) {
						fake_event(KEY_LEFTMETA, KEY_STROKE_UP);
					}
					if (right_super_held) {
						fake_event(KEY_RIGHTMETA, KEY_STROKE_UP);
					}
				}

				fake_event(KEY_LEFTCTRL, KEY_STROKE_DOWN);
				if (terminal_state) {
					fake_event(KEY_LEFTSHIFT, KEY_STROKE_DOWN);
				}
				write_event(&syn);
				usleep(delay);

				/* Let the original key through */
				write_event(&event);
				usleep(delay);

				did_modify = TRUE;

				/* Undo everything we just did */
				if (sent_super_down) {
					if (left_super_held) {
						fake_event(KEY_LEFTMETA, KEY_STROKE_DOWN);
					}
					if (right_super_held) {
						fake_event(KEY_RIGHTMETA, KEY_STROKE_DOWN);
					}
				}

				fake_event(KEY_LEFTCTRL, KEY_STROKE_UP);
				if (terminal_state) {
					fake_event(KEY_LEFTSHIFT, KEY_STROKE_UP);
				}
				write_event(&syn);
				usleep(delay);

				continue;
			} else {
				/* We held the super down event back, so send it now */
				if (!sent_super_down) {
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
			}
		}

		write_event(&event);
    }
}

