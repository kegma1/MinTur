#include "stop_detail_window.h"

static const int16_t MARGIN = 8;

static Window *s_stop_detail_window;

static TextLayer *s_title_layer;
static Layer *s_devider_layer;

Stop *current_stop;


void stop_detail_set_stop(Stop *ns) {
    current_stop = ns;
}

void stop_detail_handle_message(DictionaryIterator *iterator) {
}

static void devider_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    
    const int16_t yy = 11;    

    graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorWhite, GColorBlack));
    graphics_draw_line(ctx, GPoint(0, yy), GPoint(bounds.size.w, yy));
}

static void stop_detail_window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
    
    s_title_layer = text_layer_create(GRect(0, 32, bounds.size.w, 20));
    text_layer_set_text(s_title_layer, current_stop->name);
    text_layer_set_text_alignment(s_title_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_title_layer));

    s_devider_layer = layer_create(GRect(MARGIN, 40, bounds.size.w - 2 * MARGIN, 20));
    layer_set_update_proc(s_devider_layer, devider_update_proc);
    layer_add_child(window_layer, s_devider_layer);    
}

static void stop_detail_window_unload(Window *window) {

}

Window *stop_detail_window_create(void) {
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

