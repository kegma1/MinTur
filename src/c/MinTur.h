#pragma once
#include <pebble.h>

typedef enum {
	REQUEST_NEARBY_STOPS = 1,
	REQUEST_STOP_DETAILS,
	REQUEST_QUAY,
	REQUEST_CALL,
	POST_NEARBY_STOP,
	POST_QUAY_DATA,
	POST_CALL,
} MessageTypes;

bool comm_is_js_ready();
void set_timeout_timer(int timeout_ms, AppTimerCallback callback);
