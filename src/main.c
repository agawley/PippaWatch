#include <pebble.h>
#include "effect_layer.h"

#define KEY_INVERTED 0

Window *window;
TextLayer *battery_layer;
TextLayer *day_layer;
TextLayer *time_layer;
TextLayer *date_layer;
BitmapLayer *bt_icon_layer;
EffectLayer* inverter_layer;

GFont helv_bold_lg;
GFont helv_bold_sm;
GFont helv_bold_xsm;
GFont helv_xsm;

GBitmap *bt_icon_bitmap;

int SCREEN_WIDTH = PBL_IF_RECT_ELSE(144, 180);
int SCREEN_HEIGHT = PBL_IF_RECT_ELSE(168, 180);

bool inverted = false;

void handle_time_change(struct tm *tick_time, TimeUnits units_changed) {
    
  static char time_buffer[10];
  static char temp_hour[10];
  static char day_buffer[10];
  static char date_buffer[20];
  int hr;
  
  if (clock_is_24h_style()) {
    strftime(time_buffer, sizeof(time_buffer), "%H:%M", tick_time);
  } else {
    strftime(time_buffer, sizeof(time_buffer), "%l:%M", tick_time);
    // this little section strips the leading whitespace from timebuffer when hr < 10
    strftime(temp_hour, sizeof(time_buffer), "%I", tick_time);
    hr = atoi(temp_hour);
    if (hr < 10) {
      memmove(time_buffer, time_buffer + 1, strlen(time_buffer));
    }
  }
  text_layer_set_text(time_layer, time_buffer);

  if (units_changed & DAY_UNIT) {
    strftime(day_buffer, sizeof(day_buffer), "%A", tick_time);
    text_layer_set_text(day_layer, day_buffer);
    strftime(date_buffer, sizeof(date_buffer), "%B %e", tick_time);
    text_layer_set_text(date_layer, date_buffer);
  }
  
}

void handle_bt_change(bool connected) {  
  // Show icon if disconnected
  layer_set_hidden(bitmap_layer_get_layer(bt_icon_layer), connected);

  if(!connected) {
    // Issue a vibrating alert
    vibes_double_pulse();
  }
}

void handle_battery_change(BatteryChargeState charge_state) {
  static char s_battery_buffer[20];

  if (charge_state.is_charging) {
    snprintf(s_battery_buffer, sizeof(s_battery_buffer), "chg %d%%", charge_state.charge_percent);
  } else {
    snprintf(s_battery_buffer, sizeof(s_battery_buffer), "%d%%", charge_state.charge_percent);
  }
  text_layer_set_text(battery_layer, s_battery_buffer);
  if (charge_state.charge_percent <= 10) {
    vibes_short_pulse();
    text_layer_set_text_color(battery_layer, COLOR_FALLBACK(GColorRed, GColorBlack));
    text_layer_set_font(battery_layer, helv_bold_xsm);
  } else {
    text_layer_set_text_color(battery_layer, COLOR_FALLBACK(GColorDarkGray, GColorBlack));
    text_layer_set_font(battery_layer, helv_xsm);
  }
}

