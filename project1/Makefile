# Compiler and source extension
CC = g++
EXT = cpp

OBJS =  HttpMessage.o test.o
CFLAGS = -g -Wall -Wpedantic -Wextra -Wshadow -Wfloat-equal -std=c++11 -Wno-sign-compare $(INCLUDES)
TARGET = test
.PHONY : all clean

all : $(TARGET)

test : $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o : %.$(EXT)
	$(CC) $(CFLAGS) -o $@ -c $^

clean :
	-rm $(TARGET) *.o
