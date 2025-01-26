#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <string.h>

#define FACTOR 90
#define WIDTH (FACTOR * 16)
#define HEIGHT (FACTOR * 9)
#define PATTERN_WIDTH  128
#define PATTERN_HEIGHT 128
#define MAX_ALERT_MESSAGES 10
#define ALERT_MARGIN 10
#define ALERT_TEXT_SIZE 30
#define ALERT_TEXT_PAD 10
#define DEFAULT_TEXT_SIZE 30
#define TEXT_MARGIN 25

#define BACKGROUND_COLOR ColorFromHSV(0, 0.0f, 0.25f)


Color colors[] = {WHITE, BLACK, GREEN, BLUE, RED};
#define COLORS_COUNT (sizeof(colors)/sizeof(colors[0]))


void print_usage(const char *program)
{
    printf("Usage: %s <depth-map.png> [pattern.png]\n", program);
}


const char *shift_args(int *argc, const char ***argv)
{
    const char *arg = **argv;
    (*argc) -= 1;
    (*argv) += 1;
    return arg;
}


typedef enum AlertState {
    ALERT_NONE = 0,
    ALERT_SUCCES,
    ALERT_FAILURE
} AlertState;

typedef struct AlertMessage {
    char text[1024];
    AlertState state;
    float time;
} AlertMessage;

typedef struct AlertMessages {
    AlertMessage messages[MAX_ALERT_MESSAGES];
    int start;
    int size;
} AlertMessages;


AlertMessages alert_messages = {0};


void add_alert_message(const char *message, AlertState state)
{
    int i = (alert_messages.start + alert_messages.size) % MAX_ALERT_MESSAGES;
    strcpy(alert_messages.messages[i].text, message);
    alert_messages.messages[i].state = state;
    alert_messages.messages[i].time = 1.5f;

    if (alert_messages.size < MAX_ALERT_MESSAGES)
        alert_messages.size += 1;
    else
        alert_messages.start = (alert_messages.start + 1) % MAX_ALERT_MESSAGES;
}

static Image depth_map = {0};
static Image pattern = {0};
static Texture2D texture = {0};
static Image output = {0};

void generate_autostereogram(void)
{
    if (!IsImageValid(pattern)) return;
    if (!IsImageValid(depth_map)) return;

    output = GenImageColor(depth_map.width, depth_map.height, WHITE);
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

    texture = LoadTextureFromImage(output);
}


void draw_autostereogram_screen(void)
{
    ClearBackground(BLACK);
    DrawTexture(texture, GetScreenWidth()/2 - texture.width/2, GetScreenHeight()/2 - texture.height/2, WHITE);

    const char *text = "Press [S] to save stereogram";
    int text_x = GetScreenWidth()/2 - MeasureText(text, DEFAULT_TEXT_SIZE)/2;
    int text_y = GetScreenHeight()/2 + 10 + texture.height/2;
    DrawText(text, text_x, text_y, DEFAULT_TEXT_SIZE, WHITE);

    if (IsKeyPressed(KEY_S)) {
        const char *text_success = "Image saved as `output.png` successfully!";
        const char *text_failure = "Cannot save image. Sorry :(";
        if (ExportImage(output, "output.png")) {
            add_alert_message(text_success, ALERT_SUCCES);
        } else {
            add_alert_message(text_failure, ALERT_FAILURE);
        }
    }

}

