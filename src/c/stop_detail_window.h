#pragma once
#include <pebble.h>
#include "stops_nearby_window.h"

Window* stop_detail_window_create(void);
void stop_detail_window_destroy(void);
Window* stop_detail_window_get(void);

void stop_detail_set_stop(Stop* ns);

void stop_detail_handle_message(DictionaryIterator *iterator);
void line_data_handle_message(DictionaryIterator *iterator);
void stop_detail_request(void);

