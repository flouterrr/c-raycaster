#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "raylib.h"
#include "raymath.h"

#define MIN(a, b) ((a < b) ? a : b)
#define MAX(a, b) ((a > b) ? a : b)

#define RENDER_ASPECT_RATIO_W 16
#define RENDER_ASPECT_RATIO_H 9
#define RENDER_FACTOR 15

#define RENDER_WIDTH (RENDER_ASPECT_RATIO_W * RENDER_FACTOR)
#define RENDER_HEIGHT (RENDER_ASPECT_RATIO_H * RENDER_FACTOR)

#define INITIAL_WINDOW_FACTOR 70

#define HALF_PI (PI * 0.5f)

#define FOV HALF_PI
#define NEAR_CLIP_PLANE 0.1f
#define FAR_CLIP_PLANE 10.0f

float wrap_angle(float angle) { return Wrap(angle, 0.0f, 2 * PI); }

Vector2 vec2_from_angle(float angle)
{
    Vector2 v = {0.0f, -1.0f};
    return Vector2Normalize(Vector2Rotate(v, angle));
}

float angle_of_vec2(Vector2 vec)
{
    Vector2 v = {0.0f, -1.0f};
    return wrap_angle(Vector2Angle(v, vec));
}


typedef struct {
    uint8_t* tiles;
    uint32_t grid_rows;
    uint32_t grid_cols;
} scene_t;

void init_scene(scene_t* scene, uint8_t tiles[], uint32_t grid_cols,
                uint32_t grid_rows)
{
    scene->tiles = (uint8_t*)calloc(grid_cols * grid_rows, sizeof(uint8_t));
    for (size_t i = 0; i < grid_cols * grid_rows; ++i) {
        scene->tiles[i] = tiles[i];
    }
    scene->grid_cols = grid_cols;
    scene->grid_rows = grid_rows;
}

void cleanup_scene(scene_t* scene) { free(scene->tiles); }

#define PIXEL_BUFFER_SIZE (RENDER_WIDTH * RENDER_HEIGHT)
Color g_pixel_buffer[PIXEL_BUFFER_SIZE] = {0};

void set_pixel(uint32_t x, uint32_t y, Color col)
{
    g_pixel_buffer[y * RENDER_WIDTH + x] = col;
}

void clear_pixel_buffer()
{
    for (size_t i = 0; i < PIXEL_BUFFER_SIZE; ++i) {
        g_pixel_buffer[i] = BLACK;
    }
}

uint8_t get_tile_id(scene_t* scene, uint32_t x, uint32_t y)
{
    assert(x < scene->grid_cols && y < scene->grid_rows);
    return scene->tiles[y * scene->grid_cols + x];
}

Color get_tile_color(uint8_t tile_id)
{
    assert(tile_id != 0);
    switch (tile_id) {
    case 1:
        return GetColor(0xff0000ff);
        break;
    case 2:
        return GetColor(0x00ff00ff);
        break;
    case 3:
        return GetColor(0x00ffffff);
        break;
    default:
        return MAGENTA;
    }
}

uint8_t point_touching_wall(scene_t* scene, Vector2 point)
{
    if (point.x <= EPSILON || point.x >= (float)scene->grid_cols - EPSILON ||
        point.y <= EPSILON || point.y >= (float)scene->grid_rows - EPSILON)
    {
        return 0;
    }

    uint8_t tid;
    for (int i = 1; i >= -1; i -= 2) {
        for (int j = 1; j >= -1; j -= 2) {
            tid = get_tile_id(scene, (uint32_t)(point.x + (i * EPSILON)),
                              (uint32_t)(point.y + (i * EPSILON)));
            if (tid != 0) {
                return tid;
            }
        }
    }

    return 0;
}

typedef struct {
    Vector2 point;
    bool hit_vertical;
    uint8_t tile_id;
} ray_result_t;

