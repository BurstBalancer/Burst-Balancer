CPFLAGS = -g -rdynamic --std=c++17 -O2

bsketch: main.cpp BSketch.h
	g++ -mavx2 main.cpp -o bsketch $(CPFLAGS)

clean: 
	rm bsketch *.csv

