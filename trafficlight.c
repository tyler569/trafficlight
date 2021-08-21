#include <stdbool.h>
#include <sys/time.h>
#include <SDL2/SDL.h>
#include <cairo/cairo.h>

bool done = false;

void render_frame(cairo_t *cr, int frame);

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

int main(int argc, char **argv) {
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
        int color;
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
        int color;
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
        printf("\t\t{ .time_offset = %i, .state = \"%s\" }\n",
                stage->time_offset, stage->state);
    }
    printf("}");
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

struct light_desc simple_gyr = {
    .loop_time = 50,
    .stages = {
        {0, "__g"},
        {20, "_y_"},
        {25, "r__"},
        {0, ""},
    }
};

struct light_desc flashing_amber = {
    .loop_time = 2,
    .stages = {
        {0, "___"},
        {1, "_y_"},
        {0, ""},
    },
};

struct light_desc flashing_red = {
    .loop_time = 2,
    .stages = {
        {0, "r__"},
        {1, "___"},
        {0, ""},
    },
};


void render_frame(cairo_t *cr, int frame) {
    cairo_set_source_rgba(cr, 0.5, 0.5, 0.5, 1.0);
    cairo_rectangle(cr, 0, 0, 640, 480);
    cairo_fill(cr);

#define L1 100, 80, 30
#define L2 200, 80, 30
#define L3 300, 80, 30

    int ms = millisecond_now();
    int second = (ms / 1000);
    int stage = second % 60;
    if (stage < 10) {
        light(cr, L1, "__g<g");
        light(cr, L2, "_:_<g;_g");
        light(cr, L3, "__g");
    } else if (stage < 15) {
        light(cr, L1, "__g<y");
        light(cr, L2, "_:<y_;_g");
        light(cr, L3, "__g");
    } else if (stage < 30) {
        light(cr, L1, "__g_");
        light(cr, L2, "_:__;_g");
        light(cr, L3, "__g");
    } else if (stage < 35) {
        light(cr, L1, "_y__");
        light(cr, L2, "_:__;y_");
        light(cr, L3, "_y_");
    } else {
        light(cr, L1, "r___");
        light(cr, L2, "r:__;__");
        light(cr, L3, "r__");
    }

#define L4 400, 80, 30
#define L5 500, 80, 30

    light_desc(cr, L4, &flashing_amber, 0, second);
    light_desc(cr, L5, &flashing_red, 0, second);
    light_desc(cr, 100, 250, 30, &simple_gyr, 0, second);
    light_desc(cr, 200, 250, 30, &simple_gyr, 25, second);

}

// "r__"
// "r__<y"
// "r__<y_"
// "r:__;_<g"