void possibly_invert() {
  if (inverted) {
    layer_add_child(window_get_root_layer(window), effect_layer_get_layer(inverter_layer));
  } else {
    layer_remove_from_parent(effect_layer_get_layer(inverter_layer));
  }
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  Tuple *data = dict_find(iterator, KEY_INVERTED);
  if (data) {
    inverted = data->value->uint8;
    possibly_invert();
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}


void handle_init(void) {
  
  int MARGIN = PBL_IF_RECT_ELSE(2, 8);
  
  int time_height = 60;
  int time_y_pos = (SCREEN_HEIGHT / 2) - (time_height / 2); // vertically centered
  int day_height = 20;
  int day_y_pos = time_y_pos - day_height; // above time
  int date_height = 20;
  int date_y_pos = time_y_pos + time_height; // below time
  int battery_height = 24;
  int battery_x_pos = MARGIN;
  int battery_y_pos = PBL_IF_RECT_ELSE(MARGIN, SCREEN_HEIGHT - battery_height);
  int battery_width = SCREEN_WIDTH - (MARGIN * 2);
  int bt_height = 16;
  int bt_width = 16;
  int bt_x_pos = PBL_IF_RECT_ELSE(MARGIN, (SCREEN_WIDTH / 2) - (bt_width / 2));
  int bt_y_pos = MARGIN;
  
  window = window_create();
  
  // Get fonts
  helv_xsm = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_HELV_12));
  helv_bold_xsm = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_HELV_BOLD_12));
  helv_bold_sm = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_HELV_BOLD_16));
  helv_bold_lg = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_HELV_BOLD_SUBSET_48));
  
  // Create the battery layer
  battery_layer = text_layer_create(GRect(battery_x_pos, battery_y_pos, battery_width, battery_height));
  text_layer_set_text(battery_layer, "N/A");
  text_layer_set_background_color(battery_layer, GColorClear);
  text_layer_set_font(battery_layer, helv_xsm);
  text_layer_set_text_color(battery_layer, COLOR_FALLBACK(GColorDarkGray, GColorBlack));
  text_layer_set_text_alignment(battery_layer, PBL_IF_RECT_ELSE(GTextAlignmentRight, GTextAlignmentCenter));
  
  // Create the BT layer
  bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_ICON);
  bt_icon_layer = bitmap_layer_create(GRect(bt_x_pos, bt_y_pos, bt_width, bt_height));
  bitmap_layer_set_bitmap(bt_icon_layer, bt_icon_bitmap);
  
  // Create the day layer
  day_layer = text_layer_create(GRect(0, day_y_pos, SCREEN_WIDTH, day_height));
  text_layer_set_text(day_layer, "No time!");
  text_layer_set_font(day_layer, helv_bold_sm);
  text_layer_set_text_color(day_layer, COLOR_FALLBACK(GColorDarkGray, GColorBlack));
  text_layer_set_text_alignment(day_layer, GTextAlignmentCenter);
  
  // Create the time layer
  time_layer = text_layer_create(GRect(0, time_y_pos, SCREEN_WIDTH, time_height));
  text_layer_set_text(time_layer, "No time!");
  text_layer_set_background_color(time_layer, GColorClear);
  text_layer_set_font(time_layer, helv_bold_lg);
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
  
  // Create the date layer
  date_layer = text_layer_create(GRect(0, date_y_pos, SCREEN_WIDTH, date_height));
  text_layer_set_text(date_layer, "No time!");
  text_layer_set_background_color(date_layer, GColorClear);
  text_layer_set_text_color(date_layer, COLOR_FALLBACK(GColorDarkGray, GColorBlack));
  text_layer_set_font(date_layer, helv_bold_sm);
  text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
  
  // Create the inverter layer
  inverter_layer = effect_layer_create(GRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT));
  effect_layer_add_effect(inverter_layer, effect_invert, NULL);
    
  // Add the layers to the window
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(bt_icon_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(battery_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(day_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(time_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(date_layer));
  
  // Load inverted key
  if (persist_exists(KEY_INVERTED)) {
    inverted = persist_read_bool(KEY_INVERTED);
  }
  if (inverted) {
    layer_add_child(window_get_root_layer(window), effect_layer_get_layer(inverter_layer));
  }
  
  // set the time, BT
  time_t now = time(NULL);
  handle_time_change(localtime(&now), MINUTE_UNIT | DAY_UNIT);
  handle_bt_change(connection_service_peek_pebble_app_connection());
  handle_battery_change(battery_state_service_peek());
  
  // Push the window
  window_stack_push(window, true);
  
  // subscribe to services
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = handle_bt_change
  });
  battery_state_service_subscribe(handle_battery_change);
  tick_timer_service_subscribe(MINUTE_UNIT, handle_time_change);  
  
  // Register message callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Open AppMessage with sensible buffer sizes
  app_message_open(64, 64);
  
}

void handle_deinit(void) {
  
  // destroy layers
  text_layer_destroy(day_layer);
  text_layer_destroy(time_layer);
  text_layer_destroy(date_layer);
  text_layer_destroy(battery_layer);
  effect_layer_destroy(inverter_layer);

  gbitmap_destroy(bt_icon_bitmap);
  bitmap_layer_destroy(bt_icon_layer);
  
  // unsub services
  battery_state_service_unsubscribe();
  connection_service_unsubscribe();
  tick_timer_service_unsubscribe();
  
  // Unload fonts
  fonts_unload_custom_font(helv_bold_lg);
  fonts_unload_custom_font(helv_bold_sm);
  fonts_unload_custom_font(helv_xsm);
  fonts_unload_custom_font(helv_bold_xsm);
    
  // save state
  persist_write_bool(KEY_INVERTED, inverted);
  
  window_destroy(window);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
