CFLAGS += -Wall -Wextra -O3
LDFLAGS += -lm


.PHONY: all clean

all: t2m x11key

t2m: t2m.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

x11key: x11key.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) -pthread -lX11 \
		`pkg-config --cflags --libs ao`

clean:
	rm -fv t2m x11key
