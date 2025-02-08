#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <string.h>

#define GUI_IMPLEMENTATION
#include "gui.h"

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

#define BACKGROUND_COLOR    ColorFromHSV(  0, 0.0f, 0.15f)
#define IMAGE_COLOR         ColorFromHSV(  0, 0.0f, 0.20f)
#define ALERT_SUCCESS_COLOR ColorFromHSV(130, 0.6f, 0.8f )
#define ALERT_FAILURE_COLOR ColorFromHSV(  0, 0.6f, 0.8f )


Color colors[] = {WHITE, BLACK, GREEN, BLUE, RED};
#define COLORS_COUNT (sizeof(colors)/sizeof(colors[0]))


void print_usage(const char *program)
{
    printf("Usage: %s <depth-map.png> [pattern.png]\n", program);
}


const char *shift_args(int *argc, const char ***argv)
{
    if (*argc <= 0) return NULL;

    const char *arg = **argv;
    (*argc) -= 1;
    (*argv) += 1;
    return arg;
}


typedef enum AlertType {
    ALERT_SUCCES,
    ALERT_FAILURE,
} AlertType;

typedef struct AlertMessage {
    char text[1024];
    AlertType type;
    float time;
} AlertMessage;

typedef struct AlertMessages {
    AlertMessage messages[MAX_ALERT_MESSAGES];
    int start;
    int size;
} AlertMessages;

AlertMessages alert_messages = {0};


void add_alert_message(const char *message, AlertType type)
{
    int i = (alert_messages.start + alert_messages.size) % MAX_ALERT_MESSAGES;
    strcpy(alert_messages.messages[i].text, message);
    alert_messages.messages[i].type = type;
    alert_messages.messages[i].time = 2.5f;

    if (alert_messages.size < MAX_ALERT_MESSAGES)
        alert_messages.size += 1;
    else
        alert_messages.start = (alert_messages.start + 1) % MAX_ALERT_MESSAGES;
}


typedef enum State {
    STATE_DRAG_AND_DROP,
    STATE_SHOW_AUTOSTEREOGRAM,
    STATE_EXIT,
} State;

static Font font = {0};
static State state = STATE_DRAG_AND_DROP;
static Image depth_map = {0};
static Texture2D depth_map_texture = {0};
static Image pattern = {0};
static Texture2D pattern_texture = {0};
static Texture2D texture = {0};
static Image output = {0};

bool generate_autostereogram(void)
{
    if (!IsImageValid(pattern)) return false;
    if (!IsImageValid(depth_map)) return false;

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
    return true;
}


static bool tooltip_show = false;
static Rectangle tooltip_element_boundary = {0};
static char tooltip_buffer[1024] = {0};

static void tooltip(Rectangle boundary, const char *text, bool persists)
{
    if (!(CheckCollisionPointRec(GetMousePosition(), boundary) || persists)) return;
    tooltip_show = true;
    // TODO: this may not work properly if text contains UTF-8
    snprintf(tooltip_buffer, sizeof(tooltip_buffer), "%s", text);
    tooltip_element_boundary = boundary;
}

void start_tooltip_frame(void)
{
    tooltip_show = false;
}

void end_tooltip_frame(void)
{
    if (!tooltip_show) return;

    Vector2 text_size = MeasureTextEx(font, tooltip_buffer, 30, 3);

    Vector2 rect_size = {
        .x = text_size.x + 20,
        .y = text_size.y + 20,
    };

    Vector2 rect_pos = {
        .x = tooltip_element_boundary.x + tooltip_element_boundary.width/2 - rect_size.x/2,
        .y = tooltip_element_boundary.y - rect_size.y - 10
    };

    Rectangle boundary = {
        rect_pos.x, 
        rect_pos.y, 
        rect_size.x, 
        rect_size.y
    };

    DrawRectangleRec(boundary, IMAGE_COLOR);
    DrawRectangleLinesEx(boundary, 2, BLACK);
    gui_draw_text_centered(font, tooltip_buffer, boundary, 30, 3, WHITE);
}

bool draw_button(Rectangle rect, const char *text, const char *hover_text)
{
    DrawRectangleRec(rect, IMAGE_COLOR);
    gui_draw_text_centered(font, text, rect, 30, 3, WHITE);
    bool is_hovered = CheckCollisionPointRec(GetMousePosition(), rect);
    if (is_hovered) {
        DrawRectangleLinesEx(rect, 3, WHITE);
        tooltip(rect, hover_text, false);
    }

    return is_hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}


void draw_autostereogram_screen(void)
{
    layout_begin(GUI_LAYOUT_VERTICAL, screen_rect(), 12, 15, 10);

    Rectangle source = {
        .x = 0,
        .y = 0,
        .width = texture.width,
        .height = texture.height,
    };
    Rectangle dest = layout_slot_ex(9);
    DrawRectangleRec(dest, IMAGE_COLOR);
    DrawTexturePro(texture, source, gui_fit_rect(dest, source), Vector2Zero(), 0.f, WHITE);

    if (draw_button(layout_slot(), "Save", "Save result [S]") || IsKeyPressed(KEY_S)) {
        const char *text_success = "Image saved as `output.png` successfully!";
        const char *text_failure = "Cannot save image. Sorry :(";
        if (ExportImage(output, "output.png")) {
            add_alert_message(text_success, ALERT_SUCCES);
        } else {
            add_alert_message(text_failure, ALERT_FAILURE);
        }
    }

    if (draw_button(layout_slot(), "Return", "Return to main screen [Q]") || IsKeyPressed(KEY_Q)) {
        state = STATE_DRAG_AND_DROP;
    }

    if (draw_button(layout_slot(), "Exit", "Exit from program [ESC]")) {
        state = STATE_EXIT;
    }

    layout_end();

    return;
}


