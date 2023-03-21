# ======================= #
# Quantik solver makefile #
# ======================= #

# compiler variables
CC = gcc
CFLAGS = -pipe -Wall -Wextra -Werror -std=gnu17
CDEBUG = -g -Og
CRELEASE = -DNDEBUG -O2

# other variables
RM = rm -f

# ===== rules =====

vpath %.c quantik

.PHONY: all clean

# create all executables
all: test_quantik demo_weak_solve quantik_solver

test_quantik: CFLAGS += $(CDEBUG)
test_quantik: test_quantik.c quantik.c

quantik_solver: CFLAGS += $(CRELEASE)
quantik_solver: main_solver.c quantik.c
	$(CC) $(CFLAGS) -o $@ $^

demo_weak_solve: CFLAGS += $(CRELEASE)
demo_weak_solve: demo_weak_solve.c quantik.c
	$(CC) $(CFLAGS) -o $@ $^

# clean build directory contents
clean:
	$(RM) test_quantik demo_weak_solve quantik_solver
