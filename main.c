/*
 * BREAKOUT for Miyoo Mini Plus
 * -----------------------------
 * A small Arkanoid/Breakout clone written in plain C + SDL2.
 * No external assets (images/fonts) are needed - everything is drawn
 * with filled rectangles, including a tiny built-in pixel font.
 *
 * Builds and runs natively on Linux/macOS/Windows (with SDL2 installed)
 * for fast iteration, and cross-compiles for the Miyoo Mini Plus with
 * the community "union-miyoomini-toolchain". See README.md for details.
 *
 * Controls (Miyoo Mini Plus physical buttons -> SDL keysym, as used by
 * the common homebrew toolchain. Adjust the KEY_* defines below if your
 * particular firmware/toolchain maps things differently):
 *
 *   D-Pad        -> move paddle
 *   A            -> launch ball / confirm / restart
 *   START        -> pause / unpause
 *   MENU (ESC)   -> quit back to the launcher
 *
 * On a PC keyboard for testing: Arrow keys, Space (=A), Enter (=START),
 * Escape (=MENU).
 */

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define PI_F 3.14159265358979323846f

/* ------------------------------------------------------------------ */
/* Screen / device constants                                          */
/* ------------------------------------------------------------------ */
#define SCREEN_W 640
#define SCREEN_H 480
#define HUD_H 40

/* ------------------------------------------------------------------ */
/* Button mapping - matches the keysyms commonly emulated by Miyoo    */
/* Mini Plus homebrew toolchains for the physical buttons. Keep this  */
/* in one place so it's easy to retarget for a different firmware.    */
/* ------------------------------------------------------------------ */
#define KEY_UP      SDLK_UP
#define KEY_DOWN    SDLK_DOWN
#define KEY_LEFT    SDLK_LEFT
#define KEY_RIGHT   SDLK_RIGHT
#define KEY_A       SDLK_SPACE     /* A button   */
#define KEY_B       SDLK_LCTRL     /* B button   */
#define KEY_X       SDLK_LSHIFT    /* X button   */
#define KEY_Y       SDLK_LALT      /* Y button   */
#define KEY_START   SDLK_RETURN    /* START      */
#define KEY_SELECT  SDLK_RCTRL     /* SELECT     */
#define KEY_MENU1   SDLK_ESCAPE    /* MENU (most firmwares) */
#define KEY_MENU2   SDLK_HOME      /* MENU (some firmwares) */

/* ------------------------------------------------------------------ */
/* Gameplay constants                                                  */
/* ------------------------------------------------------------------ */
#define PADDLE_W 90
#define PADDLE_H 12
#define PADDLE_Y (SCREEN_H - 30)
#define PADDLE_SPEED 420.0f

#define BALL_SIZE 8
#define BALL_BASE_SPEED 260.0f
#define BALL_SPEED_PER_LEVEL 22.0f
#define BALL_MAX_SPEED 480.0f
#define BALL_MAX_BOUNCE_DEG 60.0f

#define BRICK_ROWS 6
#define BRICK_COLS 10
#define BRICK_MARGIN_X 20
#define BRICK_GAP 4
#define BRICK_H 16
#define BRICK_TOP_Y (HUD_H + 30)
#define BRICK_ROW_GAP 6

#define START_LIVES 3

/* ------------------------------------------------------------------ */
/* Tiny built-in 5x7 pixel font - just enough glyphs for this game.   */
/* ------------------------------------------------------------------ */
typedef const char *Glyph[7];

