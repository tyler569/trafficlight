#include <cstring>
#include <sys/time.h>
#include <SDL.h>
#include <cairo.h>

#include "draw.h"

int window_w = 640;
int window_h = 480;

bool done = false;

void render_frame(cairo_t *cr, int frame);
void load_light_specs();
void load_draw_instructions();
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



void print_light_spec(struct light_spec *spec) {
    printf("light_spec {\n");
    printf("\t.loop_time = %i,\n", spec->loop_time);
    printf("\t.stage_count = %i,\n", spec->stage_count);
    printf("\t.name = \"%s\",\n", spec->name);
    printf("\t.rtl = %s,\n", spec->rtl ? "true" : "false");

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


static struct light_stage empty_stage = { .state = "" };

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
    return &empty_stage;
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
            case '#':
                color = COLOR_DONT_PRINT;
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
    char line[256];
    struct light_spec *spec;
    FILE *file = fopen("lightspec", "r");

    while (!feof(file)) {
        fgets(line, 256, file);
        if (line[0] == '\n' || line[0] == '#') {
            continue;
        }
        if (!isdigit(line[0])) {
            spec_count++;
        }
    }

    spec_array = (struct light_spec *)calloc(spec_count, sizeof(struct light_spec));
    spec = &spec_array[-1];
    rewind(file);

    while (!feof(file)) {
        fgets(line, 256, file);
        if (line[0] == '\n' || line[0] == '#') {
            continue;
        }
        if (!isdigit(line[0])) {
            char rtl[64];
            spec++;
            int n = sscanf(line, "%s %i %s", spec->name, &spec->loop_time, rtl);
            if (n > 2 && strcmp(rtl, "rtl") == 0) {
                spec->rtl = true;
            }
            stage_id = 0;
        } else {
            sscanf(
                    line,
                    "%i %s",
                    &spec->stages[stage_id].time_offset,
                    spec->stages[stage_id].state
            );
            stage_id++;
            spec->stage_count++;
        }
    }

    for (int i = 0; i < spec_count; i++) {
        fill_lamps(&spec_array[i]);
        // print_light_spec(&spec_array[i]);
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

    instruction_array = (struct draw_instruction *)calloc(instruction_count, sizeof(struct draw_instruction));

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

    printf("lights in use:\n");
    long light_printed = 0;
    for (int i = 0; i < instruction_count; i++) {
        int light_id = instruction_array[i].light_id;
        if ((light_printed & (1 << light_id)) == 0) {
            printf("id %i: ", light_id);
            print_light_spec(&spec_array[light_id]);
            light_printed |= 1 << light_id;
        }
    }
}
