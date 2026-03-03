#include "MinTur.h"
#include "stops_nearby_window.h"
#include "stop_detail_window.h"

static bool s_js_ready;

static AppTimer *s_timeout_timer;

bool comm_is_js_ready() {
	return s_js_ready;
}

void set_timeout_timer(int timeout_ms, AppTimerCallback callback) {
	s_timeout_timer = app_timer_register(timeout_ms, callback, NULL);
}

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
		stops_nearby_handle_message(iterator);
		break;
	case POST_QUAY_DATA:
		quay_data_handle_message(iterator);
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
	if (s_js_ready && s_timeout_timer != NULL) {
		app_timer_cancel(s_timeout_timer);
		s_timeout_timer = NULL;
	}
	APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}


static void init(void) {
	app_message_register_inbox_received(inbox_received_callback);
	app_message_register_inbox_dropped(inbox_dropped_callback);
	app_message_register_outbox_failed(outbox_failed_callback);
	app_message_register_outbox_sent(outbox_sent_callback);

	const int inbox_size = 512;
	const int outbox_size = 512;
	app_message_open(inbox_size, outbox_size);

	Window *stops_nearby_window = stops_nearby_window_create();
	stop_detail_window_create();

	const bool animated = true;
	window_stack_push(stops_nearby_window, animated);
}

static void deinit(void) {
	stops_nearby_window_destroy();
	stop_detail_window_destroy();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
