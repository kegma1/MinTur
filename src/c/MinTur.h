#pragma once
#include <pebble.h>

typedef enum {
	REQUEST_NEARBY_STOPS = 1,
	REQUEST_STOP_DETAILS,
	POST_NEARBY_STOP,
} MessageTypes;

bool comm_is_js_ready();
void set_timeout_timer(int timeout_ms, AppTimerCallback callback);