static const char *const *glyph_for(char c) {
    static const char *G_SPACE[7] = {".....",".....",".....",".....",".....",".....","....."};
    static const char *G_0[7] = {"#####","#...#","#...#","#...#","#...#","#...#","#####"};
    static const char *G_1[7] = {"..#..",".##..","..#..","..#..","..#..","..#..",".###."};
    static const char *G_2[7] = {"#####","....#","....#","#####","#....","#....","#####"};
    static const char *G_3[7] = {"#####","....#","....#","#####","....#","....#","#####"};
    static const char *G_4[7] = {"#...#","#...#","#...#","#####","....#","....#","....#"};
    static const char *G_5[7] = {"#####","#....","#....","#####","....#","....#","#####"};
    static const char *G_6[7] = {"#####","#....","#....","#####","#...#","#...#","#####"};
    static const char *G_7[7] = {"#####","....#","....#","...#.","..#..",".#...","#...."};
    static const char *G_8[7] = {"#####","#...#","#...#","#####","#...#","#...#","#####"};
    static const char *G_9[7] = {"#####","#...#","#...#","#####","....#","....#","#####"};
    static const char *G_A[7] = {".###.","#...#","#...#","#####","#...#","#...#","#...#"};
    static const char *G_B[7] = {"####.","#...#","#...#","####.","#...#","#...#","####."};
    static const char *G_C[7] = {".####","#....","#....","#....","#....","#....",".####"};
    static const char *G_E[7] = {"#####","#....","#....","####.","#....","#....","#####"};
    static const char *G_G[7] = {".####","#....","#....","#.###","#...#","#...#",".####"};
    static const char *G_I[7] = {"#####","..#..","..#..","..#..","..#..","..#..","#####"};
    static const char *G_K[7] = {"#...#","#..#.","#.#..","##...","#.#..","#..#.","#...#"};
    static const char *G_L[7] = {"#....","#....","#....","#....","#....","#....","#####"};
    static const char *G_M[7] = {"#...#","##.##","#.#.#","#.#.#","#...#","#...#","#...#"};
    static const char *G_O[7] = {".###.","#...#","#...#","#...#","#...#","#...#",".###."};
    static const char *G_P[7] = {"####.","#...#","#...#","####.","#....","#....","#...."};
    static const char *G_R[7] = {"####.","#...#","#...#","####.","#.#..","#..#.","#...#"};
    static const char *G_S[7] = {".####","#....","#....",".###.","....#","....#","####."};
    static const char *G_T[7] = {"#####","..#..","..#..","..#..","..#..","..#..","..#.."};
    static const char *G_U[7] = {"#...#","#...#","#...#","#...#","#...#","#...#",".###."};
    static const char *G_V[7] = {"#...#","#...#","#...#","#...#",".#.#.",".#.#.","..#.."};

    switch (c) {
        case '0': return G_0; case '1': return G_1; case '2': return G_2;
        case '3': return G_3; case '4': return G_4; case '5': return G_5;
        case '6': return G_6; case '7': return G_7; case '8': return G_8;
        case '9': return G_9;
        case 'A': return G_A; case 'B': return G_B; case 'C': return G_C;
        case 'E': return G_E; case 'G': return G_G; case 'I': return G_I;
        case 'K': return G_K; case 'L': return G_L; case 'M': return G_M;
        case 'O': return G_O; case 'P': return G_P; case 'R': return G_R;
        case 'S': return G_S; case 'T': return G_T; case 'U': return G_U;
        case 'V': return G_V;
        case ' ': return G_SPACE;
        default:  return G_SPACE;
    }
}

static void draw_glyph(SDL_Renderer *r, int x, int y, int scale, char c, SDL_Color col) {
    const char *const *rows = glyph_for(c);
    SDL_SetRenderDrawColor(r, col.r, col.g, col.b, col.a);
    for (int row = 0; row < 7; row++) {
        const char *line = rows[row];
        for (int colx = 0; colx < 5; colx++) {
            if (line[colx] == '#') {
                SDL_Rect px = { x + colx * scale, y + row * scale, scale, scale };
                SDL_RenderFillRect(r, &px);
            }
        }
    }
}

static int text_width(const char *text, int scale) {
    int len = (int)strlen(text);
    if (len == 0) return 0;
    return len * 6 * scale - scale; /* 5 px glyph + 1 px gap, minus trailing gap */
}

static void draw_text(SDL_Renderer *r, int x, int y, int scale, const char *text, SDL_Color col) {
    int cx = x;
    for (const char *p = text; *p; p++) {
        draw_glyph(r, cx, y, scale, *p, col);
        cx += 6 * scale;
    }
}

static void draw_text_centered(SDL_Renderer *r, int cx, int y, int scale, const char *text, SDL_Color col) {
    int w = text_width(text, scale);
    draw_text(r, cx - w / 2, y, scale, text, col);
}

/* ------------------------------------------------------------------ */
/* Game data structures                                               */
/* ------------------------------------------------------------------ */
typedef struct {
    float x, y, w, h;
    int alive;
    SDL_Color color;
    int points;
} Brick;

