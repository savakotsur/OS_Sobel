CPPFLAGS = -Wall -Werror -Wextra -std=c++17

all:
	g++ $(CPPFLAGS) sobel.cpp

clean:
	rm -rf *.out