CFLAGS += -Wall -Wextra -O3
LDFLAGS += -lm


.PHONY: clean

t2m: t2m.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -fv t2m
