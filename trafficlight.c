#include <stdbool.h>
#include <sys/time.h>
#include <SDL2/SDL.h>
#include <cairo/cairo.h>

bool done = false;

void render_frame(cairo_t *cr, int frame);
void load_light_specs(void);
void load_draw_instructions(void);

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
            640,
            480,
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
    long desired_frame_duration = 1000 / 60;
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

enum lamp_color {
    COLOR_BG,
    COLOR_RED,
    COLOR_AMBER,
    COLOR_GREEN,
    COLOR_OFF,
};

enum lamp_type {
    LAMP_FULL,
    LAMP_LARROW,
    LAMP_RARROW,
    LAMP_FARROW,
};


void set_color(cairo_t *cr, enum lamp_color color) {
    switch (color) {
    case COLOR_BG:
        cairo_set_source_rgba(cr, 0, 0, 0, 1);
        break;
    case COLOR_RED:
        cairo_set_source_rgba(cr, 1, 0, 0, 1);
        break;
    case COLOR_AMBER:
        cairo_set_source_rgba(cr, 1, 0.8, 0, 1);
        break;
    case COLOR_GREEN:
        cairo_set_source_rgba(cr, 0, 1, 0.8, 1);
        break;
    case COLOR_OFF:
        cairo_set_source_rgba(cr, 0.1, 0.1, 0.1, 1);
        break;
    default:
        cairo_set_source_rgba(cr, 1, 0, 1, 1);
    }
}

void margin(cairo_t *cr, int x, int y, int size) {
    int margin = size * 0.1;
    size += margin * 2;
    cairo_rectangle(cr, x - margin, y - margin, size, size);
    set_color(cr, COLOR_AMBER);
    cairo_fill(cr);
}

void lamp(cairo_t *cr, int x, int y, int size, enum lamp_color color, enum lamp_type type) {
    cairo_rectangle(cr, x, y, size, size);
    set_color(cr, COLOR_BG);
    cairo_fill(cr);

    switch (type) {
    case LAMP_FULL:
        set_color(cr, color);
        cairo_arc(cr, x + size / 2, y + size / 2, size * 0.4, 0, 2 * M_PI);
        cairo_fill(cr);
        break;
    case LAMP_LARROW: {
        set_color(cr, COLOR_OFF);
        cairo_arc(cr, x + size / 2, y + size / 2, size * 0.4, 0, 2 * M_PI);
        cairo_fill(cr);

        double width = size / 15.0;
        double dw = width / 3;
        cairo_set_line_width(cr, width);
        set_color(cr, color);
        cairo_move_to(cr, x + size * 0.4, y + size / 2);
        cairo_line_to(cr, x + size * 0.8, y + size / 2);
        cairo_stroke(cr);
        cairo_move_to(cr, x + size * 0.2 - dw, y + size / 2 + dw);
        cairo_line_to(cr, x + size * 0.5, y + size * 0.2);
        cairo_stroke(cr);
        cairo_move_to(cr, x + size * 0.2 - dw, y + size / 2 - dw);
        cairo_line_to(cr, x + size * 0.5, y + size * 0.8);
        cairo_stroke(cr);
        break;
    }
    case LAMP_RARROW: {
        set_color(cr, COLOR_OFF);
        cairo_arc(cr, x + size / 2, y + size / 2, size * 0.4, 0, 2 * M_PI);
        cairo_fill(cr);

        double width = size / 15.0;
        double dw = width / 3;
        cairo_set_line_width(cr, width);
        set_color(cr, color);
        cairo_move_to(cr, x + size * 0.6, y + size / 2);
        cairo_line_to(cr, x + size * 0.2, y + size / 2);
        cairo_stroke(cr);
        cairo_move_to(cr, x + size * 0.8 + dw, y + size / 2 - dw);
        cairo_line_to(cr, x + size * 0.5, y + size * 0.8);
        cairo_stroke(cr);
        cairo_move_to(cr, x + size * 0.8 + dw, y + size / 2 + dw);
        cairo_line_to(cr, x + size * 0.5, y + size * 0.2);
        cairo_stroke(cr);
        break;
    }
    case LAMP_FARROW: {
        set_color(cr, COLOR_OFF);
        cairo_arc(cr, x + size / 2, y + size / 2, size * 0.4, 0, 2 * M_PI);
        cairo_fill(cr);

        double width = size / 15.0;
        double dw = width / 3;
        cairo_set_line_width(cr, width);
        set_color(cr, color);
        cairo_move_to(cr, x + size / 2, y + size * 0.4);
        cairo_line_to(cr, x + size / 2, y + size * 0.8);
        cairo_stroke(cr);
        cairo_move_to(cr, x + size / 2 - dw, y + size * 0.2 - dw);
        cairo_line_to(cr, x + size * 0.8, y + size * 0.5);
        cairo_stroke(cr);
        cairo_move_to(cr, x + size / 2 + dw, y + size * 0.2 - dw);
        cairo_line_to(cr, x + size * 0.2, y + size * 0.5);
        cairo_stroke(cr);
        break;
    }
    }
}