// TODO: Rewrite this hell :^)
void draw_drag_and_drop_screen(void)
{
    ClearBackground(BACKGROUND_COLOR);

    char text[] = "Drag and drop image file";
    char pattern_text[] = "Here to use as pattern image";
    char depth_map_text[] = "Here to use as depth map";

    int screen_half_width = GetScreenWidth()/2;

    Rectangle left_screen_half_rec = {
        .x = 0, .y = 0,
        .width = screen_half_width,
        .height = GetScreenHeight()
    };

    Rectangle right_screen_half_rec = {
        .x = screen_half_width, .y = 0,
        .width = screen_half_width,
        .height = GetScreenHeight()
    };

    DrawRectangleRec(left_screen_half_rec, ColorFromHSV(0, 0.0f, 0.30f));
    DrawRectangleRec(right_screen_half_rec, ColorFromHSV(0, 0.0f, 0.40f));

    int x = screen_half_width  - MeasureText(text, DEFAULT_TEXT_SIZE)/2;
    int y = TEXT_MARGIN;
    DrawText(text, x, y, DEFAULT_TEXT_SIZE, WHITE);

    x = screen_half_width + MeasureText(pattern_text, DEFAULT_TEXT_SIZE)/2;
    DrawText(depth_map_text, x, y + 50, DEFAULT_TEXT_SIZE, WHITE);

    x = screen_half_width/2 - MeasureText(pattern_text, DEFAULT_TEXT_SIZE)/2;
    DrawText(pattern_text, x, y + 50, DEFAULT_TEXT_SIZE, WHITE);


    if (IsFileDropped()) {
        FilePathList dropped_files = LoadDroppedFiles();
        if (dropped_files.count > 1) {
            add_alert_message("Drag and drop one file at a time.", ALERT_FAILURE);
            UnloadDroppedFiles(dropped_files);
            return;
        }

        if (CheckCollisionPointRec(GetMousePosition(), left_screen_half_rec)) {
            pattern = LoadImage(dropped_files.paths[0]);
            if (!IsImageValid(pattern)) {
                add_alert_message("Couldn't load image!", ALERT_FAILURE);
            }
            add_alert_message("Pattern image loaded successfully.", ALERT_SUCCES);
        } else if (CheckCollisionPointRec(GetMousePosition(), right_screen_half_rec)) {
            depth_map = LoadImage(dropped_files.paths[0]);
            if (!IsImageValid(depth_map)) {
                add_alert_message("Couldn't load image!", ALERT_FAILURE);
            } else {
               generate_autostereogram();
            }
        printf("good bye!\n");
        }
        UnloadDroppedFiles(dropped_files);
    }
}

void draw_alert(void)
{
    float dt = GetFrameTime();
    int alert_y = 0;
    for (int i = 0; i < alert_messages.size; ++i) {
        int index = (alert_messages.start + i) % MAX_ALERT_MESSAGES;
        AlertMessage *msg = &alert_messages.messages[index];

        if (msg->time >= 0.0f) {
            int alert_width = MeasureText(msg->text, ALERT_TEXT_SIZE) + ALERT_TEXT_PAD*2;
            int alert_height = ALERT_TEXT_SIZE + ALERT_TEXT_PAD*2;

            int alert_x = GetScreenWidth() - alert_width - ALERT_MARGIN;
            alert_y += ALERT_MARGIN;

            Color color = GREEN;
            color = msg->state == ALERT_FAILURE ? RED : color;

            DrawRectangle(alert_x, alert_y, alert_width, alert_height, color);
            DrawText(msg->text, alert_x + ALERT_TEXT_PAD, alert_y + ALERT_TEXT_PAD, ALERT_TEXT_SIZE, WHITE);

            alert_y += alert_height;

            msg->time -= dt;
        }
    }
}

int main(int argc, const char **argv)
{
    const char *program = shift_args(&argc, &argv);
    (void) program; // ignore program name for now

    const char *depth_map_filepath = NULL;
    if (argc >= 1) {
        depth_map_filepath = shift_args(&argc, &argv);
    }

    const char *patter_filepath = NULL;
    if (argc >= 1) {
        patter_filepath = shift_args(&argc, &argv);
    }

    SetTraceLogLevel(LOG_WARNING);
    InitWindow(WIDTH, HEIGHT, "Stereograms");
    SetTargetFPS(60);

    pattern = GenImageWhiteNoise(PATTERN_WIDTH, PATTERN_HEIGHT, 0.5);

    if (depth_map_filepath != NULL) {
        depth_map = LoadImage(depth_map_filepath);
        if (!IsImageValid(depth_map)) {
            return 1;
        }

        if (patter_filepath != NULL) {
            UnloadImage(pattern);
            pattern = LoadImage(patter_filepath);
            if (!IsImageValid(pattern)) {
                return 1;
            }
        }
        generate_autostereogram();
    }

    while (!WindowShouldClose()) {
        BeginDrawing();
            if (IsTextureValid(texture)) {
                draw_autostereogram_screen();
            } else {
                draw_drag_and_drop_screen();
            }
            draw_alert();
        EndDrawing();
    }


    UnloadImage(depth_map);
    UnloadImage(pattern);
    UnloadImage(output);

    UnloadTexture(texture);

    CloseWindow();
    return 0;
}
