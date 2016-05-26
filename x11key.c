#include <ao/ao.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>

#define SND_SAMPLERATE 44100

static int hz = 900;
static bool down = false;
static bool running = true;

static void *
player()
{
    double val;
    int counter = 0, ival, fade = 0, fade_max = SND_SAMPLERATE / 500.0;
    ao_device *device;
    ao_sample_format format = {0};
    int default_driver;
    char buffer[100];
    size_t at = 0;

    /* Note: "sizeof buffer % 2" must be zero. */

    ao_initialize();
    default_driver = ao_default_driver_id();
    format.bits = 16;
    format.channels = 1;
    format.rate = SND_SAMPLERATE;
    format.byte_format = AO_FMT_LITTLE;
    device = ao_open_live(default_driver, &format, NULL);
    if (device == NULL)
    {
        fprintf(stderr, "Cannot open sound device.\n");
        return NULL;
    }

    while (running)
    {
        /* We always calculate the actual sine wave. When the button is
         * pressed, "fade" slowly increases for each sample until
         * "fade_max" is reached. This avoids waveform artifacts
         * ("click, clack"). Similarly, when the button is released
         * again, "fade" will be decreased. */

        if (down)
            fade++;
        else
            fade--;

        fade = fade > fade_max ? fade_max : fade;
        fade = fade < 0 ? 0 : fade;

        val = sin((double)counter / SND_SAMPLERATE * hz * 2 * M_PI);
        val *= (double)fade / fade_max;

        ival = (int)(((1 << 15) - 1) * val);
        buffer[at++] = ival & 0xFF;
        buffer[at++] = (ival >> 8) & 0xFF;

        if (at == sizeof buffer)
        {
            /* Implicit timing because this call is blocking. */
            ao_play(device, buffer, sizeof buffer);
            at = 0;
        }

        counter++;
        counter %= SND_SAMPLERATE;
    }

    ao_close(device);
    ao_shutdown();

    return NULL;
}

int
main(int argc, char **argv)
{
    int opt;
    Display *dpy;
    Window root, win;
    int screen;
    XEvent ev;
    XClientMessageEvent *cm;
    pthread_t player_thread;
    Atom wmdelete, wmprotocols;

    while ((opt = getopt(argc, argv, "f:")) != -1)
    {
        switch (opt)
        {
            case 'f':
                hz = atoi(optarg);
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

    pthread_create(&player_thread, NULL, player, NULL);

    win = XCreateSimpleWindow(dpy, root, 0, 0, 100, 100, 0, 0, 0);
    wmdelete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    wmprotocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
    XSetWMProtocols(dpy, win, &wmdelete, 1);
    XSelectInput(dpy, win, ButtonPressMask | ButtonReleaseMask);
    XMapRaised(dpy, win);

    while (running && !XNextEvent(dpy, &ev))
    {
        switch (ev.type)
        {
            case ButtonRelease:
                down = false;
                break;
            case ButtonPress:
                down = true;
                break;
            case ClientMessage:
                cm = &ev.xclient;
                if (cm->message_type == wmprotocols &&
                    (Atom)cm->data.l[0] == wmdelete)
                    running = false;
                break;
        }
    }

    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);

    pthread_join(player_thread, NULL);

    return 0;
}
