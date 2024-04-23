#include "draw.h"
#include "trafficlight.h"
#include <cairo.h>
#include <math.h>
#include <stdio.h>

struct saved_color {
	double r, g, b;
};

struct saved_color colors[] = {
	[COLOR_UNSET] = { 1, 0, 1 },
	[COLOR_UNKNOWN] = { 1, 0, 1 },

	[COLOR_BG] = { 0, 0, 0 },
	[COLOR_RED] = { 1, 0, 0 },
	[COLOR_AMBER] = { 1, 0.8, 0 },
	[COLOR_GREEN] = { 0, 1, 0.8 },
	[COLOR_WHITE] = { 1, 1, 1 },
	[COLOR_OFF] = { 0.1, 0.1, 0.1 },
};

void set_color_o(cairo_t *cr, enum lamp_color color, bool on) {
	if (color == COLOR_OFF) {
		// don't make off darker
		on = true;
	}

	struct saved_color c = colors[color];
	if (on) {
		cairo_set_source_rgb(cr, c.r, c.g, c.b);
	} else {
		cairo_set_source_rgb(cr, c.r * 0.2, c.g * 0.2, c.b * 0.2);
	}
}

void set_color(cairo_t *cr, enum lamp_color color) {
	set_color_o(cr, color, true);
}

void margin(cairo_t *cr, int x, int y, int size) {
	int margin = size * 0.1;
	size += margin * 2;
	cairo_rectangle(cr, x - margin, y - margin, size, size);
	set_color(cr, COLOR_AMBER);
	cairo_fill(cr);
}

void lamp_left_arrow(cairo_t *cr, double arrow_width) {
	cairo_set_line_width(cr, arrow_width);
	cairo_move_to(cr, -1 * 0.1, 0);
	cairo_line_to(cr, 1 * 0.3, 0);

	cairo_move_to(cr, 0, -1 * 0.3);
	cairo_line_to(cr, -1 * 0.3, 0);
	cairo_line_to(cr, 0, 1 * 0.3);

	cairo_stroke(cr);
}

void lamp_bar(cairo_t *cr, double bar_width) {
	cairo_set_line_width(cr, bar_width);
	cairo_move_to(cr, -1 * 0.3, 0);
	cairo_line_to(cr, 1 * 0.3, 0);
	cairo_stroke(cr);
}

void lamp_x(cairo_t *cr, double x_width) {
	cairo_set_line_width(cr, x_width);
	cairo_move_to(cr, -1 * 0.22, -1 * 0.22);
	cairo_line_to(cr, 1 * 0.22, 1 * 0.22);
	cairo_move_to(cr, 1 * 0.22, -1 * 0.22);
	cairo_line_to(cr, -1 * 0.22, 1 * 0.22);
	cairo_stroke(cr);
}

void lamp_square(cairo_t *cr) {
	cairo_rectangle(cr, -0.22, -0.22, 0.5, 0.5);
	cairo_fill(cr);
}

void error_once(int c) {
	static char shown[226] = { 0 };
	if (!shown[c]) {
		fprintf(
			stderr, "WARNING: '%c' is not a known lamp color or modifier\n", c);
		shown[c] = 1;
	}
}

void lamp(cairo_t *cr, int x, int y, int size, enum lamp_color color, bool on,
	enum lamp_shape shape, enum lamp_symbol symbol) {
	enum lamp_color fill_color = color;
	double arrow_width = 1.0 / 15.0;
	double bar_width = 1.0 / 8.0;
	double x_width = 1.0 / 10.0;

	cairo_rectangle(cr, x, y, size, size);
	set_color(cr, COLOR_BG);
	cairo_fill(cr);

	cairo_save(cr);
	cairo_translate(cr, x + size / 2, y + size / 2);
	cairo_scale(cr, size, size);

	if (symbol != LAMP_Y_NONE) {
		fill_color = COLOR_OFF;
	}

	switch (shape) {
	case LAMP_S_CIRCLE:
		set_color_o(cr, fill_color, on);
		cairo_arc(cr, 0, 0, 1 * 0.4, 0, 2 * M_PI);
		cairo_fill(cr);
		break;
	case LAMP_S_SQUARE:
		set_color_o(cr, fill_color, on);
		cairo_rectangle(cr, -1 * 0.4, -1 * 0.4, 1 * 0.8, 1 * 0.8);
		cairo_fill(cr);
		break;
	case LAMP_S_DIAMOND:
		set_color_o(cr, fill_color, on);
		cairo_rotate(cr, M_PI / 4);
		cairo_rectangle(cr, -1 * 0.3, -1 * 0.3, 1 * 0.6, 1 * 0.6);
		cairo_fill(cr);
		break;
	case LAMP_S_TRIANGLE:
		// TODO
		break;
	}

	switch (symbol) {
	case LAMP_Y_NONE:
		break;
	case LAMP_Y_LARROW:
		set_color_o(cr, color, on);
		lamp_left_arrow(cr, arrow_width);
		break;
	case LAMP_Y_RARROW:
		set_color_o(cr, color, on);
		cairo_rotate(cr, M_PI);
		lamp_left_arrow(cr, arrow_width);
		break;
	case LAMP_Y_FARROW:
		set_color_o(cr, color, on);
		cairo_rotate(cr, M_PI / 2);
		lamp_left_arrow(cr, arrow_width);
		break;
	case LAMP_Y_HORIZ:
		set_color_o(cr, color, on);
		lamp_bar(cr, bar_width);
		break;
	case LAMP_Y_VERT:
		set_color_o(cr, color, on);
		cairo_rotate(cr, M_PI / 2);
		lamp_bar(cr, bar_width);
		break;
	case LAMP_Y_X:
		set_color_o(cr, color, on);
		lamp_x(cr, x_width);
		break;
	case LAMP_Y_SQUARE:
		set_color_o(cr, color, on);
		lamp_square(cr);
		break;
	}
	cairo_restore(cr);
}

