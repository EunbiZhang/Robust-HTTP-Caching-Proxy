CFLAGS=-O3 -std=c++14 -pthread -fPIC -Wall -Werror

TARGETS=proxy

all: $(TARGETS)
clean:
	rm -f $(TARGETS)

proxy: proxy.cpp proxy.h service.cpp cache.cpp parse.cpp request.cpp response.cpp 
	g++ $(CFLAGS) -g -o $@ $<

