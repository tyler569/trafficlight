#pragma once

enum lamp_color {
	COLOR_UNSET,
	COLOR_UNKNOWN,
	COLOR_BG,
	COLOR_RED,
	COLOR_AMBER,
	COLOR_GREEN,
	COLOR_WHITE,
	COLOR_OFF,
	COLOR_DONT_PRINT,
};

enum lamp_shape {
	LAMP_S_CIRCLE,
	LAMP_S_SQUARE,
	LAMP_S_DIAMOND,
	LAMP_S_TRIANGLE,
};

enum lamp_symbol {
	LAMP_Y_NONE,
	LAMP_Y_LARROW,
	LAMP_Y_RARROW,
	LAMP_Y_FARROW,
	LAMP_Y_HORIZ,
	LAMP_Y_VERT,
	LAMP_Y_X,
	LAMP_Y_SQUARE,
};

struct light_stage {
	int time_offset;
	const char *state;
};

struct light_spec {
	int loop_time;
	bool rtl;
	const char *layout;
	struct light_stage stages[];
};