void light(cairo_t *cr, int x, int y, int size, struct light_spec *spec,
	int stage_id, long time) {
	int doghouse_top = -1;

	enum lamp_shape shape = LAMP_S_CIRCLE;
	enum lamp_symbol symbol = LAMP_Y_NONE;

	int lamp_id = 0;
	int n = 0;
	int original_x = x;
	int next_flash = 0;
	bool next_large = false;
	int large_count = 0;
	const char *colors = spec->layout;
	const char *states = spec->stages[stage_id].state;
	int state_i = 0;

	cairo_save(cr);

	if (spec->rtl) {
		cairo_translate(cr, x + size / 2, y + size / 2);
		cairo_rotate(cr, 3 * M_PI / 2);
		cairo_translate(cr, -(x + size / 2), -(y + size / 2));
	}

	for (const char *c = colors; *c; c++) {
		switch (*c) {
		case ':':
			if (doghouse_top >= 0)
				n = doghouse_top;
			else
				doghouse_top = n;
			x = original_x - size / 2;
			continue;
		case ';':
			if (doghouse_top >= 0)
				n = doghouse_top;
			else
				doghouse_top = n;
			x = original_x + size / 2;
			continue;
		case '.':
			x = original_x;
			doghouse_top = 0;
			continue;
		case 'l':
			next_large = true;
			continue;
		case '<':
		case '>':
		case '^':
		case '-':
		case '|':
		case 'x':
		case 's':
		case 'd':
		case 'f':
		case 'F':
			continue;
		default:
			break;
		}
		int this_size = size;
		int this_x = x;
		int y_offset = size * n + large_count * size / 2;
		if (next_large) {
			this_size = size * 3 / 2;
			this_x = x - size / 4;
			large_count++;
		}

		margin(cr, this_x, y + y_offset, this_size);
		next_large = false;
		n++;
	}
	large_count = 0;
	doghouse_top = -1;
	n = 0;
	x = original_x;
	for (const char *c = colors; *c; c++) {
		int color = 0;
		switch (*c) {
		case 'r':
			color = COLOR_RED;
			break;
		case 'y':
		case 'a':
			color = COLOR_AMBER;
			break;
		case 'g':
			color = COLOR_GREEN;
			break;
		case 'w':
			color = COLOR_WHITE;
			break;
		case '_':
			color = COLOR_OFF;
			break;
		case '#':
			color = COLOR_DONT_PRINT;
			break;
		case ':':
			if (doghouse_top >= 0)
				n = doghouse_top;
			else
				doghouse_top = n;
			x = original_x - size / 2;
			continue;
		case ';':
			if (doghouse_top >= 0)
				n = doghouse_top;
			else
				doghouse_top = n;
			x = original_x + size / 2;
			continue;
		case '.':
			doghouse_top = -1;
			x = original_x;
			continue;
		case 'l':
			next_large = true;
			continue;
		case '<':
			symbol = LAMP_Y_LARROW;
			continue;
		case '>':
			symbol = LAMP_Y_RARROW;
			continue;
		case '^':
			symbol = LAMP_Y_FARROW;
			continue;
		case '-':
			symbol = LAMP_Y_HORIZ;
			continue;
		case '|':
			symbol = LAMP_Y_VERT;
			continue;
		case 'x':
			symbol = LAMP_Y_X;
			continue;
		case 's':
			shape = LAMP_S_SQUARE;
			continue;
		case 'd':
			shape = LAMP_S_DIAMOND;
			continue;
		case 'f':
			next_flash = 1;
			continue;
		case 'F':
			next_flash = 2;
			continue;
		default:
			error_once(*c);
		}

		int this_size = size;
		int this_x = x;
		int y_offset = size * n + large_count * size / 2;
		if (next_large) {
			this_size = size * 3 / 2;
			this_x = x - size / 4;
			large_count++;
		}

		bool on = true;

		switch (states[state_i]) {
		case 'r':
			color = COLOR_RED;
			break;
		case 'y':
			color = COLOR_AMBER;
			break;
		case 'g':
			color = COLOR_GREEN;
			break;
		case 'w':
			color = COLOR_WHITE;
			break;
		case '_':
			on = false;
			break;
		case '#':
			break;
		case 'f':
			next_flash = 1;
			break;
		case 'F':
			next_flash = 2;
			break;
		}
		state_i++;

		if (next_flash == 1 && (time / 750) % 2 == 0)
			on = false;
		if (next_flash == 2 && (time / 750) % 2 == 1)
			on = false;

		if (color != COLOR_DONT_PRINT) {
			lamp(cr, this_x, y + y_offset, this_size, color, on, shape, symbol);
		}

		next_large = false;
		next_flash = 0;
		shape = LAMP_S_CIRCLE;
		symbol = LAMP_Y_NONE;
		lamp_id++;
		n++;
	}

	cairo_restore(cr);
}
