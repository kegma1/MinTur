#include "MinTur.h"
#include "stops_nearby_window.h"
#include "stop_detail_window.h"

static Window *s_stops_nearby_window;

static MenuLayer *s_menu_layer;

static Stop nearby_stops[MAX_STOPS_NEARBY];
static int nearby_stop_count = 0;


static void select_callback(struct MenuLayer *s_menu_layer, MenuIndex *cell_index, void *callback_context) {
    stop_detail_set_stop(&nearby_stops[cell_index->row]);
	Window* stop_detail_window = stop_detail_window_get();
	window_stack_push(stop_detail_window, true);
}

static uint16_t get_sections_count_callback(struct MenuLayer *menulayer, uint16_t section_index, void *callback_context) {
    return nearby_stop_count;
}

#ifdef PBL_ROUND
static int16_t get_cell_height_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
  return 60;
}
#endif

static void draw_row_handler(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
  Stop *ns = &nearby_stops[cell_index->row];

  char dist_as_string[10];

  snprintf(dist_as_string, sizeof(dist_as_string), "(%dm)", ns->distance);


  menu_cell_basic_draw(ctx, cell_layer, ns->name, dist_as_string, NULL);
}

static void request_nearby_stops_timout_timer_handler(void *context);
static void request_nearby_stops(void);


static void request_nearby_stops() {
	APP_LOG(APP_LOG_LEVEL_INFO, "sending request");

	DictionaryIterator *iter;
	AppMessageResult result = app_message_outbox_begin(&iter);

	if (result == APP_MSG_OK) {
		dict_write_int8(iter, MESSAGE_KEY_MSG_TYPE, REQUEST_NEARBY_STOPS);

		result = app_message_outbox_send();
		if (result != APP_MSG_OK) {
			APP_LOG(APP_LOG_LEVEL_ERROR, "Error sending the outbox: %d", (int)result);
		}

		const int inertval_ms = 1000;
		set_timeout_timer(inertval_ms, request_nearby_stops_timout_timer_handler);
	} else {
		APP_LOG(APP_LOG_LEVEL_ERROR, "Error preparing the outbox: %d", (int)result);
	}
}

static void request_nearby_stops_timout_timer_handler(void *context) {
	request_nearby_stops();
}


static void stops_nearby_window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    s_menu_layer = menu_layer_create(bounds);
    menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks){
        .get_num_rows = get_sections_count_callback,
        .get_cell_height = PBL_IF_ROUND_ELSE(get_cell_height_callback, NULL),
        .draw_row = draw_row_handler,
        .select_click = select_callback
    });
    menu_layer_set_click_config_onto_window(s_menu_layer, window);
    layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));

    request_nearby_stops();
}

static void stops_nearby_window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
}

void stops_nearby_handle_message(DictionaryIterator *iterator) {
	Tuple *index_t = dict_find(iterator, MESSAGE_KEY_STOP_INDEX);
	Tuple *name_t = dict_find(iterator, MESSAGE_KEY_STOP_NAME);
	Tuple *dist_t = dict_find(iterator, MESSAGE_KEY_DISTANCE);

	if (index_t && name_t && dist_t) {
		int index = index_t->value->int32;

		if (index < 0 || index >= MAX_STOPS_NEARBY) {
			return;
		}

		nearby_stops[index].index = index;
		nearby_stops[index].distance = dist_t->value->int32;

		snprintf(nearby_stops[index].name, sizeof(nearby_stops[index].name), "%s", name_t->value->cstring);

		if (index + 1 > nearby_stop_count) {
			nearby_stop_count = index + 1;
		}

		menu_layer_reload_data(s_menu_layer);

	}
}


Window* stops_nearby_window_create(void) {
    Window *window = window_create();
    window_set_window_handlers(window, (WindowHandlers) {
        .load = stops_nearby_window_load,
        .unload = stops_nearby_window_unload,
    });

	s_stops_nearby_window = window;

    return window;
}

void stops_nearby_window_destroy() {
    window_destroy(s_stops_nearby_window);
	s_stops_nearby_window = NULL;
}

Window* stops_nearby_window_get(void) {
	return s_stops_nearby_window;
}