void draw_image_slot(Rectangle rect, const char *name, Image *image, Texture *texture)
{
    DrawRectangleRec(rect, IMAGE_COLOR);
    layout_begin(GUI_LAYOUT_VERTICAL, rect, 10, 10, 10);
    gui_draw_text_centered(font, name, layout_slot(), 30, 3, WHITE);
    Rectangle drop_boundary = layout_slot_ex(9);
    if (IsTextureValid(*texture)) {
        Rectangle source = {
            .x = 0,
            .y = 0,
            .width = texture->width,
            .height = texture->height
        };
        DrawTexturePro(*texture, source, drop_boundary, Vector2Zero(), 0, WHITE);
    }
    layout_end();

    if (!CheckCollisionPointRec(GetMousePosition(), drop_boundary) || !IsFileDropped()) {
        return;
    }

    FilePathList dropped_files = LoadDroppedFiles();
    if (dropped_files.count > 1) {
        add_alert_message("Drag and drop one file at a time.", ALERT_FAILURE);
        UnloadDroppedFiles(dropped_files);
        return;
    }

    if (IsImageValid(*image)) UnloadImage(*image);
    *image = LoadImage(dropped_files.paths[0]);
    if (!IsImageValid(*image)) {
        add_alert_message("Couldn't load image!", ALERT_FAILURE);
        UnloadDroppedFiles(dropped_files);
        return;
    }

    if (IsTextureValid(*texture)) UnloadTexture(*texture);
    *texture = LoadTextureFromImage(*image);

    add_alert_message(TextFormat("%s loaded successfully", name), ALERT_SUCCES);
    UnloadDroppedFiles(dropped_files);
}

void draw_drag_and_drop_screen(void)
{

    layout_begin(GUI_LAYOUT_VERTICAL, screen_rect(), 11, 10, 25);

    gui_draw_text_centered(font, "Drag and drop image file", layout_slot(), 30, 3, WHITE);
    layout_begin(GUI_LAYOUT_HORIZONTAL, layout_slot_ex(8), 2, 25, 0);
    draw_image_slot(layout_slot(), "Depth map", &depth_map, &depth_map_texture);
    draw_image_slot(layout_slot(), "Pattern image", &pattern, &pattern_texture);
    layout_end();

    if (draw_button(layout_slot(), "Generate", "Generate autostereogram [G]")) {
        if (!IsImageValid(depth_map)) {
            add_alert_message("No depth map is provided", ALERT_FAILURE);
            return;
        }

        if (!IsImageValid(pattern)) {
            pattern = GenImageWhiteNoise(PATTERN_WIDTH, PATTERN_HEIGHT, 0.5);
        }

        if (generate_autostereogram()) {
            state = STATE_SHOW_AUTOSTEREOGRAM;
        }
    }
    
    if (draw_button(layout_slot(), "Exit", "Exit from program [ESC]")) {
        state = STATE_EXIT;
    }

    layout_end();

    if (IsFileDropped()) {
        UnloadDroppedFiles(LoadDroppedFiles());
    }

    return;
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

            Color color = msg->type == ALERT_FAILURE ? ALERT_FAILURE_COLOR : ALERT_SUCCESS_COLOR;

            DrawRectangle(alert_x, alert_y, alert_width, alert_height, color);
            DrawText(msg->text, alert_x + ALERT_TEXT_PAD, alert_y + ALERT_TEXT_PAD, ALERT_TEXT_SIZE, WHITE);

            alert_y += alert_height;

            msg->time -= dt;
        }
    }
}

void load_image_if_exists(Image *image, Texture *texture, const char *file_path)
{
    if (file_path == NULL) return;

    *image = LoadImage(file_path);
    if (!IsImageValid(*image)) return;

    *texture = LoadTextureFromImage(*image);
    if (!IsTextureValid(*texture)) return;
}

int main(int argc, const char **argv)
{
    const char *program = shift_args(&argc, &argv);
    (void) program; // ignore program name for now

    SetTraceLogLevel(LOG_WARNING);
    SetTargetFPS(60);
    InitWindow(WIDTH, HEIGHT, "Stereograms");

    load_image_if_exists(&depth_map, &depth_map_texture, shift_args(&argc, &argv));
    load_image_if_exists(&pattern, &pattern_texture, shift_args(&argc, &argv));

    font = GetFontDefault();
    
    bool exit = false;

    while (!WindowShouldClose() && !exit) {
        BeginDrawing();
        start_tooltip_frame();
            ClearBackground(BACKGROUND_COLOR);
            switch (state) {
                case STATE_SHOW_AUTOSTEREOGRAM: {
                    draw_autostereogram_screen();
                } break;
                case STATE_DRAG_AND_DROP: {
                    draw_drag_and_drop_screen();
                } break;
                case STATE_EXIT: {
                    exit = true;
                } break;
                default: break;
            }
            draw_alert();
        end_tooltip_frame();
        EndDrawing();
    }

    UnloadImage(depth_map);
    UnloadImage(pattern);
    UnloadImage(output);
    UnloadTexture(texture);
    UnloadTexture(depth_map_texture);
    UnloadTexture(pattern_texture);

    CloseWindow();

    return 0;
}
