CFLAGS := \
	$(shell sdl2-config --cflags) \
	$(shell pkg-config cairo --cflags) \
	-Wall \
	-Wextra \
	-Wpedantic \
	-std=c2x \
	-fsanitize=address

LDFLAGS := \
	$(shell sdl2-config --libs) \
	$(shell pkg-config cairo --libs)

all: trafficlight
