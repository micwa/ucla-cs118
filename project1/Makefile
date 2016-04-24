# Compiler and source extension
CC = g++

SOURCES = \
  HttpMessage.cpp \
  FileRequest.cpp \
  FileResponse.cpp \
  WebUtil.cpp
OBJS = $(subst .cpp,.o,$(SOURCES))

CFLAGS = -g -Wall -Wpedantic -Wextra -Wshadow -Wno-sign-compare \
		 	-pthread -std=c++11
TARGET = web-client web-server test
USERID=204458659
.PHONY : all clean

all : $(TARGET)

web-client : $(OBJS) web-client.o
	$(CC) $(CFLAGS) -o $@ $^
web-server : $(OBJS) web-server.o
	$(CC) $(CFLAGS) -o $@ $^
test : $(OBJS) test.o
	$(CC) $(CFLAGS) -o $@ $^

%.o : %.cpp
	$(CC) $(CFLAGS) -o $@ -c $^

clean :
	-rm -rf $(TARGET) *.o *.gch *~ *.swp *.dSYM *.tar.gz

tarball: clean
	tar -cvf $(USERID).tar.gz *
