#include <pebble.h>
#include <inttypes.h>

// These cannot be assigned to const
#define CADENCE_LOW GColorDarkCandyAppleRed
#define CADENCE_GOOD GColorIslamicGreen
#define CADENCE_HIGH GColorDukeBlue

static const uint32_t const VIBE_PATTERN_CADENCE_LOW[] = { 100, 100, 100, 100, 400 };
static const uint32_t const VIBE_PATTERN_CADENCE_HIGH[] = { 100, 100, 100, 100, 100 };
static const uint32_t const VIBE_PATTERN_CADENCE_GOOD[] = { 100, 100, 100 };

static Window *s_main_window;
static TextLayer *s_speed_layer;
static TextLayer *s_speed_unit_layer;
static TextLayer *s_cadence_layer;
static TextLayer *s_cadence_unit_layer;

static void inbox_received_callback(DictionaryIterator *iter, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Received message from phone. Size: %"PRIu32, dict_size(iter));
  
  Tuple *speed_tuple = dict_find(iter, MESSAGE_KEY_Speed);
  if(speed_tuple) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Updating speed: %s", speed_tuple->value->cstring);
    text_layer_set_text(s_speed_layer, speed_tuple->value->cstring);
  }
  
  Tuple *cadence_tuple = dict_find(iter, MESSAGE_KEY_Cadence);
  if(cadence_tuple) {
    static char s_buffer[4];
    snprintf(s_buffer, sizeof(s_buffer), "%d", (int)cadence_tuple->value->int32);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Updating cadence: %s", s_buffer);
    text_layer_set_text(s_cadence_layer, s_buffer);
  }
  
  Tuple *vibe_tuple = dict_find(iter, MESSAGE_KEY_VibePattern);
  if(vibe_tuple) {
    if(vibe_tuple->value->int8 == 1) {
      VibePattern pat = {
        .durations = VIBE_PATTERN_CADENCE_LOW,
        .num_segments = ARRAY_LENGTH(VIBE_PATTERN_CADENCE_LOW),
      };
      vibes_enqueue_custom_pattern(pat);
      text_layer_set_background_color(s_cadence_layer, CADENCE_LOW);
    }
    else if(vibe_tuple->value->int8 == 2) {
      VibePattern pat = {
        .durations = VIBE_PATTERN_CADENCE_HIGH,
        .num_segments = ARRAY_LENGTH(VIBE_PATTERN_CADENCE_HIGH),
      };
      vibes_enqueue_custom_pattern(pat);
      text_layer_set_background_color(s_cadence_layer, CADENCE_HIGH);
    }
    else if(vibe_tuple->value->int8 == 3) {
      VibePattern pat = {
        .durations = VIBE_PATTERN_CADENCE_GOOD,
        .num_segments = ARRAY_LENGTH(VIBE_PATTERN_CADENCE_GOOD),
      };
      vibes_enqueue_custom_pattern(pat);
      text_layer_set_background_color(s_cadence_layer, CADENCE_GOOD);
    }
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped. Reason: %d", (int)reason);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_speed_layer = text_layer_create(GRect(0, 0, bounds.size.w, bounds.size.h / 2));
  text_layer_set_background_color(s_speed_layer, GColorIslamicGreen);
  text_layer_set_text_color(s_speed_layer, GColorWhite);
  text_layer_set_text(s_speed_layer, "0.0");
  text_layer_set_font(s_speed_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_speed_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_speed_layer));
  
  s_speed_unit_layer = text_layer_create(GRect(0, 50, bounds.size.w, 20));
  text_layer_set_background_color(s_speed_unit_layer, GColorClear);
  text_layer_set_text_color(s_speed_unit_layer, GColorWhite);
  text_layer_set_text(s_speed_unit_layer, "km/h");
  text_layer_set_font(s_speed_unit_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_speed_unit_layer, GTextAlignmentCenter);
  layer_add_child((Layer *)s_speed_layer, text_layer_get_layer(s_speed_unit_layer));
  
  s_cadence_layer = text_layer_create(GRect(0, bounds.size.h / 2, bounds.size.w, bounds.size.h / 2));
  text_layer_set_background_color(s_cadence_layer, CADENCE_LOW); // Bike is stopped when app is started
  text_layer_set_text_color(s_cadence_layer, GColorWhite);
  text_layer_set_text(s_cadence_layer, "0");
  text_layer_set_font(s_cadence_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_cadence_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_cadence_layer));
  
  s_cadence_unit_layer = text_layer_create(GRect(0, 50, bounds.size.w, 20));
  text_layer_set_background_color(s_cadence_unit_layer, GColorClear);
  text_layer_set_text_color(s_cadence_unit_layer, GColorWhite);
  text_layer_set_text(s_cadence_unit_layer, "RPM");
  text_layer_set_font(s_cadence_unit_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_cadence_unit_layer, GTextAlignmentCenter);
  layer_add_child((Layer *)s_cadence_layer, text_layer_get_layer(s_cadence_unit_layer));
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_speed_layer);
  text_layer_destroy(s_speed_unit_layer);
  text_layer_destroy(s_cadence_layer);
  text_layer_destroy(s_cadence_unit_layer);
}


static void init() {
  s_main_window = window_create();
  
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  window_stack_push(s_main_window, true);
  
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  
  AppMessageResult res = app_message_open(64, 64);
  if(res != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to open message buffer: %i", res);
  }
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Speed key: %"PRIu32,  MESSAGE_KEY_Speed);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Cadence key: %"PRIu32,  MESSAGE_KEY_Cadence);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}