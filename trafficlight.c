#include <string.h>
#include <sys/time.h>
#include <SDL.h>
#include <cairo.h>
#include "trafficlight.h"
#include "draw.h"

int window_w = 640;
int window_h = 480;

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

int main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) {
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

struct light_spec left_arrow = {
    .loop_time = 50,
    .layout = "<r<y<g",
    .stages = {
        { 0, "__#" },
        { 20, "_#_" },
        { 25, "#__" },
        { 45, "#F_" },
        { 0, nullptr }
    },
};

void render_frame(cairo_t *cr, [[maybe_unused]] int frame) {
    cairo_set_source_rgba(cr, 0.5, 0.5, 0.5, 1.0);
    cairo_rectangle(cr, 0, 0, window_w, window_h);
    cairo_fill(cr);

    long ms = millisecond_now();

    draw_light_spec(cr, 50, 50, 100, &left_arrow, 0, ms);
}