ray_result_t cast_ray(scene_t* scene, Vector2 from, float dir)
{
    const int MAX_STEPS = (int)FAR_CLIP_PLANE + 1;
    Vector2 pr, pc;
    float dx, dy;
    uint8_t r_tid, c_tid;
    {
        // checking against grid columns
        pc = from;
        dx = (float)(dir < PI) - (from.x - floorf(from.x));
        if (dir == 0.0f) {
            dy = -from.y;
        } else if (dir == PI) {
            dy = -((float)scene->grid_rows - from.y);
        } else if (dir == HALF_PI || dir == 3 * HALF_PI) {
            dy = 0.0f;
        } else {
            dy = -(dx / tanf(dir));
        }

        pc.x += dx;
        pc.y += dy;

        dx = (dir < PI) ? 1.0f : -1.0f;
        if (dir == 0.0f) {
            dy = -1.0f;
        } else if (dir == PI) {
            dy = 1.0f;
        } else if (dir == HALF_PI || dir == 3 * HALF_PI) {
            dy = 0.0f;
        } else {
            dy = -(dx / tanf(dir));
        }

        for (int i = 0; i < MAX_STEPS; ++i) {
            c_tid = point_touching_wall(scene, pc);
            if (c_tid != 0) {
                break;
            }
            pc.x += dx;
            pc.y += dy;
        }
    }

    {
        // checking against grid rows
        pr = from;
        dy = (float)(dir > HALF_PI && dir < 3 * HALF_PI) -
             (from.y - floorf(from.y));

        if (dir == 3 * HALF_PI) {
            dx = -from.x;
        } else if (dir == HALF_PI) {
            dx = -((float)scene->grid_cols - from.x);
        } else if (dir == 0.0f || dir == PI) {
            dx = 0.0f;
        } else {
            dx = -(dy * tanf(dir));
        }

        pr.x += dx;
        pr.y += dy;

        dy = (dir > HALF_PI && dir < 3 * HALF_PI) ? 1.0f : -1.0f;
        if (dir == 0.0f || dir == PI) {
            dx = 0.0f;
        } else if (dir == HALF_PI) {
            dx = 1.0f;
        } else if (dir == 3 * HALF_PI) {
            dx = -1.0f;
        } else {
            dx = -(dy * tanf(dir));
        }

        for (int i = 0; i < MAX_STEPS; ++i) {
            r_tid = point_touching_wall(scene, pr);
            if (r_tid != 0) {
                break;
            }
            pr.x += dx;
            pr.y += dy;
        }
    }

    ray_result_t r;
    if (Vector2DistanceSqr(from, pc) < Vector2DistanceSqr(from, pr)) {
        r.point = pc;
        r.hit_vertical = true;
        r.tile_id = c_tid;
    } else {
        r.point = pr;
        r.hit_vertical = false;
        r.tile_id = r_tid;
    }

    return r;
}

typedef struct {
    Vector2 pos;
    float rot;
} player_t;


Vector2 get_screen_plane_point(player_t* player, uint8_t index)
{
    assert(index <= 1);
    Vector2 r = Vector2Scale(vec2_from_angle(player->rot), NEAR_CLIP_PLANE);
    Vector2 v;
    if (index == 0) {
        v.x = r.y;
        v.y = -r.x;
    } else {
        v.x = -r.y;
        v.y = r.x;
    }
    v = Vector2Add(Vector2Add(player->pos, r), v);
    return v;
}

float fog_curve = 20.0f;

void render_world(scene_t* scene, player_t* player)
{
    clear_pixel_buffer();

    Vector2 p1 = get_screen_plane_point(player, 0);
    Vector2 p2 = get_screen_plane_point(player, 1);

    for (int x = 0; x < RENDER_WIDTH; ++x) {
        Vector2 p3 = Vector2Lerp(p1, p2, (float)x / (float)RENDER_WIDTH);
        Vector2 ray_dir_vec = Vector2Subtract(p3, player->pos);

        ray_result_t result =
            cast_ray(scene, player->pos, angle_of_vec2(ray_dir_vec));

        if (result.tile_id == 0) {
            continue;
        }

        Vector2 v_to_col = Vector2Subtract(result.point, player->pos);
        float dist = Vector2DotProduct(vec2_from_angle(player->rot), v_to_col);
        uint32_t line_height = RENDER_HEIGHT / dist;
        uint32_t y_start, y_end;
        if (line_height >= RENDER_HEIGHT) {
            y_start = 0;
            y_end = RENDER_HEIGHT;
        } else {
            y_start = (RENDER_HEIGHT / 2) - (line_height / 2);
            y_end = (RENDER_HEIGHT / 2) + (line_height / 2);
        }

        Color col = get_tile_color(result.tile_id);
        // distance fog
        float fog = Clamp(dist, 0.0f, FAR_CLIP_PLANE) / FAR_CLIP_PLANE;
        fog = powf(fog, fog_curve);
        col = ColorBrightness(col, -fog);
        // basic shading based on side of wall
        float shade = result.hit_vertical ? 0.15f : 0.0f;
        col = ColorBrightness(col, -shade);
        for (uint32_t y = y_start; y < y_end; ++y) {
            set_pixel(x, y, col);
        }
    }
}


