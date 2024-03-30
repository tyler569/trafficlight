#pragma once

#include <cairo.h>
#include "trafficlight.h"

void light(cairo_t *cr, int x, int y, int size, struct light_spec *spec, int stage_id, long time);
