#pragma once
#include <pebble.h>

#define MAX_STOPS_NEARBY 10

typedef struct {
  int index;
  char name[32];
  int distance;
} NearbyStop;


Window* stops_nearby_window_create(void);
void stops_nearby_window_destroy(Window *window);

void stops_nearby_handle_message(DictionaryIterator *iterator);
void stops_nearby_request(void);
