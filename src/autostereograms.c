#include <raylib.h>
#include <raymath.h>
#include <stdio.h>

#define WIDTH 500
#define HEIGHT 500
#define PATTERN_WIDTH  128
#define PATTERN_HEIGHT 128


Color colors[] = {WHITE, BLACK, GREEN, BLUE, RED};
#define COLORS_COUNT (sizeof(colors)/sizeof(colors[0]))


void print_usage(const char *program)
{
    printf("USAGE: %s <depth-map.png> [pattern.png]\n", program);
}


const char *shift_args(int *argc, const char ***argv) 
{
    const char *arg = **argv; 
    (*argc) -= 1;
    (*argv) += 1;
    return arg;
}


int main(int argc, const char **argv)
{
    const char *program = shift_args(&argc, &argv);

    if (argc < 1) {
        print_usage(program);
        fprintf(stderr, "{ERROR]: No depth map is provided!\n");
        return 1;
    }
    const char *depth_map_filepath = shift_args(&argc, &argv);

    const char *patter_filepath = NULL; 
    if (argc >= 1) {
        patter_filepath = shift_args(&argc, &argv); 
    }
    
    SetTraceLogLevel(LOG_WARNING); 
    InitWindow(800, 600, "Stereograms");
    SetTargetFPS(60);

    Image depth_map = LoadImage(depth_map_filepath);
    if (!IsImageReady(depth_map)) {
        return 1;
    }
    
    Image pattern; 
    if (patter_filepath == NULL) {
        pattern = GenImageWhiteNoise(PATTERN_WIDTH, PATTERN_HEIGHT, 0.5);
    } else {
        pattern = LoadImage(patter_filepath); 
        if (!IsImageReady(pattern)) {
            return 1;
        }
    }

    Image output = GenImageColor(depth_map.width, depth_map.height, WHITE);
    Rectangle src_rec = {0, 0, pattern.width, pattern.height};
    for (int y = 0; y < output.height; y += pattern.height) {
        for (int x = 0; x < output.width; x += pattern.width) {
            Rectangle dest_rec = {x, y, pattern.width, pattern.height};
            ImageDraw(&output, pattern, src_rec, dest_rec, WHITE);
        }
    }

    for (int y = 0; y < depth_map.height; ++y) {
        for (int x = 0; x < depth_map.width; ++x) {
            float factor = 1.0f - (GetImageColor(depth_map, x, y)).r / 255.0f;
            int x_offset = factor * 20;
            Color color;
            if (x < pattern.width) {
                color = GetImageColor(pattern, (x + x_offset) % pattern.width, y % pattern.height);
            } else {
                color = GetImageColor(output, (x + x_offset - pattern.width), y); 
            }
            ImageDrawPixel(&output, x, y, color);
        }
    }
    ImageDrawCircle(&output, output.width/2 - pattern.width/2, 20, 10, GRAY);
    ImageDrawCircle(&output, output.width/2 + pattern.width/2, 20, 10, WHITE);
    
    
    Texture2D texture = LoadTextureFromImage(output);
    
    bool draw_alert = false;
    float time = 0.0f;
    while (!WindowShouldClose()) {
        BeginDrawing();
            ClearBackground(BLACK);

            DrawTexture(texture, GetScreenWidth()/2 - texture.width/2, GetScreenHeight()/2 - texture.height/2, WHITE);

            const char *text = "Press [S] to save stereogram";
            int text_x = GetScreenWidth()/2 - MeasureText(text, 30)/2;
            int text_y = GetScreenHeight()/2 + 10 + texture.height/2;
            DrawText(text, text_x, text_y, 30, WHITE);
            
            if (draw_alert) {
                int alert_text_size = 30;
                int alert_text_pad = 10;
                const char *alert_text = "File saved as `output.png` successfully!";

                int alert_margin = 25;
                int alert_width = MeasureText(alert_text, alert_text_size) + alert_text_pad*2;
                int alert_height = alert_text_size + alert_text_pad*2;
                
                int alert_x = GetScreenWidth() - alert_width - alert_margin;
                int alert_y = alert_margin;

                DrawRectangle(alert_x, alert_y, alert_width, alert_height, GRAY); 
                DrawText(alert_text, alert_x + alert_text_pad, alert_y + alert_text_pad, alert_text_size, WHITE);

                time -= GetFrameTime();
                if (time <= 0.0f) draw_alert = false;
            }

            if (IsKeyPressed(KEY_S)) {
                ExportImage(output, "output.png");
                time = 2.0f;
                draw_alert = true;
            }
        EndDrawing();
    }
    

    UnloadImage(depth_map);
    UnloadImage(output);

    UnloadTexture(texture);

    CloseWindow();
    return 0;
}