typedef enum {
    STATE_TITLE,
    STATE_SERVE,
    STATE_PLAY,
    STATE_PAUSED,
    STATE_LEVEL_CLEAR,
    STATE_GAME_OVER
} GameState;

typedef struct {
    float x, y;       /* paddle top-left */
    float ball_x, ball_y;
    float ball_vx, ball_vy;
    Brick bricks[BRICK_ROWS][BRICK_COLS];
    int bricks_left;
    int score;
    int lives;
    int level;
    GameState state;
    GameState pause_return_state;
    float state_timer;   /* generic countdown used by LEVEL_CLEAR etc. */
    float blink_timer;
    int left_held, right_held;
    int running;

    /* audio */
    SDL_AudioDeviceID audio_dev;
    int audio_ok;
    int audio_freq;
} Game;

/* ------------------------------------------------------------------ */
/* Tiny built-in beep synth (no external sound files needed)          */
/* ------------------------------------------------------------------ */
#define MAX_TONE_SAMPLES 24000
static Sint16 tone_buf[MAX_TONE_SAMPLES];

static void play_tone(Game *g, float freq_hz, float duration_s, float volume) {
    if (!g->audio_ok) return;
    int count = (int)(g->audio_freq * duration_s);
    if (count > MAX_TONE_SAMPLES) count = MAX_TONE_SAMPLES;
    if (count <= 0) return;

    for (int i = 0; i < count; i++) {
        float t = (float)i / (float)g->audio_freq;
        /* simple square wave with a short fade-out to avoid clicks */
        float phase = fmodf(t * freq_hz, 1.0f);
        float s = (phase < 0.5f) ? 1.0f : -1.0f;
        float fade = 1.0f - (float)i / (float)count;
        tone_buf[i] = (Sint16)(s * volume * fade * 6000.0f);
    }
    SDL_ClearQueuedAudio(g->audio_dev);
    SDL_QueueAudio(g->audio_dev, tone_buf, (Uint32)(count * sizeof(Sint16)));
    SDL_PauseAudioDevice(g->audio_dev, 0);
}

static void sfx_paddle(Game *g)  { play_tone(g, 440.0f, 0.06f, 0.5f); }
static void sfx_brick(Game *g)   { play_tone(g, 880.0f, 0.05f, 0.4f); }
static void sfx_wall(Game *g)    { play_tone(g, 300.0f, 0.04f, 0.3f); }
static void sfx_lose_life(Game *g) { play_tone(g, 150.0f, 0.30f, 0.6f); }
static void sfx_level(Game *g)   { play_tone(g, 660.0f, 0.20f, 0.5f); }
static void sfx_gameover(Game *g) { play_tone(g, 120.0f, 0.50f, 0.6f); }

/* ------------------------------------------------------------------ */
/* Game logic                                                          */
/* ------------------------------------------------------------------ */
static const SDL_Color ROW_COLORS[BRICK_ROWS] = {
    {220, 40, 40, 255},
    {230, 120, 30, 255},
    {230, 210, 30, 255},
    {60, 200, 60, 255},
    {50, 180, 220, 255},
    {90, 90, 230, 255},
};
static const int ROW_POINTS[BRICK_ROWS] = {60, 50, 40, 30, 20, 10};

static void build_bricks(Game *g) {
    int avail = SCREEN_W - 2 * BRICK_MARGIN_X - (BRICK_COLS - 1) * BRICK_GAP;
    int brick_w = avail / BRICK_COLS;
    g->bricks_left = 0;
    for (int r = 0; r < BRICK_ROWS; r++) {
        for (int c = 0; c < BRICK_COLS; c++) {
            Brick *b = &g->bricks[r][c];
            b->w = (float)brick_w;
            b->h = (float)BRICK_H;
            b->x = (float)(BRICK_MARGIN_X + c * (brick_w + BRICK_GAP));
            b->y = (float)(BRICK_TOP_Y + r * (BRICK_H + BRICK_ROW_GAP));
            b->alive = 1;
            b->color = ROW_COLORS[r];
            b->points = ROW_POINTS[r];
            g->bricks_left++;
        }
    }
}

static float current_ball_speed(Game *g) {
    float s = BALL_BASE_SPEED + (g->level - 1) * BALL_SPEED_PER_LEVEL;
    if (s > BALL_MAX_SPEED) s = BALL_MAX_SPEED;
    return s;
}

