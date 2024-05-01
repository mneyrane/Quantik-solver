# ======================= #
# Quantik solver makefile #
# ======================= #

# compiler variables
CC = gcc
CFLAGS = -pipe -Wall -Wextra -Werror -std=gnu17
CDEBUG = -g -Og
CRELEASE = -DNDEBUG -O3

# other variables
RM = rm -f

# ===== rules =====

vpath %.c quantik

.PHONY: all clean

# create all executables
all: release test_quantik

release: CFLAGS += $(CRELEASE)
release: quantik_solver

debug: CFLAGS += $(CDEBUG)
debug: quantik_solver

test_quantik: CFLAGS += $(CDEBUG)
test_quantik: test_quantik.c quantik.c

quantik_solver: main_solver.c quantik.c
	$(CC) $(CFLAGS) -o $@ $^

# TODO: remove or modify later
demo_longest_stall: CFLAGS += $(CRELEASE)
demo_longest_stall: demo_longest_stall.c quantik.c
	$(CC) $(CFLAGS) -o $@ $^

# clean build directory contents
clean:
	$(RM) test_quantik quantik_solver demo_longest_stall
