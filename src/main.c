#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "cpu.h"
#include "bus.h"
#include <SDL.h>
#define WIDTH 160
#define HEIGHT 144
#define SCALE 4

static uint32_t framebuffer[HEIGHT][WIDTH];



int main(int argc, char *argv[]) {

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <rom.gb>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "rb");

    if (file == NULL) {
        perror("fopen");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    if (size < 0) {
        perror("ftell");
        fclose(file);
        return 1;
    }

    uint8_t *data = calloc(0x10000, 1);

    if (!data) {
        fclose(file);
        return 1;
    }

    size_t rom_size = (size_t)size;

    if (rom_size > 0x8000)
        rom_size = 0x8000;  // for now, before implementing MBCs

    size_t bytes_read = fread(data, 1, rom_size, file);

    if (bytes_read != (size_t)rom_size) {
        fprintf(stderr, "Failed to read entire file\n");
        free(data);
        fclose(file);
        return 1;
    }

    fclose(file);

    // SDL STUFF

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Game Boy Emulator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WIDTH * SCALE,
        HEIGHT * SCALE,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!window) {
        fprintf(stderr, "Window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!renderer) {
        fprintf(stderr, "Renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_RenderSetLogicalSize(renderer, WIDTH, HEIGHT);

    SDL_Texture *texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        WIDTH,
        HEIGHT
    );

    if (!texture) {
        fprintf(stderr, "Texture: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_SetTextureScaleMode(texture, SDL_ScaleModeNearest);

    bool running = true;
    SDL_Event event;

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            framebuffer[y][x] =0xffffffff;
        }
    }
    // Copy CPU framebuffer to SDL texture.
    SDL_UpdateTexture(
        texture,
        NULL,
        framebuffer,
        WIDTH * sizeof(uint32_t)
    );

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);


    CPU cpu;
    Bus bus;
    reset_bus(&bus);
    bus.memory = data;

    reset(&cpu);

    // for (int i=0;i<1000000000;i++) {
    while (running) {
        while (SDL_PollEvent(&event)) {

            if (event.type == SDL_QUIT) {
                running = false;
            }

            else if (event.type == SDL_KEYDOWN) {

                switch (event.key.keysym.sym) {

                    case SDLK_RIGHT:
                        bus.joypad.right = 1;
                        break;

                    case SDLK_LEFT:
                        bus.joypad.left = 1;
                        break;

                    case SDLK_UP:
                        bus.joypad.up = 1;
                        break;

                    case SDLK_DOWN:
                        bus.joypad.down = 1;
                        break;

                    case SDLK_z:
                        bus.joypad.a = 1;
                        break;

                    case SDLK_x:
                        bus.joypad.b = 1;
                        break;

                    case SDLK_BACKSPACE:
                        bus.joypad.select = 1;
                        break;

                    case SDLK_RETURN:
                        bus.joypad.start = 1;
                        break;
                }
            }

            else if (event.type == SDL_KEYUP) {

                switch (event.key.keysym.sym) {

                    case SDLK_RIGHT:
                        bus.joypad.right = 0;
                        break;

                    case SDLK_LEFT:
                        bus.joypad.left = 0;
                        break;

                    case SDLK_UP:
                        bus.joypad.up = 0;
                        break;

                    case SDLK_DOWN:
                        bus.joypad.down = 0;
                        break;

                    case SDLK_z:
                        bus.joypad.a = 0;
                        break;

                    case SDLK_x:
                        bus.joypad.b = 0;
                        break;

                    case SDLK_BACKSPACE:
                        bus.joypad.select = 0;
                        break;

                    case SDLK_RETURN:
                        bus.joypad.start = 0;
                        break;
                }
            }
        }
        while(!bus.ppu.render_frame) {
            tick(&cpu, &bus);
        }
        bus.ppu.render_frame = 0;
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                framebuffer[y][x] = bus.ppu.framebuffer[y * WIDTH + x];
            }
        }
        SDL_UpdateTexture(
            texture,
            NULL,
            framebuffer,
            WIDTH * sizeof(uint32_t)
        );

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        // print_registers(&cpu);
    }
    free(data);


    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}