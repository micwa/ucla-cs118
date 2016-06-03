CXX=g++
CXXOPTIMIZE= -O2
CXXFLAGS= -g -Wall -Wpedantic -Wextra -Wshadow -pthread -std=c++11 $(CXXOPTIMIZE)
USERID=904469956, 204458659

# Add all .cpp files that need to be compiled for your server
SERVER_FILES=server.cpp \
			 	simpleTCP.cpp \
				TCPutil.cpp

# Add all .cpp files that need to be compiled for your client
CLIENT_FILES=client.cpp \
			 	simpleTCP.cpp \
				TCPutil.cpp

all: server client

*.o: *.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS) $@.cpp

server: $(SERVER_FILES:.cpp=.o)
	$(CXX) -o $@ $(CXXFLAGS) $(SERVER_FILES:.cpp=.o)

client: $(CLIENT_FILES:.cpp=.o)
	$(CXX) -o $@ $(CXXFLAGS) $(CLIENT_FILES:.cpp=.o)

clean:
	rm -rf *.o *~ *.gch *.swp *.dSYM server client *.tar.gz

tarball: clean
	tar -cvf $(USERID).tar.gz *
