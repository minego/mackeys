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

typedef enum {
	MOD_L_ALT				= 1 << 0,
	MOD_L_CTRL				= 1 << 1,
	MOD_L_META				= 1 << 2,
	MOD_L_SHIFT				= 1 << 3,

	MOD_R_ALT				= 1 << 4,
	MOD_R_CTRL				= 1 << 5,
	MOD_R_META				= 1 << 6,
	MOD_R_SHIFT				= 1 << 7,
} MODS;

typedef struct input_event input_event;

const struct input_event	syn	= { .type = EV_SYN, .code = SYN_REPORT, .value = 0 };

/* Global options */
int delay = 20000;

static void usage(const char *program)
{
    fprintf(stderr,
			"%s - Map macOS copy, paste and cut events to ctrl+insert, shift+insert and shift+delete\n"
			"\n"
			"usage %s [-h | [-d delay]]\n"
			"\n"
			"options:\n"
			"   -h          Show this message and exit\n"
			"   -d          Delay used for key sequences (default: 20000 microseconds)\n"

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

	write_event(&syn);
	usleep(delay);
}

static unsigned char which_mod(input_event *event)
{
	if (event->type != EV_KEY) {
		return 0;
	}

	/* Is this event for a modifier? */
	switch (event->code) {
		case KEY_LEFTALT:		return MOD_L_ALT;
		case KEY_LEFTCTRL:		return MOD_L_CTRL;
		case KEY_LEFTMETA:		return MOD_L_META;
		case KEY_LEFTSHIFT:		return MOD_L_SHIFT;

		case KEY_RIGHTALT:		return MOD_R_ALT;
		case KEY_RIGHTCTRL:		return MOD_R_CTRL;
		case KEY_RIGHTMETA:		return MOD_R_META;
		case KEY_RIGHTSHIFT:	return MOD_R_SHIFT;

		default:				return 0;
	}
}

static int which_key(unsigned char cur)
{
	switch (cur) {
		case MOD_L_ALT:			return KEY_LEFTALT;
		case MOD_L_CTRL:		return KEY_LEFTCTRL;
		case MOD_L_META:		return KEY_LEFTMETA;
		case MOD_L_SHIFT:		return KEY_LEFTSHIFT;

		case MOD_R_ALT:			return KEY_RIGHTALT;
		case MOD_R_CTRL:		return KEY_RIGHTCTRL;
		case MOD_R_META:		return KEY_RIGHTMETA;
		case MOD_R_SHIFT:		return KEY_RIGHTSHIFT;

		default:				return 0;
	}
}

/* Update the bitfield mods to reflect the state after the specified event */
static void update_mods(unsigned char *mods, input_event *event)
{
	unsigned char cur;

	/* Is this event for a modifier? */
	cur = which_mod(event);

	if (cur != 0) {
		switch (event->value) {
			case KEY_STROKE_UP:
				(*mods) &= ~cur;
				break;

			case KEY_STROKE_DOWN:
				(*mods) |= cur;
				break;
		}
	}
}

/*
	Send down and up events to make the current mod state match the desired
	mod state.
*/
static void apply_mods(unsigned char *current_mods, unsigned char desired_mods)
{
	int bit;
	unsigned char cur;

	for (bit = 0; bit < 8; bit++) {
		cur = 1 << bit;

		if (((*current_mods) & cur) == (desired_mods & cur)) {
			/* This mod is already in the needed state */
			continue;
		}

		if ((desired_mods & cur)) {
			fake_event(which_key(cur), KEY_STROKE_DOWN);
			(*current_mods) |= cur;
		} else {
			fake_event(which_key(cur), KEY_STROKE_UP);
			(*current_mods) &= ~cur;
		}
	}
}

int main(int argc, char **argv)
{
	char				o;
	input_event			event;
	unsigned char		cur;
	int					faked		= 0;
	unsigned char		mods_bk		= 0;
	unsigned char		mods_in		= 0;
	unsigned char		mods_out	= 0;

	while (-1 != (o = getopt(argc, argv, "hd:"))) {
        switch (o) {
            case 'h':
                usage(argv[0]);
				return EXIT_SUCCESS;

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

		cur = which_mod(&event);

		/* Ensure that mods_in contains the state as we've read them */
		update_mods(&mods_in, &event);

		switch (event.code) {
			case KEY_LEFTMETA:
			case KEY_RIGHTMETA:
				switch (event.value) {
					case KEY_STROKE_UP:
						/*
							If we didn't send any faked copy/paste events and
							the meta keystroke was eaten then it should be sent
							now to allow taps.
						*/
						mods_bk = mods_in;
						mods_bk |= cur;

						if (faked == 0 && mods_bk != mods_out) {
							apply_mods(&mods_out, mods_bk);
						}

					case KEY_STROKE_REPEAT:
						/*
							If we have sent the down event then let this event
							through. Otherwise eat it.
						*/
						if (mods_out & cur) {
							break;
						} else {
							continue;
						}

					case KEY_STROKE_DOWN:
						/*
							Eat the down event for the meta keys, until we know
							that this is not a meta+x|c|v keystroke.
						*/
						faked = 0;
						continue;
				}
				break;

			case KEY_X:
			case KEY_C:
			case KEY_V:
				if ((mods_in & (MOD_L_META | MOD_R_META)) == 0) {
					/* If a meta key isn't being held then these keys aren't modified */
					break;
				}

				if (event.value != KEY_STROKE_DOWN) {
					/* The event is triggered on the key down only */
					continue;
				}

				/*
					Keep track of how many faked copy/paste events we've applied
					so that we know if the meta key was tapped or not.
				*/
				faked++;

				/* Remember what the state was previously */
				mods_bk = mods_out;

				/* Apply the desired mods for this faked event */
				switch (event.code) {
					case KEY_C:
						apply_mods(&mods_out, MOD_L_CTRL);
						break;

					case KEY_X:
					case KEY_V:
						apply_mods(&mods_out, MOD_L_SHIFT);
						break;
				}

				/* Send the actual key now that the mods are in the right state */
				switch (event.code) {
					case KEY_X:
						fake_event(KEY_DELETE, KEY_STROKE_DOWN);
						fake_event(KEY_DELETE, KEY_STROKE_UP);
						break;

					case KEY_C:
					case KEY_V:
						fake_event(KEY_INSERT, KEY_STROKE_DOWN);
						fake_event(KEY_INSERT, KEY_STROKE_UP);
						break;
				}

				/* Restore the previous modifier state */
				apply_mods(&mods_out, mods_bk);

				/* Eat the original event */
				continue;

			case KEY_LEFTALT:
			case KEY_LEFTCTRL:
			case KEY_LEFTSHIFT:
			case KEY_RIGHTALT:
			case KEY_RIGHTCTRL:
			case KEY_RIGHTSHIFT:
				/*
					All mods other than meta should be sent normally, but should
					not fall into the default case which could send an extra
					meta down event.
				*/
				break;

			default:
				/*
					If a down event for a meta key was previously eaten then
					send it now.
				*/
				if (mods_in != mods_out) {
					apply_mods(&mods_out, mods_in);
				}
				break;
		}


		/* Write the event, and update mods_out to match the state we've written */
		write_event(&event);
		update_mods(&mods_out, &event);
	}
}

