#include "stop_detail_window.h"
#include "MinTur.h"

static const int16_t MARGIN = 8;

static Window *s_stop_detail_window;

static TextLayer *s_title_layer;
static Layer *s_devider_layer;
static Layer *s_icon_layer;

static GDrawCommandImage *s_mode_icons[5];

Stop *current_stop;

typedef enum {
    MODE_BUS,
    MODE_TRAM,
    MODE_METRO,
    MODE_RAIL,
    MODE_WATER,
    MODE_UNKNOWN
} TransportMode;

TransportMode parse_mode(const char *mode_str) {
    if (strcmp(mode_str, "bus")   == 0) return MODE_BUS;
    if (strcmp(mode_str, "water") == 0) return MODE_WATER;
    if (strcmp(mode_str, "rail")  == 0) return MODE_RAIL;
    if (strcmp(mode_str, "tram")  == 0) return MODE_TRAM;
    if (strcmp(mode_str, "metro") == 0) return MODE_METRO;
    return MODE_UNKNOWN;
}

#define MAX_LINES_PER_MODE 10
#define MAX_LINE_CODE_LEN 8

typedef struct {
    TransportMode mode;
    char line_codes[MAX_LINES_PER_MODE][MAX_LINE_CODE_LEN];
    int count;
} ModeLines;

static ModeLines line_data[5] = {
    {MODE_BUS, {}, 0},
    {MODE_TRAM, {}, 0},
    {MODE_METRO, {}, 0},
    {MODE_RAIL, {}, 0},
    {MODE_WATER, {}, 0},
};

void reset_line_data() {
    for (int i = 0; i < MODE_UNKNOWN; i++) {
        line_data[i].count = 0;
    }
}

void stop_detail_set_stop(Stop *ns) {
    current_stop = ns;
}

void stop_detail_handle_message(DictionaryIterator *iterator) {
}


void line_data_handle_message(DictionaryIterator *iterator) {
    Tuple *line_code_t = dict_find(iterator, MESSAGE_KEY_LINE_CODE);
	Tuple *transport_mode_t = dict_find(iterator, MESSAGE_KEY_LINE_TRANSPORT_MODE);
    Tuple *done_sending_t = dict_find(iterator, MESSAGE_KEY_DONE_SENDING);

    if (line_code_t && transport_mode_t && done_sending_t) {
        
        TransportMode mode = parse_mode(transport_mode_t->value->cstring);
        if (mode == MODE_UNKNOWN) return;
        if (line_data[mode].count == MAX_LINES_PER_MODE) return;

        int i = line_data[mode].count;

        snprintf(line_data[mode].line_codes[i], sizeof(line_data[mode].line_codes[i]), "%s", line_code_t->value->cstring);
        // APP_LOG(APP_LOG_LEVEL_INFO, "%s, %d, %d", line_data[mode].line_codes[i], line_data[mode].count, line_data[mode].mode);

        line_data[mode].count += 1;
        
        if (done_sending_t->value->int8 == 1) {
            layer_mark_dirty(s_icon_layer);
        }
    }
}

static void request_lines_per_transportMode_timout_timer_handler(void *context);
static void request_lines_per_transportMode(void);

static void request_lines_per_transportMode() {
	APP_LOG(APP_LOG_LEVEL_INFO, "sending request for lines");

    reset_line_data();

	DictionaryIterator *iter;
	AppMessageResult result = app_message_outbox_begin(&iter);

	if (result == APP_MSG_OK) {
		dict_write_int8(iter, MESSAGE_KEY_MSG_TYPE, REQUEST_NEARBY_LINES_PER_TRANSPORT_MODE);
		dict_write_int8(iter, MESSAGE_KEY_STOP_INDEX, current_stop->index);

        const int inertval_ms = 1000;
		set_timeout_timer(inertval_ms, request_lines_per_transportMode_timout_timer_handler);

		result = app_message_outbox_send();
		if (result != APP_MSG_OK) {
			APP_LOG(APP_LOG_LEVEL_ERROR, "Error sending the outbox: %d", (int)result);
		}
	} else {
		APP_LOG(APP_LOG_LEVEL_ERROR, "Error preparing the outbox: %d", (int)result);
	}
}


static void request_lines_per_transportMode_timout_timer_handler(void *context) {
    request_lines_per_transportMode();
}



static void devider_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    
    const int16_t yy = 11;    

    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_draw_line(ctx, GPoint(0, yy), GPoint(bounds.size.w, yy));
}