static void reset_ball_on_paddle(Game *g) {
    g->ball_x = g->x + PADDLE_W / 2.0f - BALL_SIZE / 2.0f;
    g->ball_y = PADDLE_Y - BALL_SIZE - 1;
    g->ball_vx = 0;
    g->ball_vy = 0;
}

static void launch_ball(Game *g) {
    float speed = current_ball_speed(g);
    float deg = (float)(rand() % 41 - 20); /* -20..20 degrees off vertical */
    float rad = deg * PI_F / 180.0f;
    g->ball_vx = speed * sinf(rad);
    g->ball_vy = -speed * cosf(rad);
}

static void new_game(Game *g) {
    g->score = 0;
    g->lives = START_LIVES;
    g->level = 1;
    g->x = (SCREEN_W - PADDLE_W) / 2.0f;
    build_bricks(g);
    reset_ball_on_paddle(g);
    g->state = STATE_SERVE;
}

static void next_level(Game *g) {
    g->level++;
    g->x = (SCREEN_W - PADDLE_W) / 2.0f;
    build_bricks(g);
    reset_ball_on_paddle(g);
    g->state = STATE_SERVE;
}

static void update_play(Game *g, float dt) {
    /* paddle movement */
    if (g->left_held)  g->x -= PADDLE_SPEED * dt;
    if (g->right_held) g->x += PADDLE_SPEED * dt;
    if (g->x < 0) g->x = 0;
    if (g->x > SCREEN_W - PADDLE_W) g->x = SCREEN_W - PADDLE_W;

    /* ball movement */
    g->ball_x += g->ball_vx * dt;
    g->ball_y += g->ball_vy * dt;

    /* wall collisions */
    if (g->ball_x <= 0) { g->ball_x = 0; g->ball_vx = -g->ball_vx; sfx_wall(g); }
    if (g->ball_x + BALL_SIZE >= SCREEN_W) { g->ball_x = SCREEN_W - BALL_SIZE; g->ball_vx = -g->ball_vx; sfx_wall(g); }
    if (g->ball_y <= HUD_H) { g->ball_y = (float)HUD_H; g->ball_vy = -g->ball_vy; sfx_wall(g); }

    /* paddle collision (only when moving downward) */
    float pad_x = g->x, pad_y = (float)PADDLE_Y, pad_w = (float)PADDLE_W, pad_h = (float)PADDLE_H;
    if (g->ball_vy > 0 &&
        g->ball_x < pad_x + pad_w && g->ball_x + BALL_SIZE > pad_x &&
        g->ball_y < pad_y + pad_h && g->ball_y + BALL_SIZE > pad_y) {

        float paddle_center = g->x + PADDLE_W / 2.0f;
        float ball_center = g->ball_x + BALL_SIZE / 2.0f;
        float offset = (ball_center - paddle_center) / (PADDLE_W / 2.0f);
        if (offset < -1.0f) offset = -1.0f;
        if (offset > 1.0f) offset = 1.0f;

        float speed = current_ball_speed(g);
        float deg = offset * BALL_MAX_BOUNCE_DEG;
        float rad = deg * PI_F / 180.0f;
        g->ball_vx = speed * sinf(rad);
        g->ball_vy = -speed * cosf(rad);
        g->ball_y = (float)PADDLE_Y - BALL_SIZE - 1;
        sfx_paddle(g);
    }

    /* brick collisions - resolve at most one brick per frame */
    for (int r = 0; r < BRICK_ROWS && g->bricks_left > 0; r++) {
        for (int c = 0; c < BRICK_COLS; c++) {
            Brick *b = &g->bricks[r][c];
            if (!b->alive) continue;
            if (g->ball_x < b->x + b->w && g->ball_x + BALL_SIZE > b->x &&
                g->ball_y < b->y + b->h && g->ball_y + BALL_SIZE > b->y) {

                float ball_cx = g->ball_x + BALL_SIZE / 2.0f;
                float ball_cy = g->ball_y + BALL_SIZE / 2.0f;
                float brick_cx = b->x + b->w / 2.0f;
                float brick_cy = b->y + b->h / 2.0f;
                float dx = ball_cx - brick_cx;
                float dy = ball_cy - brick_cy;
                float overlap_x = (BALL_SIZE / 2.0f + b->w / 2.0f) - fabsf(dx);
                float overlap_y = (BALL_SIZE / 2.0f + b->h / 2.0f) - fabsf(dy);

                if (overlap_x < overlap_y) g->ball_vx = -g->ball_vx;
                else                       g->ball_vy = -g->ball_vy;

                b->alive = 0;
                g->bricks_left--;
                g->score += b->points;
                sfx_brick(g);
                goto brick_done;
            }
        }
    }
brick_done:

    /* ball fell off the bottom */
    if (g->ball_y > SCREEN_H) {
        g->lives--;
        sfx_lose_life(g);
        if (g->lives <= 0) {
            g->state = STATE_GAME_OVER;
            g->state_timer = 0;
            sfx_gameover(g);
        } else {
            reset_ball_on_paddle(g);
            g->state = STATE_SERVE;
        }
        return;
    }

    /* level cleared */
    if (g->bricks_left <= 0) {
        g->state = STATE_LEVEL_CLEAR;
        g->state_timer = 2.0f;
        sfx_level(g);
    }
}

