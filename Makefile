trafficlight: trafficlight.c
	clang -std=c2x -Wall -Wextra trafficlight.c -o trafficlight -lsdl2 -lcairo