void draw_line_code(GContext *ctx, char *line_code, GRect pos) {
    GSize text_size = graphics_text_layout_get_content_size (
        line_code, 
        fonts_get_system_font(FONT_KEY_GOTHIC_14), 
        pos,
        GTextOverflowModeTrailingEllipsis,
        GTextAlignmentCenter
    );

    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_context_set_text_color(ctx, GColorWhite);
    GRect background_size = GRect(pos.origin.x, pos.origin.y, text_size.w + 2, text_size.h + 1);

    graphics_fill_rect(ctx, background_size, 1, GCornersAll);

    graphics_draw_text(
        ctx, 
        line_code, 
        fonts_get_system_font(FONT_KEY_GOTHIC_14), 
        GRect(pos.origin.x + 1, pos.origin.y - 2, text_size.w, text_size.h),
        GTextOverflowModeTrailingEllipsis,
        GTextAlignmentCenter,
        NULL
    );
}

GSize get_line_code_size(GContext *ctx, char *line_code, GRect pos) {
    
    GSize text_size = graphics_text_layout_get_content_size (
        line_code, 
        fonts_get_system_font(FONT_KEY_GOTHIC_14), 
        pos,
        GTextOverflowModeTrailingEllipsis,
        GTextAlignmentCenter
    );

    GRect background_size = GRect(pos.origin.x, pos.origin.y, text_size.w + 2, text_size.h + 1);

    return background_size.size;
}

static void icon_layer_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);

    GPoint offset = GPoint(0, 0);
    
    for (int mode = 0; mode < MODE_UNKNOWN; mode++) {
        if (line_data[mode].count > 0) {
            // gdraw_command_image_draw(ctx, s_mode_icons[mode], GPoint(0, offset));

            // graphics_context_set_fill_color(ctx, GColorBlack);
            graphics_context_set_text_color(ctx, GColorBlack);

            for (int i = 0; i < line_data[mode].count; i++) {
                GSize line_code_size = get_line_code_size(
                    ctx, 
                    line_data[mode].line_codes[i], 
                    GRect(offset.x, offset.y, bounds.size.w - 27, 25)
                );

                if (offset.x + line_code_size.w > bounds.size.w) {
                    offset.x = 0;
                    offset.y += line_code_size.h + 1;
                }

                draw_line_code (
                    ctx, 
                    line_data[mode].line_codes[i], 
                    GRect(offset.x, offset.y, bounds.size.w - 27, 25) 
                );

                offset.x += line_code_size.w + 1;
                if (offset.x > bounds.size.w) {
                    offset.x = 0;
                    offset.y += line_code_size.h + 1;
                }
            }
        }
    }
}


static void stop_detail_window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    s_icon_layer = layer_create(GRect(MARGIN, MARGIN*4 + 25, bounds.size.w - 2 * MARGIN, bounds.size.h));
    layer_set_update_proc(s_icon_layer, icon_layer_update_proc);

    layer_add_child(window_layer, s_icon_layer);

    s_title_layer = text_layer_create(GRect(MARGIN, MARGIN, bounds.size.w - 2 * MARGIN, 20));
    text_layer_set_text(s_title_layer, current_stop->name);
    layer_add_child(window_layer, text_layer_get_layer(s_title_layer));

    s_devider_layer = layer_create(GRect(MARGIN, MARGIN + MARGIN, bounds.size.w - 2 * MARGIN, 20));
    layer_set_update_proc(s_devider_layer, devider_update_proc);
    layer_add_child(window_layer, s_devider_layer);
    
    request_lines_per_transportMode();
}

static void stop_detail_window_unload(Window *window) {
    text_layer_destroy(s_title_layer);
    layer_destroy(s_devider_layer);
}

Window *stop_detail_window_create(void) {
    s_mode_icons[MODE_BUS]   = gdraw_command_image_create_with_resource(RESOURCE_ID_ICON_BUS);
    s_mode_icons[MODE_WATER] = gdraw_command_image_create_with_resource(RESOURCE_ID_ICON_FERRY);
    s_mode_icons[MODE_TRAM]  = gdraw_command_image_create_with_resource(RESOURCE_ID_ICON_TRAM);
    s_mode_icons[MODE_RAIL]  = gdraw_command_image_create_with_resource(RESOURCE_ID_ICON_TRAIN);
    s_mode_icons[MODE_METRO] = gdraw_command_image_create_with_resource(RESOURCE_ID_ICON_METRO);

    Window *window = window_create();
    window_set_window_handlers(window, (WindowHandlers) {
        .load = stop_detail_window_load,
        .unload = stop_detail_window_unload,
    });
    
    s_stop_detail_window = window;

    return window;
}

void stop_detail_window_destroy() {
    window_destroy(s_stop_detail_window);
    s_stop_detail_window = NULL;
}

Window *stop_detail_window_get() {
    return s_stop_detail_window;
}

