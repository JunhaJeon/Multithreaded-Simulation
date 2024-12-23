# Compiler and flags
CC = gcc
CFLAGS = -g -Wall -Wextra -pthread
LDFLAGS = -pthread

# Sources, objects, and executable
SRCS = system.c resource.c manager.c main.c event.c
OBJS = $(SRCS:.c=.o)
EXEC = main

# Default target
all: $(EXEC)

# Rule to build the executable
$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJS) $(LDFLAGS)

# Rule for compiling .c files to .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target
clean:
	rm -f $(OBJS) $(EXEC)

# Phony targets
.PHONY: all clean

