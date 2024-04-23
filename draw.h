#pragma once

#include "trafficlight.h"
#include <cairo.h>

void light(cairo_t *cr, int x, int y, int size, struct light_spec *spec,
	int stage_id, long time);
