CFLAGS := -std=c++20
LDFLAGS := $(shell pkg-config --libs libnetfilter_queue) -lpthread

all: build/ build/main
	./script/setrule
	-sudo build/main
	./script/reset

build/main: build/main.o build/NFQueue.o
	c++ -o $@ $^ $(LDFLAGS)

build/main.o: main.cpp NFQueue.h
	c++ -o $@ -c $< $(CFLAGS) 

build/NFQueue.o: NFQueue.cpp NFQueue.h
	c++ -o $@ -c $< $(CFLAGS) 

build/:
	-mkdir build/
	echo "*" > build/.gitignore
