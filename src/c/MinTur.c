#include <pebble.h>
static Window *s_stops_nearby_window;
static MenuLayer *s_menu_layer;

#define MAX_STOPS_NEARBY 10

static bool s_js_ready;

static AppTimer *s_timeout_timer;

typedef struct {
  int index;
  char name[32];
  int distance;
} NearbyStop;

typedef enum {
	REQUEST_NEARBY_STOPS = 1,
	REQUEST_STOP_DETAILS,
	POST_NEARBY_STOP,
} MessageTypes;

static NearbyStop nearby_stops[MAX_STOPS_NEARBY];
static int nearby_stop_count = 0;

// ------------------------------------ Nearby stops ------------------------------------------------
static void select_callback(struct MenuLayer *s_menu_layer, MenuIndex *cell_index, void *callback_context) {
    
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
  NearbyStop *ns = &nearby_stops[cell_index->row];

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
		s_timeout_timer = app_timer_register(inertval_ms, request_nearby_stops_timout_timer_handler, NULL);
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

static void post_nearby_stop(DictionaryIterator *iterator) {
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

// ----------------------------------- main app -----------------------------------------------

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Message recived");

    Tuple *ready_t = dict_find(iterator, MESSAGE_KEY_JS_READY);

    if (ready_t) {
        s_js_ready = true;
        APP_LOG(APP_LOG_LEVEL_INFO, "JS is ready");
    }

	Tuple *message_type_t = dict_find(iterator, MESSAGE_KEY_MSG_TYPE);
	if (!message_type_t)
		return;

	switch (message_type_t->value->int32) {
	case POST_NEARBY_STOP:
		post_nearby_stop(iterator);
		break;
	
	default:
		APP_LOG(APP_LOG_LEVEL_WARNING, "Unsupported message type");
		break;
	}

    
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
	if (s_js_ready) {
		app_timer_cancel(s_timeout_timer);
	}
	APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}


static void init(void) {
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  const int inbox_size = 500;
  const int outbox_size = 128;
  app_message_open(inbox_size, outbox_size);

  s_stops_nearby_window = window_create();
  window_set_window_handlers(s_stops_nearby_window, (WindowHandlers) {
    .load = stops_nearby_window_load,
    .unload = stops_nearby_window_unload,
  });

  const bool animated = true;
  window_stack_push(s_stops_nearby_window, animated);
}

static void deinit(void) {
  window_destroy(s_stops_nearby_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