static void update(Game *g, float dt) {
    g->blink_timer += dt;
    if (g->blink_timer > 1.0f) g->blink_timer -= 1.0f;

    switch (g->state) {
        case STATE_TITLE:
            break;
        case STATE_SERVE:
            if (g->left_held)  g->x -= PADDLE_SPEED * dt;
            if (g->right_held) g->x += PADDLE_SPEED * dt;
            if (g->x < 0) g->x = 0;
            if (g->x > SCREEN_W - PADDLE_W) g->x = SCREEN_W - PADDLE_W;
            g->ball_x = g->x + PADDLE_W / 2.0f - BALL_SIZE / 2.0f;
            break;
        case STATE_PLAY:
            update_play(g, dt);
            break;
        case STATE_PAUSED:
            break;
        case STATE_LEVEL_CLEAR:
            g->state_timer -= dt;
            if (g->state_timer <= 0) next_level(g);
            break;
        case STATE_GAME_OVER:
            break;
    }
}

/* ------------------------------------------------------------------ */
/* Rendering                                                           */
/* ------------------------------------------------------------------ */
static void fill_rectf(SDL_Renderer *r, float x, float y, float w, float h, SDL_Color c) {
    SDL_Rect rect = { (int)x, (int)y, (int)w, (int)h };
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_RenderFillRect(r, &rect);
}

static void render_hud(SDL_Renderer *r, Game *g) {
    SDL_Color white = {230, 230, 230, 255};
    char buf[32];

    snprintf(buf, sizeof(buf), "SCORE %06d", g->score);
    draw_text(r, 10, 10, 2, buf, white);

    snprintf(buf, sizeof(buf), "LVL %02d", g->level);
    draw_text_centered(r, SCREEN_W / 2, 10, 2, buf, white);

    snprintf(buf, sizeof(buf), "LIVES %d", g->lives);
    int w = text_width(buf, 2);
    draw_text(r, SCREEN_W - 10 - w, 10, 2, buf, white);
}

static void render(SDL_Renderer *r, Game *g) {
    SDL_SetRenderDrawColor(r, 8, 8, 16, 255);
    SDL_RenderClear(r);

    SDL_Color white = {230, 230, 230, 255};
    SDL_Color yellow = {230, 210, 30, 255};

    if (g->state == STATE_TITLE) {
        draw_text_centered(r, SCREEN_W / 2, 160, 6, "BREAKOUT", yellow);
        if (g->blink_timer < 0.5f)
            draw_text_centered(r, SCREEN_W / 2, 260, 3, "PRESS START", white);
        SDL_RenderPresent(r);
        return;
    }

    render_hud(r, g);

    /* bricks */
    for (int row = 0; row < BRICK_ROWS; row++)
        for (int col = 0; col < BRICK_COLS; col++) {
            Brick *b = &g->bricks[row][col];
            if (b->alive) fill_rectf(r, b->x, b->y, b->w, b->h, b->color);
        }

    /* paddle */
    SDL_Color paddle_col = {220, 220, 240, 255};
    fill_rectf(r, g->x, (float)PADDLE_Y, (float)PADDLE_W, (float)PADDLE_H, paddle_col);

    /* ball */
    SDL_Color ball_col = {255, 255, 255, 255};
    fill_rectf(r, g->ball_x, g->ball_y, (float)BALL_SIZE, (float)BALL_SIZE, ball_col);

    if (g->state == STATE_SERVE && g->blink_timer < 0.5f)
        draw_text_centered(r, SCREEN_W / 2, PADDLE_Y - 40, 2, "PRESS A", white);

    if (g->state == STATE_PAUSED)
        draw_text_centered(r, SCREEN_W / 2, SCREEN_H / 2 - 10, 5, "PAUSE", white);

    if (g->state == STATE_LEVEL_CLEAR)
        draw_text_centered(r, SCREEN_W / 2, SCREEN_H / 2 - 10, 4, "LEVEL CLEAR", yellow);

    if (g->state == STATE_GAME_OVER) {
        SDL_Color red = {220, 60, 60, 255};
        draw_text_centered(r, SCREEN_W / 2, 170, 5, "GAME OVER", red);
        char buf[32];
        snprintf(buf, sizeof(buf), "SCORE %06d", g->score);
        draw_text_centered(r, SCREEN_W / 2, 250, 3, buf, white);
        if (g->blink_timer < 0.5f)
            draw_text_centered(r, SCREEN_W / 2, 320, 2, "PRESS START", white);
    }

    SDL_RenderPresent(r);
}

