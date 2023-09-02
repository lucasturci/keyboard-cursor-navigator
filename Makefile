all:
	g++ -o main main.cpp -std=c++17 -O3 -levdev -lxdo
build:
	g++ -o /usr/local/bin/keyboard-cursor-navigator main.cpp -std=c++17 -O3 -levdev -lxdo
