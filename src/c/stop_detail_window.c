#include "stop_detail_window.h"
#include "MinTur.h"

static const int16_t MARGIN = 8;

static Window *s_stop_detail_window;

static TextLayer *s_title_layer;
static TextLayer *s_description_layer;
static Layer *s_devider_layer;

static Layer *s_content_layer;

Stop *current_stop;


typedef struct {
    int index;
    char code[8];
    char id[16];
    char name[32];
    char desc[32];
} Quay;

Quay current_quay;

void stop_detail_set_stop_and_index(Stop *ns, int quay_index) {
    current_stop = ns;
    current_quay.index = quay_index;
}

void stop_detail_handle_message(DictionaryIterator *iterator) {
}


static void update_title_layer() {
    static char title_text[64];
    snprintf(title_text, sizeof(title_text), "%s %s", current_quay.name, current_quay.code);
    text_layer_set_text(s_title_layer, title_text);
}

void quay_data_handle_message(DictionaryIterator *iterator) {
    APP_LOG(APP_LOG_LEVEL_INFO, "handeling quay data");
    Tuple *quay_index_t = dict_find(iterator, MESSAGE_KEY_QUAY_INDEX);
    Tuple *quay_id_t    = dict_find(iterator, MESSAGE_KEY_QUAY_ID);
    Tuple *quay_name_t  = dict_find(iterator, MESSAGE_KEY_QUAY_NAME);
    Tuple *quay_code_t  = dict_find(iterator, MESSAGE_KEY_QUAY_CODE);
    Tuple *quay_desc_t  = dict_find(iterator, MESSAGE_KEY_QUAY_DESC);

    if (quay_index_t && quay_id_t && quay_name_t && quay_code_t && quay_desc_t) {
        current_quay.index = quay_index_t->value->int8;
        
        snprintf(current_quay.id,   sizeof(current_quay.id),   "%s", quay_id_t->value->cstring);
        snprintf(current_quay.name, sizeof(current_quay.name), "%s", quay_name_t->value->cstring);
        snprintf(current_quay.code, sizeof(current_quay.code), "%s", quay_code_t->value->cstring);
        snprintf(current_quay.desc, sizeof(current_quay.desc), "%s", quay_desc_t->value->cstring);

        layer_mark_dirty(s_content_layer);
    }   
}

static void request_quay(int stop_id, int quay_id) {
    APP_LOG(APP_LOG_LEVEL_INFO, "sending request for quay [%d, %d]", stop_id, quay_id);

	DictionaryIterator *iter;
	AppMessageResult result = app_message_outbox_begin(&iter);

	if (result == APP_MSG_OK) {
		dict_write_int8(iter, MESSAGE_KEY_MSG_TYPE, REQUEST_QUAY);
		dict_write_int8(iter, MESSAGE_KEY_STOP_INDEX, stop_id);
        dict_write_int8(iter, MESSAGE_KEY_QUAY_INDEX, quay_id);

		result = app_message_outbox_send();
		if (result != APP_MSG_OK) {
			APP_LOG(APP_LOG_LEVEL_ERROR, "Error sending the outbox: %d", (int)result);
		}
	} else {
		APP_LOG(APP_LOG_LEVEL_ERROR, "Error preparing the outbox: %d", (int)result);
	}
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
    if (current_quay.index > 0) {
        request_quay(current_stop->index, current_quay.index - 1);
    } else {
        vibes_short_pulse();
    }
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
    if (current_quay.index + 1 < current_stop->quay_count) {
        request_quay(current_stop->index, current_quay.index + 1);
    } else {
        vibes_short_pulse();
    }
}

static void click_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
    window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void devider_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_draw_line(ctx, GPoint(0, 0), GPoint(bounds.size.w, 0));
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


static void content_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    update_title_layer();
    GSize text_size = text_layer_get_content_size(s_title_layer);

    int width = bounds.size.w - 2 * MARGIN;
    
    layer_set_frame(
        text_layer_get_layer(s_description_layer),
        GRect(MARGIN, text_size.h + 4, width, 20)
    );
    
    layer_set_frame(
        s_devider_layer,
        GRect(MARGIN, text_size.h + 4, width, 1)
    );
}

static void stop_detail_window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    s_content_layer = layer_create(bounds);
    layer_set_update_proc(s_content_layer, content_update_proc);
    layer_add_child(window_layer, s_content_layer);

    s_title_layer = text_layer_create(GRect(MARGIN, 0, bounds.size.w - 2 * MARGIN, 50));
    text_layer_set_font(s_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_overflow_mode(s_title_layer, GTextOverflowModeWordWrap); 

    s_description_layer = text_layer_create(GRect(0, 0, 0, 0));
    text_layer_set_font(s_description_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text(s_description_layer, current_quay.desc);

    s_devider_layer = layer_create(GRect(0, 0, 0, 0));
    layer_set_update_proc(s_devider_layer, devider_update_proc);

    layer_add_child(s_content_layer, text_layer_get_layer(s_title_layer));
    layer_add_child(s_content_layer, text_layer_get_layer(s_description_layer));
    layer_add_child(s_content_layer, s_devider_layer);

    request_quay(current_stop->index, current_quay.index);
}

static void stop_detail_window_unload(Window *window) {
    text_layer_destroy(s_title_layer);
    text_layer_destroy(s_description_layer);
    layer_destroy(s_devider_layer);
    layer_destroy(s_content_layer);
}

Window *stop_detail_window_create(void) {
    Window *window = window_create();

    window_set_click_config_provider(window, click_config_provider);

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