void render_minimap(scene_t* scene, player_t* player)
{
    const int offset_x = 40;
    const int offset_y = 40;
    const int grid_size = 40;
    const Color grid_color = GetColor(0x505050FF);

    // transparent backdrop
    DrawRectangle(offset_x, offset_y, scene->grid_cols * grid_size,
                  scene->grid_rows * grid_size, GetColor(0x22222244));

    // drawing tiles representation
    for (size_t row = 0; row < scene->grid_rows; ++row) {
        const int y = (row * grid_size) + offset_y;
        for (size_t col = 0; col < scene->grid_cols; ++col) {
            const int x = (col * grid_size) + offset_x;
            uint8_t tile_id = get_tile_id(scene, col, row);
            if (tile_id != 0) {
                Color tile_color = get_tile_color(tile_id);
                DrawRectangle(x, y, grid_size, grid_size, tile_color);
            }
        }
    }

    // drawing grid lines
    for (size_t row = 0; row <= scene->grid_rows; ++row) {
        const int y = (row * grid_size) + offset_y;
        const int sx = offset_x;
        const int dx = (scene->grid_cols * grid_size) + offset_x;
        DrawLine(sx, y, dx, y, grid_color);
    }
    for (size_t col = 0; col <= scene->grid_cols; ++col) {
        const int x = (col * grid_size) + offset_x;
        const int sy = offset_y;
        const int dy = (scene->grid_rows * grid_size) + offset_y;
        DrawLine(x, sy, x, dy, grid_color);
    }

    // drawing player representation
    {
        Vector2 p = {(player->pos.x * grid_size) + offset_x,
                     (player->pos.y * grid_size) + offset_y};
        DrawCircleV(p, 10, PINK);

        Vector2 rv = vec2_from_angle(player->rot);
        const float rot_line_size = 20.0f;
        Vector2 v = Vector2Add(p, Vector2Scale(rv, rot_line_size));
        DrawLineEx(p, v, 5, PINK);
        Vector2 p1 = get_screen_plane_point(player, 0);
        Vector2 p2 = get_screen_plane_point(player, 1);
        /*
        for (int i = 0; i < RENDER_WIDTH; ++i) {
            float t = (float)i / (float)RENDER_WIDTH;
            Vector2 p3 = Vector2Lerp(p1, p2, t);
            Vector2 rdirv = Vector2Subtract(p3, player->pos);
            float hue = Lerp(0.0f, 360.0f, t);
            ray_result_t result =
                cast_ray(scene, player->pos, angle_of_vec2(rdirv));
            Vector2 ray = Vector2Scale(
                Vector2Subtract(result.point, player->pos), grid_size);
            Color col = ColorFromHSV(result.hit_vertical ? (180.0f + hue) : hue,
                                     1.0f, 1.0f);
            col.a = 100;
            DrawLineEx(p, Vector2Add(p, ray), 1, col);
        }
        */

        Vector2 p1m = Vector2Scale(p1, grid_size);
        p1m.x += offset_x;
        p1m.y += offset_y;
        Vector2 p2m = Vector2Scale(p2, grid_size);
        p2m.x += offset_x;
        p2m.y += offset_y;
        DrawLineEx(p, p1m, 1.0f, WHITE);
        DrawLineEx(p, p2m, 1.0f, WHITE);
        DrawLineEx(p1m, p2m, 1.0f, WHITE);
    }
}


int main(int argc, char** argv)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(RENDER_ASPECT_RATIO_W * INITIAL_WINDOW_FACTOR,
               RENDER_ASPECT_RATIO_H * INITIAL_WINDOW_FACTOR, "Raycaster");

    float pixel_width = (float)GetScreenWidth() / (float)RENDER_WIDTH;
    float pixel_height = (float)GetScreenHeight() / (float)RENDER_HEIGHT;

    for (size_t i = 0; i < PIXEL_BUFFER_SIZE; ++i) {
        g_pixel_buffer[i].a = 255;
    }

    scene_t scene;
    {
        /* clang-format off */
        uint8_t tiles[] = {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
            0, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 3,
            0, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 3,
            0, 0, 0, 0, 0, 0, 2, 2, 1, 0, 0, 0, 3,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
        };
        /* clang-format on */
        init_scene(&scene, tiles, 13, 8);
    }

    player_t player = {
        .pos = {2.0f, 2.5f},
        .rot = 0.0f,
    };

    while (!WindowShouldClose()) {
        BeginDrawing();

        ClearBackground(MAGENTA);

        // update
        {
            const float dt = GetFrameTime();


            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                DisableCursor();
            }
            if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                EnableCursor();
            }

            const float mouse_sens = 0.004f;
            Vector2 mouse_delta = GetMouseDelta();
            player.rot = wrap_angle(player.rot + (mouse_delta.x * mouse_sens));

            const float move_speed = 2.0f;
            Vector2 forward_vec = vec2_from_angle(player.rot);
            if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) {
                player.pos = Vector2Add(
                    player.pos, Vector2Scale(forward_vec, move_speed * dt));
            }
            if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) {
                player.pos = Vector2Subtract(
                    player.pos, Vector2Scale(forward_vec, move_speed * dt));
            }
            Vector2 strafe_vec = {-forward_vec.y, forward_vec.x};
            if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
                player.pos = Vector2Subtract(
                    player.pos, Vector2Scale(strafe_vec, move_speed * dt));
            }
            if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
                player.pos = Vector2Add(
                    player.pos, Vector2Scale(strafe_vec, move_speed * dt));
            }
        }

        // render

        render_world(&scene, &player);

        if (IsWindowResized()) {
            pixel_width = (float)GetScreenWidth() / (float)RENDER_WIDTH;
            pixel_height = (float)GetScreenHeight() / (float)RENDER_HEIGHT;
        }
        for (size_t y = 0; y < RENDER_HEIGHT; ++y) {
            for (size_t x = 0; x < RENDER_WIDTH; ++x) {
                DrawRectangle(floorf(x * pixel_width), floorf(y * pixel_height),
                              ceilf(pixel_width), ceilf(pixel_height),
                              g_pixel_buffer[y * RENDER_WIDTH + x]);
            }
        }

        render_minimap(&scene, &player);

        DrawFPS(2, 2);
        EndDrawing();
    }

    cleanup_scene(&scene);

    CloseWindow();
}
