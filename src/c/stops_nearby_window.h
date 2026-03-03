#pragma once
#include <pebble.h>

#define MAX_STOPS_NEARBY 10

typedef struct {
  int index;
  // char id[32];
  int distance;
  int quay_count;
  char name[32];
  // float lon;
  // float lat;

} Stop;


Window* stops_nearby_window_create(void);
void stops_nearby_window_destroy(void);
Window* stops_nearby_window_get(void);

void stops_nearby_handle_message(DictionaryIterator *iterator);

