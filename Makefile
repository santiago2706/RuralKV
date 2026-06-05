CC=gcc
CFLAGS=-Wall -Wextra -pthread -std=c11 -O2
INCLUDES=-I./include
SRCS=$(wildcard src/*.c)
OBJS=$(SRCS:.c=.o)
TARGET=ruralkv

# Para soporte Universal (Sockets de Windows requieren enlazar ws2_32)
ifeq ($(OS),Windows_NT)
	TARGET=ruralkv.exe
	RM=del /Q /F
	RM_OBJS=del /Q /F src\*.o
	LDFLAGS=-lws2_32
else
	RM=rm -f
	RM_OBJS=rm -f src/*.o
	LDFLAGS=
endif

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(TARGET) $(OBJS) $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	-$(RM_OBJS)
	-$(RM) $(TARGET)
	-$(RM) ruralkv.log
