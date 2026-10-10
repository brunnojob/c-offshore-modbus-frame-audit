CC ?= cc
CFLAGS = -std=c17 -Wall -Wextra -Werror -pedantic -O2
all: app
app: src/main.c
	$(CC) $(CFLAGS) $< -lm -o $@
test: app
	./app --self-test
clean:
	rm -f app
