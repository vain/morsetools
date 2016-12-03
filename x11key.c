#include <ao/ao.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>

enum Buttons
{
    BtnNone = 0,
    BtnFast,
    BtnSlow,
};

int wpm = 20;
int hz = 900;
int sampling_rate = 44100;
double fade_length = 0.004;
char *buffer = NULL;
size_t buffer_size = 0;

bool
audio_init(ao_device **device)
{
    ao_sample_format format = {0};
    int default_driver;

    ao_initialize();
    default_driver = ao_default_driver_id();
    format.bits = 16;
    format.channels = 1;
    format.rate = sampling_rate;
    format.byte_format = AO_FMT_LITTLE;
    *device = ao_open_live(default_driver, &format, NULL);
    if (*device == NULL)
    {
        fprintf(stderr, "Cannot open sound device.\n");
        return false;
    }
    return true;
}

void
audio_deinit(ao_device *device)
{
    ao_close(device);
    ao_shutdown();
}

void
play(ao_device *device, int dits, double amp)
{
    /* This is a version of raw_sine() from t2m.c that writes to a
     * buffer and then plays it directly. */

    size_t i, num_samples, buffer_size_needed;
    double len_dit, val, fade_in, fade_out;
    size_t at = 0;

    /* WPM to "len of one dit": One "word" is 50 dits. */
    len_dit = 60.0 / (wpm * 50);

    num_samples = dits * len_dit * sampling_rate;
    buffer_size_needed = num_samples * 2 * sizeof (char);
    if (buffer_size_needed > buffer_size)
    {
        buffer = realloc(buffer, buffer_size_needed);
        if (buffer == NULL)
        {
            fprintf(stderr, "Cannot allocate buffer in play()!\n");
            return;
        }
        buffer_size = buffer_size_needed;
    }

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

        /* Convert to S16_LE, add to buffer. */
        int ival = (int)(((1 << 15) - 1) * val);
        buffer[at++] = ival & 0xFF;
        buffer[at++] = (ival >> 8) & 0xFF;
    }

    ao_play(device, buffer, buffer_size_needed);
}

int
main(int argc, char **argv)
{
    bool running = true;
    int opt;
    Display *dpy;
    Window root, win;
    int screen;
    XEvent ev;
    XClientMessageEvent *cm;
    Atom wmdelete, wmprotocols;
    XButtonEvent *be;
    ao_device *audio_device;
    bool button_fast_down = false, button_slow_down = false;
    enum Buttons current_button = BtnNone;

    while ((opt = getopt(argc, argv, "f:F:w:")) != -1)
    {
        switch (opt)
        {
            case 'f':
                hz = atoi(optarg);
                break;
            case 'F':
                fade_length = atof(optarg);
                break;
            case 'w':
                wpm = atoi(optarg);
                break;
        }
    }

    dpy = XOpenDisplay(NULL);
    if (!dpy)
    {
        fprintf(stderr, "Cannot open display\n");
        exit(EXIT_FAILURE);
    }

    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);

    win = XCreateSimpleWindow(dpy, root, 0, 0, 100, 100, 0, 0, 0);
    wmdelete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    wmprotocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
    XSetWMProtocols(dpy, win, &wmdelete, 1);
    XSelectInput(dpy, win, ButtonPressMask | ButtonReleaseMask);
    XMapRaised(dpy, win);

    if (!audio_init(&audio_device))
        exit(EXIT_FAILURE);

    while (running)
    {
        while (XPending(dpy))
        {
            XNextEvent(dpy, &ev);
            /* Keep track of the state of both buttons ("pressed",
             * "unpressed"). 'current_button' indicates which button was
             * pressed last. If you press both buttons at the same time
             * and then release one of them, 'current_button' will point
             * to the button which was pressed first (and is still
             * pressed). */
            if (ev.type == ButtonPress)
            {
                be = &ev.xbutton;
                if (be->button == Button1)
                {
                    button_fast_down = true;
                    current_button = BtnFast;
                }
                else
                {
                    button_slow_down = true;
                    current_button = BtnSlow;
                }

                /* Okay, a new button has been pressed. We now exit the
                 * inner while(XPending(dpy)) in order to force playing
                 * a sample. This avoids situations where we read a
                 * "ButtonPress" immediately followed by a
                 * "ButtonRelease". */
                break;
            }
            else if (ev.type == ButtonRelease)
            {
                be = &ev.xbutton;
                if (be->button == Button1)
                {
                    button_fast_down = false;
                    if (button_slow_down)
                        current_button = BtnSlow;
                    else
                        current_button = BtnNone;
                }
                else
                {
                    button_slow_down = false;
                    if (button_fast_down)
                        current_button = BtnFast;
                    else
                        current_button = BtnNone;
                }
            }
            else if (ev.type == ClientMessage)
            {
                cm = &ev.xclient;
                if (cm->message_type == wmprotocols &&
                    (Atom)cm->data.l[0] == wmdelete)
                    running = false;
            }
        }

        switch (current_button)
        {
            case BtnFast:
                play(audio_device, 1, 1);
                play(audio_device, 1, 0);
                break;
            case BtnSlow:
                play(audio_device, 3, 1);
                play(audio_device, 1, 0);
                break;
            case BtnNone:
                play(audio_device, 1, 0);
                break;
        }
    }

    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);

    audio_deinit(audio_device);

    if (buffer)
        free(buffer);

    return 0;
}
