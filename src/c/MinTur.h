#pragma once
#include <pebble.h>

typedef enum {
	REQUEST_NEARBY_STOPS = 1,
	REQUEST_STOP_DETAILS,
	REQUEST_NEARBY_LINES_PER_TRANSPORT_MODE,
	REQUEST_QUAY,
	POST_NEARBY_STOP,
	POST_LINE_DATA,
	POST_QUAY_DATA,
} MessageTypes;

bool comm_is_js_ready();
void set_timeout_timer(int timeout_ms, AppTimerCallback callback);