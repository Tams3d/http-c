CC       := clang
CFLAGS   := -std=c11 -Wall -Wextra -Wpedantic -D_WIN32_WINNT=0x0601 -I.
LDLIBS   := -lws2_32 -lmswsock
TARGET   := server
SRCS     := $(wildcard *.c)
OBJS     := $(SRCS:.c=.o)

ifeq ($(OS),Windows_NT)
SHELL    := cmd.exe
EXEEXT   := .exe
RM       := del /Q /F
NULLDEV  := NUL
else
EXEEXT   :=
RM       := rm -f
NULLDEV  := /dev/null
endif

.PHONY: all clean run

all: $(TARGET)$(EXEEXT)

$(TARGET)$(EXEEXT): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	$(TARGET)$(EXEEXT)

clean:
	-$(RM) $(OBJS) $(TARGET)$(EXEEXT) 2>$(NULLDEV)
