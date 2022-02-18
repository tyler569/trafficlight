#include <stdbool.h>
#include <sys/time.h>
#include <SDL.h>
#include <cairo/cairo.h>

int window_w = 640;
int window_h = 480;

bool done = false;

enum lamp_color {
    COLOR_UNSET,
    COLOR_UNKNOWN,
    COLOR_BG,
    COLOR_RED,
    COLOR_AMBER,
    COLOR_GREEN,
    COLOR_WHITE,
    COLOR_OFF,
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
    char state[32];
};

struct lamp_info {
    bool exists;
    enum lamp_color color;
    enum lamp_shape shape;
    enum lamp_symbol symbol;
};

struct light_spec {
    int loop_time;
    int stage_count;
    char name[64];
    struct lamp_info lamps[32];
    struct light_stage stages[32];
};

void render_frame(cairo_t *cr, int frame);
void load_light_specs(void);
void load_draw_instructions(void);
void light(cairo_t *cr, int x, int y, int size, struct light_spec *spec, int stage_id, long time);

long nanosecond_now() {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (long)now.tv_sec * 1000000000 + now.tv_nsec;
}

long millisecond_now() {
    return nanosecond_now() / 1000000;
}

long max(long a, long b) {
    return a > b ? a : b;
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) {
    load_light_specs();
    load_draw_instructions();

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *window = SDL_CreateWindow(
            "Traffic Light",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            window_w,
            window_h,
            SDL_WINDOW_SHOWN
    );

    SDL_Renderer *renderer = SDL_CreateRenderer(
            window,
            -1,
            SDL_RENDERER_ACCELERATED
    );

    int window_width;
    int window_height;
    SDL_GetWindowSize(window, &window_width, &window_height);

    printf("window is %ix%i\n", window_width, window_height);

    int renderer_width;
    int renderer_height;
    SDL_GetRendererOutputSize(renderer, &renderer_width, &renderer_height);

    printf("renderer is %ix%i\n", renderer_width, renderer_height);

    SDL_Surface *sdl_surface = SDL_CreateRGBSurface(
            0,
            renderer_width,
            renderer_height,
            32,
            0x00FF0000,
            0x0000FF00,
            0x000000FF,
            0
    );

    printf("surface is %ix%i, pitch %i\n",
            sdl_surface->w, sdl_surface->h, sdl_surface->pitch);

    cairo_surface_t *cairo_surface = cairo_image_surface_create_for_data(
            (unsigned char *)sdl_surface->pixels,
            CAIRO_FORMAT_RGB24,
            sdl_surface->w,
            sdl_surface->h,
            sdl_surface->pitch
    );

    cairo_t *cr = cairo_create(cairo_surface);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    SDL_Texture *texture;


    int frame = 0;
    long frame_start_ms;
    int desired_frame_rate = 60;
    long desired_frame_duration = 1000 / desired_frame_rate;
    bool rctrl = false, lctrl = false;
    while (!done) {
        frame_start_ms = millisecond_now();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                done = true;
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                case SDLK_RCTRL:
                    rctrl = true;
                    break;
                case SDLK_LCTRL:
                    lctrl = true;
                    break;
                case SDLK_c:
                    if (rctrl || lctrl)
                        done = true;
                    break;
                }
                break;
            case SDL_KEYUP:
                switch (event.key.keysym.sym) {
                case SDLK_RCTRL:
                    rctrl = false;
                    break;
                case SDLK_LCTRL:
                    lctrl = false;
                    break;
                }
                break;
            }
        }

        render_frame(cr, frame);

        texture = SDL_CreateTextureFromSurface(renderer, sdl_surface);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        SDL_DestroyTexture(texture);

        long frame_duration = millisecond_now() - frame_start_ms;

        frame++;
        SDL_Delay(max(desired_frame_duration - frame_duration, 0));
    }

    SDL_FreeSurface(sdl_surface);

    cairo_destroy(cr);
    cairo_surface_destroy(cairo_surface);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

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
    cairo_stroke(cr);

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
    cairo_move_to(cr, -1 * 0.25, -1 * 0.25);
    cairo_line_to(cr, 1 * 0.25, 1 * 0.25);
    cairo_move_to(cr, 1 * 0.25, -1 * 0.25);
    cairo_line_to(cr, -1 * 0.25, 1 * 0.25);
    cairo_stroke(cr);
}

void lamp_square(cairo_t *cr) {
    cairo_rectangle(cr, -0.25, -0.25, 0.5, 0.5);
    cairo_fill(cr);
}

