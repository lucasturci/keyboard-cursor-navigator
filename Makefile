all:
	g++ -o main main.cpp -std=c++17 -O3 -levdev -lxdo -g
build:
	g++ -o /usr/local/bin/keyboard-cursor-navigator main.cpp -std=c++17 -O2 -levdev -lxdo -g
