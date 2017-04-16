/*
 * Copyright 2017 askmeaboutloom.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#define GAME_VERSION "1.0.0"

#define REAL_WIDTH    1920
#define REAL_HEIGHT   1080
#define WINDOW_WIDTH  1280
#define WINDOW_HEIGHT 720

#define BLOCKS       256
#define BLOCK_WIDTH  120
#define BLOCK_HEIGHT 90
#define CAT_WIDTH    96
#define CAT_HEIGHT   71
#define MAX_LEVELS   128
#define RUN_FRAMES   12

#define FRAME_TIME (1000 / 60)
#define SPEED      24.0
#define GRAVITY    2.0
#define JUMP_FORCE -35.0
#define WALL_SPACE (CAT_WIDTH / 2.0 + 45.0)

#define KILLWALL  ( -50.0 - CAT_WIDTH  / 2.0)
#define KILLPLANE (1400.0 + CAT_HEIGHT / 2.0)

#define ASSET(KEY) KEY = load_texture(load_surface("assets/" #KEY ".png"))

#define R(RENDER_OP) if (RENDER_OP) { die("Render Error", SDL_GetError()); }


struct Block {
    double x;
    double y;
};

static struct {
    struct Block ring[BLOCKS];
    int          start;
    int          len;
} blocks;

static struct {
    int    walled;
    int    grounded;
    int    can_jump;
    int    run_frame;
    double x;
    double y;
    double vel;
} cat;

static struct {
    SDL_Surface *sfcs[MAX_LEVELS];
    int          len;
} levels;

static SDL_Surface *loading;
static int          load_x;
static double       max_x;

static TTF_Font     *font;
static SDL_Window   *window;
static SDL_Renderer *renderer;
static SDL_Texture  *backdrop;
static SDL_Texture  *cat_air;
static SDL_Texture  *cat_ground;
static SDL_Texture  *grayness;

static int whiteness;
static int score;
static int high_score;


static void die(const char *title, const char *message)
{
    if (!message) {
        message = "mysterious error";
    }

    fprintf(stderr, "%s: %s", title, message);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, message, window);

    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    abort();
}


static SDL_Surface *load_surface(const char *path)
{
    SDL_Surface *sfc = IMG_Load(path);

    if (!sfc) {
        die("Image Error", IMG_GetError());
    }

    return sfc;
}

static SDL_Texture *load_texture(SDL_Surface *sfc)
{
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, sfc);
    if (!tex) {
        die("Texture Error", SDL_GetError());
    }

    SDL_FreeSurface(sfc);
    return tex;
}

static void load_levels(void)
{
    for (int i = 0; i < MAX_LEVELS; ++i) {
        char buf[64];
        snprintf(buf, sizeof(buf), "assets/level%d.png", i);

        FILE *fh = fopen(buf, "r");
        if (fh) {
            fclose(fh);
            levels.sfcs[levels.len++] = load_surface(buf);
        }
        else if (errno != ENOENT) {
            die(strerror(errno), buf);
        }
    }

    if (levels.len <= 1) {
        die("Level Load Error", "Can't find enough levels.");
    }
}


static struct Block *get_block(int i)
{
    return &blocks.ring[(blocks.start + i) % BLOCKS];
}

static void add_block(double x, double y)
{
    struct Block *block = get_block(blocks.len);

    block->x = x;
    block->y = y;

    if (blocks.len >= BLOCKS) {
        blocks.start = (blocks.start + 1) % BLOCKS;
    }
    else {
        ++blocks.len;
    }
}

static SDL_Rect get_block_rect(struct Block *block)
{
    SDL_Rect rect = {block->x, block->y, BLOCK_WIDTH, BLOCK_HEIGHT};
    return rect;
}


static Uint32 get_pixel(SDL_Surface *sfc, int x, int y)
{
    Uint8 *bytes = sfc->pixels;
    Uint8 *p     = bytes + x * sfc->format->BytesPerPixel + y * sfc->pitch;
    return *(Uint32 *)p;
}

static int is_black(SDL_Surface *sfc, int x, int y)
{
    Uint8 r, g, b;
    SDL_GetRGB(get_pixel(sfc, x, y), sfc->format, &r, &b, &g);
    return r == 0 && g == 0 && b == 0;
}

static void add_partial_level(void)
{
    if (!loading || load_x >= loading->w) {
        loading = levels.sfcs[rand() % (levels.len - 1) + 1];
        load_x  = 0;
    }

    for (int y = 0; y < loading->h; ++y) {
        if (is_black(loading, load_x, y)) {
            add_block(max_x, y * BLOCK_HEIGHT);
        }
    }

    load_x += 1;
    max_x  += BLOCK_WIDTH;
}


static void reset(void)
{
    SDL_Surface *lvl = levels.sfcs[0];

    blocks.start = 0;
    blocks.len   = 0;

    for (int x = 0; x < lvl->w; ++x) {
        for (int y = 0; y < lvl->h; ++y) {
            if (is_black(lvl, x, y)) {
                add_block(x * BLOCK_WIDTH, y * BLOCK_HEIGHT);
            }
        }
    }

    loading = NULL;
    max_x   = lvl->w * BLOCK_WIDTH;

    cat.walled   = 0;
    cat.grounded = 0;
    cat.can_jump = 0;
    cat.x        = (REAL_WIDTH  - CAT_WIDTH ) / 2.0;
    cat.y        = (REAL_HEIGHT - CAT_HEIGHT) / 2.0;
    cat.vel      = 0.0;

    whiteness = 255;
    score     = 0;
}


static int get_number_len(int n)
{
    int len = 1;
    while (n > 9) {
        n /= 10;
        ++len;
    }
    return len;
}

static SDL_Texture *draw_text(int n, const char *suffix)
{
    char fmt[64];
    snprintf(fmt, sizeof(fmt), "%%0%dd %%s", get_number_len(high_score));

    char buf[1024];
    snprintf(buf, sizeof(buf), fmt, n, suffix);

    SDL_Color    white = {0xff, 0xff, 0xff};
    SDL_Surface *sfc   = TTF_RenderUTF8_Blended(font, buf, white);
    if (!sfc) {
        die("Text Render Error", TTF_GetError());
    }

    return load_texture(sfc);
}


static void update_jump(void)
{
    if (cat.grounded && cat.can_jump) {
        Uint32       mouse = SDL_GetMouseState(NULL, NULL);
        const Uint8 *keys  = SDL_GetKeyboardState(NULL);
        int          jump  = mouse & SDL_BUTTON(SDL_BUTTON_LEFT)
                          || keys[SDL_SCANCODE_SPACE]
                          || keys[SDL_SCANCODE_RETURN]
                          || keys[SDL_SCANCODE_UP];
        if (jump) {
            cat.can_jump  = 0;
            cat.vel      += JUMP_FORCE;
        }
    }
}


static void update_blocks(void)
{
    for (int i = 0; i < blocks.len; ++i) {
        get_block(i)->x -= SPEED;
    }

    max_x -= SPEED;
}


static void update_check_walls(void)
{
    SDL_Point point = {cat.x + WALL_SPACE, cat.y + CAT_HEIGHT / 2.0};
    cat.walled      = 0;

    for (int i = 0; i < blocks.len; ++i) {
        SDL_Rect rect = get_block_rect(get_block(i));

        if (SDL_PointInRect(&point, &rect)) {
            cat.x      = rect.x - WALL_SPACE;
            cat.walled = 1;
            break;
        }
    }
}

static void update_check_floors(void)
{
    SDL_Rect hitbox = {cat.x, cat.y + 51, 81, 20};

    for (int i = 0; i < blocks.len; ++i) {
        SDL_Rect rect = get_block_rect(get_block(i));

        if (SDL_HasIntersection(&hitbox, &rect))  {
            cat.grounded = 1;
            cat.can_jump = 1;
            cat.y        = rect.y - CAT_HEIGHT / 2.0 - 30.0;
            cat.vel      = 0.0;
            break;
        }
    }
}

static void update_check_ceilings(void)
{
    SDL_Rect hitbox = {cat.x, cat.y + 10, 76, 20};

    for (int i = 0; i < blocks.len; ++i) {
        SDL_Rect rect = get_block_rect(get_block(i));

        if (SDL_HasIntersection(&hitbox, &rect))  {
            cat.y   = rect.y + BLOCK_HEIGHT - 5.0;
            cat.vel = 0.0;
            break;
        }
    }
}


static void update(void)
{
    update_jump();
    update_blocks();
    update_check_walls();

    cat.vel      += GRAVITY;
    cat.y        += cat.vel;
    cat.grounded  = 0;

    if (cat.vel > 0.0) {
        update_check_floors();
    }
    else if (cat.vel < 0.0) {
        update_check_ceilings();
    }

    if (++score > high_score) {
        high_score = score;
    }

    if (cat.grounded && !cat.walled) {
        cat.run_frame = (cat.run_frame + 1) % RUN_FRAMES;
    }
    else {
        cat.run_frame = 0;
    }

    if (whiteness > 0) {
        whiteness -= 5;
    }

    if (cat.x < KILLWALL || cat.y > KILLPLANE) {
        reset();
    }
    else {
        while (max_x < REAL_WIDTH) {
            add_partial_level();
        }
    }
}


static void render_level(void)
{
    for (int i = 0; i < blocks.len; ++i) {
        struct Block *block = get_block(i);
        SDL_Rect      rect  = {block->x,        block->y,
                               BLOCK_WIDTH + 1, BLOCK_HEIGHT + 1};
        R(SDL_RenderCopy(renderer, grayness, &rect, &rect));
    }
}

static void render_cat(void)
{
    SDL_Texture *tex;

    if (cat.grounded && cat.run_frame < RUN_FRAMES / 2) {
        tex = cat_ground;
    }
    else {
        tex = cat_air;
    }

    SDL_Rect rect = {cat.x, cat.y};
    R(SDL_QueryTexture(tex, NULL, NULL, &rect.w, &rect.h));

    R(SDL_RenderCopyEx(renderer, tex, NULL, &rect, cat.vel,
                       NULL, SDL_FLIP_NONE));
}

static void render_text(SDL_Texture *tex, int x, int y)
{
    SDL_Rect rect = {x, y};
    R(SDL_QueryTexture(tex, NULL, NULL, &rect.w, &rect.h));

    R(SDL_RenderCopy(renderer, tex, NULL, &rect));

    SDL_DestroyTexture(tex);
}

static void render(void)
{
    R(SDL_RenderClear(renderer));
    R(SDL_RenderCopy(renderer, backdrop, NULL, NULL));

    render_level();
    render_cat();

    render_text(draw_text(score,      "POINTS"    ), 0, REAL_HEIGHT - 100);
    render_text(draw_text(high_score, "HIGH SCORE"), 0, REAL_HEIGHT -  55);

    if (whiteness > 0) {
        SDL_Rect rect = {0, 0, REAL_WIDTH, REAL_HEIGHT};

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, whiteness);
        SDL_RenderFillRect(renderer, &rect);

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, 255);
    }

    SDL_RenderPresent(renderer);
}


static void run_game(void)
{
    Uint32 next = SDL_GetTicks();

    reset();

    for (;;) {
        SDL_Event evt;
        while (SDL_PollEvent(&evt)) {
            if (evt.type == SDL_QUIT) {
                return;
            }
            else if (evt.type == SDL_KEYDOWN) {
                if (evt.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                    return;
                }
            }
        }

        Uint32 now = SDL_GetTicks();
        while (SDL_TICKS_PASSED(now, next)) {
            update();
            next += FRAME_TIME;
        }

        render();
    }
}


int main(int argc, char **argv)
{
    srand(time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER)) {
        die("SDL Init Error", SDL_GetError());
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        die("Image Init Error", IMG_GetError());
    }

    if (TTF_Init()) {
        die("Font Init Error", TTF_GetError());
    }

    font = TTF_OpenFont("assets/Roboto-Regular.ttf", 45);
    if (!font) {
        die("Font Load Error", TTF_GetError());
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");

    window = SDL_CreateWindow("Resultcat Run " GAME_VERSION,
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              WINDOW_WIDTH, WINDOW_HEIGHT,
                              SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        die("Window Error", SDL_GetError());
    }

    renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer) {
        die("Renderer Init Error", SDL_GetError());
    }

    if (SDL_RenderSetLogicalSize(renderer, REAL_WIDTH, REAL_HEIGHT)) {
        die("Resize Error", SDL_GetError());
    }

    ASSET(backdrop);
    ASSET(cat_air);
    ASSET(cat_ground);
    ASSET(grayness);

    load_levels();
    run_game();

    IMG_Quit();
    SDL_Quit();
    TTF_Quit();
    return 0;
}