void light(cairo_t *cr, int x, int y, int size, const char *colors) {
    int doghouse_top = 0;
    enum lamp_type next_lamp = LAMP_FULL;
    int n = 0;
    int original_x = x;
    for (const char *c = colors; *c; c++) {
        switch (*c) {
        case ':':
            if (doghouse_top)
                n = doghouse_top;
            else
                doghouse_top = n;
            x = original_x - size / 2;
            continue;
        case ';':
            if (doghouse_top)
                n = doghouse_top;
            else
                doghouse_top = n;
            x = original_x + size / 2;
            continue;
        case '<':
        case '>':
        case '^':
            continue;
        }
        margin(cr, x, y + size * n, size);
        n++;
    }
    doghouse_top = 0;
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
        case '_':
            color = COLOR_OFF;
            break;
        case ':':
            if (doghouse_top)
                n = doghouse_top;
            else
                doghouse_top = n;
            x = original_x - size / 2;
            continue;
        case ';':
            if (doghouse_top)
                n = doghouse_top;
            else
                doghouse_top = n;
            x = original_x + size / 2;
            continue;
        case '<':
            next_lamp = LAMP_LARROW;
            continue;
        case '>':
            next_lamp = LAMP_RARROW;
            continue;
        case '^':
            next_lamp = LAMP_FARROW;
            continue;
        default:
            printf("WARNING: '%c' is not a known lamp color\n", *c);
        }
        lamp(cr, x, y + size * n, size, color, next_lamp);
        next_lamp = LAMP_FULL;
        n++;
    }
}


struct light_stage {
    int time_offset;
    char state[32];
};

struct light_desc {
    int loop_time;
    struct light_stage stages[];
};


void print_light_desc(struct light_desc *desc) {
    printf("light_desc {\n\t.loop_time = %i,\n\t.stages = {\n", desc->loop_time);
    for (struct light_stage *stage = desc->stages; stage->state[0]; stage++) {
        printf("\t\t{ .time_offset = %i, .state = \"%s\" },\n",
                stage->time_offset, stage->state);
    }
    printf("\t}\n");
    printf("}\n");
}


struct light_stage *stage(struct light_desc *desc, int offset, long time) {
    long loop_time = (time + offset) % desc->loop_time;
    for (struct light_stage *stage = desc->stages; stage->state[0]; stage++) {
        if (
                loop_time >= stage[0].time_offset &&
                ( loop_time < stage[1].time_offset ||
                  stage[1].time_offset == 0 )
        ) {
            return stage;
        }
    }
    printf("WARNING: no valid state found for light at time %i\n", offset);
    print_light_desc(desc);
    return &(struct light_stage){ .state = "" };
}

void light_desc(
        cairo_t *cr,
        int x,
        int y,
        int size,
        struct light_desc *desc,
        int offset,
        long time
    ) {
    struct light_stage *current_stage = stage(desc, offset, time);
    light(cr, x, y, size, current_stage->state);
}


struct light_spec {
    char name[64];
    int stage_count;
    struct light_desc *desc;
};

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
    cairo_rectangle(cr, 0, 0, 640, 480);
    cairo_fill(cr);

    int ms = millisecond_now();
    int sec = ms / 1000;

    for (int i = 0; i < instruction_count; i++) {
        struct draw_instruction *instr = &instruction_array[i];
        struct light_desc *desc = spec_array[instr->light_id].desc;
        light_desc(cr, instr->x, instr->y, instr->size, desc, instr->offset, sec);
    }
}


void load_light_specs() {
    int spec_id = -1;
    int stage_id = 0;
    char a[256], b[256], *end;
    struct light_desc *desc;
    FILE *file = fopen("lightspec", "r");

    while (!feof(file)) {
        fscanf(file, "%s %s", a, b);
        strtol(a, &end, 10);
        if (*end) {
            spec_count++;
        }
    }

    spec_array = calloc(spec_count, sizeof(struct light_spec));

    rewind(file);
    while (!feof(file)) {
        fscanf(file, "%s %s", a, b);
        strtol(a, &end, 10);
        if (*end) {
            spec_id++;
        } else {
            spec_array[spec_id].stage_count++;
        }
    }

    rewind(file);
    spec_id = -1;
    while (!feof(file)) {
        fscanf(file, "%s %s", a, b);
        int time = strtol(a, &end, 10);
        if (*end) {
            spec_id++;
            desc = calloc(1,
                    sizeof(struct light_desc) +
                    sizeof(struct light_stage) * (
                        spec_array[spec_id].stage_count + 1
                    )
            );
            spec_array[spec_id].desc = desc;
            strcpy(spec_array[spec_id].name, a);
            desc->loop_time = strtol(b, NULL, 10);
            stage_id = 0;
        } else {
            desc->stages[stage_id].time_offset = time;
            strcpy(desc->stages[stage_id].state, b);
            stage_id++;
        }
    }

    for (int i = 0; i < spec_count; i++) {
        print_light_desc(spec_array[i].desc);
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
    char light_name[64];
    int x, y, size, offset;
    int i = 0;

    while (!feof(file)) {
        fscanf(file, "%s %i %i %i %i", light_name, &x, &y, &size, &offset);
        instruction_count++;
    }

    instruction_array = calloc(instruction_count, sizeof(struct draw_instruction));

    rewind(file);
    while (!feof(file)) {
        fscanf(file, "%s %i %i %i %i", light_name, &x, &y, &size, &offset);
        instruction_array[i].light_id = by_name(light_name);
        instruction_array[i].x = x;
        instruction_array[i].y = y;
        instruction_array[i].size = size;
        instruction_array[i].offset = offset;
        i++;
    }
}
