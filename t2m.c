#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>
#include <stdlib.h>

#define LENGTH(X) (sizeof X / sizeof X[0])

int wpm = 20;
int hz = 1500;
int sampling_rate = 44100;
double fade_length = 0.001;

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
	len_dit = 60.0 / (wpm * 50);

	num_samples = dits * len_dit * sampling_rate;
	for (i = 0; i < num_samples; i++)
	{
		/* The raw sine curve. */
		val = sin((double)i / sampling_rate * hz * 2 * M_PI) * amp;

		/* Fade in at the beginning and fade out at the end. */
		fade_in = i / (sampling_rate * fade_length);
		fade_in = fade_in > 1 ? 1 : fade_in;
		fade_out = (num_samples - i) / (sampling_rate * fade_length);
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
main(int argc, char **argv)
{
	int c, opt;

	while ((opt = getopt(argc, argv, "w:h:r:f:")) != -1)
	{
		switch (opt)
		{
			case 'w':
				wpm = atoi(optarg);
				break;
			case 'h':
				hz = atoi(optarg);
				break;
			case 'r':
				sampling_rate = atoi(optarg);
				break;
			case 'f':
				fade_length = atof(optarg);
				break;
			default: /* '?' */
				fprintf(stderr, "Usage: %s [-w WPM] [-h HZ] "
				                "[-r SAMPLING_RATE] [-f FADE_LENGTH]\n",
				        argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	while ((c = getchar()) != EOF)
		beep(translate(tolower(c)));
	exit(EXIT_SUCCESS);
}
