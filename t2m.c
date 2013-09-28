#include <stdio.h>
#include <ctype.h>
#include <math.h>

/* Compile and run as:
 *   echo 'mkmmk mmkkmm mkm m k' | ./t2m | aplay -r 44100 -f S16_LE
 */

#define LENGTH(X) (sizeof X / sizeof X[0])

#define WPM 20
#define HZ 1500
#define SAMPLING_RATE 44100
#define FADE_LENGTH 0.001

struct mapping
{
	int from;
	char *to;
};

struct mapping trans[] = {
	{ 'a', ". -   " },
	{ 'b', "- . . .   " },
	{ 'c', "- . - .   " },
	{ 'd', "- . .   " },
	{ 'e', ".   " },
	{ 'f', ". . - .   " },
	{ 'g', "- - .   " },
	{ 'h', ". . . .   " },
	{ 'i', ". .   " },
	{ 'j', ". - - -   " },
	{ 'k', "- . -   " },
	{ 'l', ". - . .   " },
	{ 'm', "- -   " },
	{ 'n', "- .   " },
	{ 'o', "- - -   " },
	{ 'p', ". - - .   " },
	{ 'q', "- - . -   " },
	{ 'r', ". - .   " },
	{ 's', ". . .   " },
	{ 't', "-   " },
	{ 'u', ". . -   " },
	{ 'v', ". . . -   " },
	{ 'w', ". - -   " },
	{ 'x', "- . . -   " },
	{ 'y', "- . - -   " },
	{ 'z', "- - . .   " },

	{ '0', "- - - - -   " },
	{ '1', ". - - - -   " },
	{ '2', ". . - - -   " },
	{ '3', ". . . - -   " },
	{ '4', ". . . . -   " },
	{ '5', ". . . . .   " },
	{ '6', "- . . . .   " },
	{ '7', "- - . . .   " },
	{ '8', "- - - . .   " },
	{ '9', "- - - - .   " },

	/* Disabled because UTF-8: */
	/* { 'à', ". - - . -   " }, */
	/* { 'å', ". - - . -   " }, */
	/* { 'ä', ". - . -   " }, */
	/* { 'è', ". - . . -   " }, */
	/* { 'é', ". . - . .   " }, */
	/* { 'ö', "- - - .   " }, */
	/* { 'ü', ". . - -   " }, */
	/* { 'ß', ". . . - - . .   " }, */
	/* { 'ñ', "- - . - -   " }, */
	{ '.', ". - . - . -   " },
	{ ',', "- - . . - -   " },
	{ ':', "- - - . . .   " },
	{ ';', "- . - . - .   " },
	{ '?', ". . - - . .   " },
	{ '-', "- . . . . -   " },
	{ '_', ". . - - . -   " },
	{ '(', "- . - - .   " },
	{ ')', "- . - - . -   " },
	{ '\'', ". - - - - .   " },
	{ '=', "- . . . -   " },
	{ '+', ". - . - .   " },
	{ '/', "- . . - .   " },
	{ '@', ". - - . - .   " },

	/* Disabled because multi-char glyph: */
	/* { 'CH', "- - - -   " }, */
};

char *
translate(int c)
{
	size_t i;
	for (i = 0; i < LENGTH(trans); i++)
		if (c == trans[i].from)
			return trans[i].to;

	/* All characters end with three dits. Let's assume a space does not
	 * appear at the beginning of a transmission (where it is useless).
	 * Then, after those three dits, we need another four to get the
	 * seven dits for a space. */
	return "    ";
}

void
raw_sine(int dits, double amp)
{
	size_t i, num_samples;
	double len_dit, val, fade_in, fade_out;

	/* WPM to "len of one dit": One "word" is 50 dits. */
	len_dit = 60.0 / (WPM * 50);

	num_samples = dits * len_dit * SAMPLING_RATE;
	for (i = 0; i < num_samples; i++)
	{
		/* The raw sine curve. */
		val = sin((double)i / SAMPLING_RATE * HZ * 2 * M_PI) * amp;

		/* Fade in at the beginning and fade out at the end. */
		fade_in = i / (SAMPLING_RATE * FADE_LENGTH);
		fade_in = fade_in > 1 ? 1 : fade_in;
		fade_out = (num_samples - i) / (SAMPLING_RATE * FADE_LENGTH);
		fade_out = fade_out > 1 ? 1 : fade_out;

		val *= fade_in;
		val *= fade_out;

		/* Print it, S16_LE. */
		int ival = (int)(((1 << 15) - 1) * val);
		printf("%c", ival & 0xFF);
		printf("%c", (ival >> 8) & 0xFF);
	}
}

void
beep(char *str)
{
	char *p;
	for (p = str; *p; p++)
	{
		switch (*p)
		{
			case '.':
				raw_sine(1, 1);
				break;

			case '-':
				raw_sine(3, 1);
				break;

			case ' ':
				raw_sine(1, 0);
				break;
		}
	}
}

int
main()
{
	int c;
	while ((c = getchar()) != EOF)
		beep(translate(tolower(c)));
	return 0;
}