void lamp(
        cairo_t *cr,
        int x,
        int y,
        int size,
        enum lamp_color color,
        bool on,
        enum lamp_shape shape,
        enum lamp_symbol symbol
) {
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
        cairo_rotate(cr, M_PI/4);
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
        cairo_rotate(cr, M_PI/2);
        lamp_left_arrow(cr, arrow_width);
        break;
    case LAMP_Y_HORIZ:
        set_color_o(cr, color, on);
        lamp_bar(cr, bar_width);
        break;
    case LAMP_Y_VERT:
        set_color_o(cr, color, on);
        cairo_rotate(cr, M_PI/2);
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

void light(
        cairo_t *cr,
        int x,
        int y,
        int size,
        struct light_spec *spec,
        int stage_id,
        long time
) {
    int doghouse_top = -1;

    enum lamp_shape shape = LAMP_S_CIRCLE;
    enum lamp_symbol symbol = LAMP_Y_NONE;

    int lamp_id = 0;
    int n = 0;
    int original_x = x;
    int next_flash = 0;
    bool next_large = false;
    int large_count = 0;
    const char *colors = spec->stages[stage_id].state;

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
            printf("WARNING: '%c' is not a known lamp color or modifier\n", *c);
        }

        int this_size = size;
        int this_x = x;
        int y_offset = size * n + large_count * size / 2;
        if (next_large) {
            this_size = size * 3 / 2;
            this_x = x - size / 4;
            large_count++;
        }

        if (next_flash == 1 && (time / 1000) % 2 == 0) color = COLOR_OFF;
        if (next_flash == 2 && (time / 1000) % 2 == 1) color = COLOR_OFF;

        bool on = true;

        if (spec->lamps[lamp_id].exists) {
            if (color == COLOR_OFF) {
                color = spec->lamps[lamp_id].color;
                shape = spec->lamps[lamp_id].shape;
                symbol = spec->lamps[lamp_id].symbol;
                on = false;
            }
        }

        lamp(cr, this_x, y + y_offset, this_size, color, on, shape, symbol);

        next_large = false;
        next_flash = 0;
        shape = LAMP_S_CIRCLE;
        symbol = LAMP_Y_NONE;
        lamp_id++;
        n++;
    }
}



void print_light_spec(struct light_spec *spec) {
    printf("light_spec {\n");
    printf("\t.loop_time = %i,\n", spec->loop_time);
    printf("\t.stage_count = %i,\n", spec->stage_count);
    printf("\t.name = \"%s\",\n", spec->name);

    printf("\t.lamps = {\n");
    for (struct lamp_info *lamp = spec->lamps; lamp->exists; lamp++) {
        printf("\t\t{ .color = %i, .shape = %i, .symbol = %i },\n",
                lamp->color, lamp->shape, lamp->symbol);
    }
    printf("\t},\n");

    printf("\t.stages = {\n");
    for (struct light_stage *stage = spec->stages; stage->state[0]; stage++) {
        printf("\t\t{ .time_offset = %i, .state = \"%s\" },\n",
                stage->time_offset, stage->state);
    }
    printf("\t},\n");
    printf("}\n");
}


struct light_stage *stage(struct light_spec *spec, int offset, long time) {
    long loop_time = (time + offset) % spec->loop_time;
    for (struct light_stage *stage = spec->stages; stage->state[0]; stage++) {
        if (
                loop_time >= stage[0].time_offset &&
                ( loop_time < stage[1].time_offset ||
                  stage[1].time_offset == 0 )
        ) {
            return stage;
        }
    }
    printf("WARNING: no valid state found for light at time %i\n", offset);
    print_light_spec(spec);
    return &(struct light_stage){ .state = "" };
}

int stage_id(struct light_spec *spec, int offset, long time) {
    long loop_time = (time + offset) % spec->loop_time;
    for (struct light_stage *stage = spec->stages; stage->state[0]; stage++) {
        if (
                loop_time >= stage[0].time_offset &&
                ( loop_time < stage[1].time_offset ||
                  stage[1].time_offset == 0 )
        ) {
            return stage - spec->stages;
        }
    }
    printf("WARNING: no valid state found for light at time %i\n", offset);
    print_light_spec(spec);
    return -1;
}

void draw_light_spec(
        cairo_t *cr,
        int x,
        int y,
        int size,
        struct light_spec *spec,
        int offset,
        long ms
    ) {
    int current_stage = stage_id(spec, offset, ms/1000);
    light(cr, x, y, size, spec, current_stage, ms);
}

struct draw_instruction {
    int light_id;
    int x, y;
    int size;
    int offset;
};


struct light_spec *spec_array;
int spec_count = 0;
struct draw_instruction *instruction_array;
int instruction_count = 0;