/* ------------------------------------------------------------------ */
/* Input                                                               */
/* ------------------------------------------------------------------ */
static void handle_keydown(Game *g, SDL_Keycode key, int repeat) {
    if (key == KEY_MENU1 || key == KEY_MENU2) {
        g->running = 0;
        return;
    }
    if (key == KEY_LEFT)  g->left_held = 1;
    if (key == KEY_RIGHT) g->right_held = 1;
    if (repeat) return; /* the rest are one-shot actions */

    switch (g->state) {
        case STATE_TITLE:
            if (key == KEY_START || key == KEY_A) new_game(g);
            break;
        case STATE_SERVE:
            if (key == KEY_A) { launch_ball(g); g->state = STATE_PLAY; }
            if (key == KEY_START) { g->pause_return_state = g->state; g->state = STATE_PAUSED; }
            break;
        case STATE_PLAY:
            if (key == KEY_START) { g->pause_return_state = g->state; g->state = STATE_PAUSED; }
            break;
        case STATE_PAUSED:
            if (key == KEY_START) g->state = g->pause_return_state;
            break;
        case STATE_LEVEL_CLEAR:
            break;
        case STATE_GAME_OVER:
            if (key == KEY_START || key == KEY_A) g->state = STATE_TITLE;
            break;
    }
}

static void handle_keyup(Game *g, SDL_Keycode key) {
    if (key == KEY_LEFT)  g->left_held = 0;
    if (key == KEY_RIGHT) g->right_held = 0;
}

/* ------------------------------------------------------------------ */
/* Entry point                                                         */
/* ------------------------------------------------------------------ */
int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    srand((unsigned int)time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Breakout",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W, SCREEN_H,
        SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!renderer) {
        fprintf(stderr, "CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_RenderSetLogicalSize(renderer, SCREEN_W, SCREEN_H);

    Game g;
    memset(&g, 0, sizeof(g));
    g.state = STATE_TITLE;
    g.running = 1;
    g.level = 1;

    /* audio (optional - game still works fine if this fails) */
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 1024;
    want.callback = NULL;
    g.audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have,
        SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    if (g.audio_dev != 0 && have.format == AUDIO_S16SYS && have.channels == 1) {
        g.audio_ok = 1;
        g.audio_freq = have.freq;
    } else {
        g.audio_ok = 0;
    }

    Uint32 last_ticks = SDL_GetTicks();

    while (g.running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) g.running = 0;
            else if (e.type == SDL_KEYDOWN) handle_keydown(&g, e.key.keysym.sym, e.key.repeat);
            else if (e.type == SDL_KEYUP) handle_keyup(&g, e.key.keysym.sym);
        }

        Uint32 now = SDL_GetTicks();
        float dt = (now - last_ticks) / 1000.0f;
        if (dt > 0.05f) dt = 0.05f;
        last_ticks = now;

        update(&g, dt);
        render(renderer, &g);

        Uint32 frame_ms = SDL_GetTicks() - now;
        if (frame_ms < 16) SDL_Delay(16 - frame_ms);
    }

    if (g.audio_dev) SDL_CloseAudioDevice(g.audio_dev);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