void render_frame(cairo_t *cr, [[maybe_unused]] int frame) {
    cairo_set_source_rgba(cr, 0.5, 0.5, 0.5, 1.0);
    cairo_rectangle(cr, 0, 0, window_w, window_h);
    cairo_fill(cr);

    long ms = millisecond_now();

    for (int i = 0; i < instruction_count; i++) {
        struct draw_instruction *instr = &instruction_array[i];
        struct light_spec *spec = spec_array + instr->light_id;
        draw_light_spec(
                cr,
                instr->x,
                instr->y,
                instr->size,
                spec,
                instr->offset,
                ms
        );
    }
}


void fill_lamps(struct light_spec *spec) {
    struct lamp_info *lamps = spec->lamps;

    for (struct light_stage *stage = spec->stages; stage->state[0]; stage++) {
        enum lamp_color color = COLOR_OFF;
        enum lamp_shape shape = LAMP_S_CIRCLE;
        enum lamp_symbol symbol = LAMP_Y_NONE;

        int lamp_id = 0;

        for (const char *c = stage->state; *c; c++) {
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
            default:
                continue;
            }

            if (color == COLOR_OFF) {
                goto next;
            }

            if (!lamps[lamp_id].exists) {
                lamps[lamp_id].color = color;
                lamps[lamp_id].shape = shape;
                lamps[lamp_id].symbol = symbol;
                lamps[lamp_id].exists = true;
            } else {
                if (lamps[lamp_id].color != color) {
                    lamps[lamp_id].color = COLOR_WHITE;
                }
                if (lamps[lamp_id].symbol != symbol) {
                    lamps[lamp_id].symbol = LAMP_Y_NONE;
                }
                if (lamps[lamp_id].shape != shape) {
                    printf("WARNING: lamp %i in '%s' changes shape!\n",
                            lamp_id, spec->name);
                }
            }

next:
            color = COLOR_OFF;
            shape = LAMP_S_CIRCLE;
            symbol = LAMP_Y_NONE;
            lamp_id++;
        }
    }
}


void load_light_specs() {
    int stage_id = 0;
    char a[256], b[256], *end;
    struct light_spec *spec;
    FILE *file = fopen("lightspec", "r");

    while (!feof(file)) {
        fscanf(file, "%s %s", a, b);
        strtol(a, &end, 10);
        if (*end) {
            spec_count++;
        }
    }

    spec_array = calloc(spec_count, sizeof(struct light_spec));
    spec = &spec_array[-1];
    rewind(file);

    while (!feof(file)) {
        fscanf(file, "%s %s", a, b);
        int time = strtol(a, &end, 10);
        if (*end) {
            spec++;
            strcpy(spec->name, a);
            spec->loop_time = strtol(b, NULL, 10);
            spec->stage_count = stage_id;
            stage_id = 0;
        } else {
            spec->stages[stage_id].time_offset = time;
            strcpy(spec->stages[stage_id].state, b);
            stage_id++;
        }
    }

    for (int i = 0; i < spec_count; i++) {
        fill_lamps(&spec_array[i]);
        print_light_spec(&spec_array[i]);
    }
}

int by_name(const char *name) {
    for (int i = 0; i < spec_count; i++) {
        if (strcmp(name, spec_array[i].name) == 0) {
            return i;
        }
    }
    printf("ERROR: No light specification exists with name '%s'\n", name);
    exit(1);
}

void load_draw_instructions() {
    FILE *file = fopen("lightscene", "r");
    char light_name[64], line[256];
    int x, y, size, offset;
    int last_x, last_size = 0;
    int i = 0;

    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#') continue;
        if (line[0] == '\n') continue;
        instruction_count++;
    }

    instruction_array = calloc(instruction_count, sizeof(struct draw_instruction));

    rewind(file);
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#') continue;
        if (line[0] == '\n') continue;
        if (sscanf(line, "%s %i %i %i %i", light_name, &x, &y, &size, &offset) != 5) {
            printf("line '%s' is not in a correct format\n", line);
        }
        if (x == -1) {
            x = last_x + last_size * 2;
        } else if (x == -2) {
            x = window_w / 2 - size / 2;
        }
        instruction_array[i].light_id = by_name(light_name);
        instruction_array[i].x = x;
        instruction_array[i].y = y;
        instruction_array[i].size = size;
        instruction_array[i].offset = offset;
        i++;
        last_x = x;
        last_size = size;
    }

    printf("instructions = {\n");
    for (int i = 0; i < instruction_count; i++) {
        struct draw_instruction *instr = &instruction_array[i];
        printf("\t{ .light_id = %i, .x = %i, .y = %i, .size = %i, .offset = %i },\n",
                instr->light_id, instr->x, instr->y, instr->size, instr->offset);
    }
    printf("}\n");
}
