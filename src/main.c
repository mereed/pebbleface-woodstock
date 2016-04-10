/*
Copyright (C) 2015 Mark Reed

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <pebble.h>

static Window *s_main_window;

static TextLayer *s_time_layer;
static TextLayer *s_date_layer;

static GBitmap *s_bitmap = NULL;
static GBitmap *start_bitmap = NULL;
static BitmapLayer *s_bitmap_layer;
static BitmapLayer *start_bitmap_layer;
static GBitmapSequence *s_sequence = NULL;

static GBitmap *bluetooth_image;
static BitmapLayer *bluetooth_layer;

int charge_percent = 0;

TextLayer *battery_text_layer;


static void timer_handler(void *context) {
  uint32_t next_delay;

  // Advance to the next APNG frame
  if(gbitmap_sequence_update_bitmap_next_frame(s_sequence, s_bitmap, &next_delay)) {
    bitmap_layer_set_bitmap(s_bitmap_layer, s_bitmap);
    gbitmap_sequence_set_play_count(s_sequence, 1);  // Added so the animation only loops once
    layer_mark_dirty(bitmap_layer_get_layer(s_bitmap_layer));
    app_timer_register(next_delay, timer_handler, NULL); // Timer for that delay
	  
  } else {
    // Start again
    gbitmap_sequence_restart(s_sequence);
  }
}

static void update_datetime() {
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char time_buffer[] = "00:00";
  static char date_buffer[] = "Sun, Oct 25";

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    strftime(time_buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    // Use 12 hour format
    strftime(time_buffer, sizeof("00:00"), "%l:%M", tick_time);
  }
  strftime(date_buffer, sizeof("Sun, Oct 25"), "%a, %b %e", tick_time);

  // Display on the TextLayer
  text_layer_set_text(s_time_layer, time_buffer);
  text_layer_set_text(s_date_layer, date_buffer);
}

static void load_sequence() {
  if(s_sequence) {
    gbitmap_sequence_destroy(s_sequence);
    s_sequence = NULL;
  }
  if(s_bitmap) {
    gbitmap_destroy(s_bitmap);
    s_bitmap = NULL;
  }

  s_sequence = gbitmap_sequence_create_with_resource(RESOURCE_ID_ANIMATION);
  s_bitmap = gbitmap_create_blank(gbitmap_sequence_get_bitmap_size(s_sequence), GBitmapFormat8Bit);
  app_timer_register(1, timer_handler, NULL);

}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_datetime();
	
  if (units_changed & MINUTE_UNIT) {

     load_sequence();
  }	
}

void update_battery_state(BatteryChargeState charge_state) {
    static char battery_text[] = "x100%";

    if (charge_state.is_charging) {

        snprintf(battery_text, sizeof(battery_text), "+%d%%", charge_state.charge_percent);
    } else {
        snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
        
    } 
    charge_percent = charge_state.charge_percent;
    text_layer_set_text(battery_text_layer, battery_text);
} 

static void toggle_bluetooth_icon(bool connected) {

  layer_set_hidden(bitmap_layer_get_layer(bluetooth_layer), !connected);
}

void bluetooth_connection_callback(bool connected) {
  toggle_bluetooth_icon(connected);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
	

 // BitmapLayer - start frame
  start_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ANIMATIONSTART);
#ifdef PBL_PLATFORM_CHALK
  start_bitmap_layer = bitmap_layer_create(GRect(0, 72, 180, 90));
#else
  start_bitmap_layer = bitmap_layer_create(GRect(0, 78, 144, 90));
#endif
  bitmap_layer_set_bitmap(start_bitmap_layer, start_bitmap);
  bitmap_layer_set_background_color(start_bitmap_layer, GColorCeleste);
  layer_add_child(window_layer, bitmap_layer_get_layer(start_bitmap_layer));
	 
  // BitmapLayer - begin
#ifdef PBL_PLATFORM_CHALK
  s_bitmap_layer = bitmap_layer_create(GRect(0, 72, 180, 90));
#else
  s_bitmap_layer = bitmap_layer_create(GRect(0, 78, 144, 90));
#endif
  bitmap_layer_set_background_color(s_bitmap_layer, GColorClear);
  bitmap_layer_set_compositing_mode(s_bitmap_layer, GCompOpSet);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bitmap_layer));

	// Time - begin
#ifdef PBL_PLATFORM_CHALK
  s_time_layer = text_layer_create(GRect(0, 23, 180, 44));
#else
  s_time_layer = text_layer_create(GRect(0, 27, 144, 44));
#endif
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  // Date - begin
#ifdef PBL_PLATFORM_CHALK
  s_date_layer = text_layer_create(GRect(0, 11, 180, 30));
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
#else
  s_date_layer = text_layer_create(GRect(0, 8, 144, 30));
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
#endif
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_text(s_date_layer, "00:00");
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
	

#ifdef PBL_PLATFORM_CHALK
    battery_text_layer = text_layer_create(GRect(62, 160, 40, 20));
#else
    battery_text_layer = text_layer_create(GRect(102, -3, 40, 20));
#endif 	
	text_layer_set_font(battery_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));

    text_layer_set_text_color(battery_text_layer, GColorWhite);
	text_layer_set_background_color(battery_text_layer, GColorClear);
    text_layer_set_text_alignment(battery_text_layer, GTextAlignmentRight);
    layer_add_child(window_layer, text_layer_get_layer(battery_text_layer));
	
	
  bluetooth_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH);
  GRect bitmap_bounds_bt_on = gbitmap_get_bounds(bluetooth_image);
#ifdef PBL_PLATFORM_CHALK
  GRect frame_bt = GRect(87, 6, bitmap_bounds_bt_on.size.w, bitmap_bounds_bt_on.size.h);
#else
  GRect frame_bt = GRect(3, 1, bitmap_bounds_bt_on.size.w, bitmap_bounds_bt_on.size.h);
#endif 	
  bluetooth_layer = bitmap_layer_create(frame_bt);
  bitmap_layer_set_bitmap(bluetooth_layer, bluetooth_image);
  layer_add_child(window_layer, bitmap_layer_get_layer(bluetooth_layer));
	
	
	update_datetime();
}

static void main_window_unload(Window *window) {
  bitmap_layer_destroy(s_bitmap_layer);
  gbitmap_sequence_destroy(s_sequence);
  gbitmap_destroy(s_bitmap);	  
  gbitmap_destroy(start_bitmap);	  
  gbitmap_destroy(bluetooth_image);	  
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(battery_text_layer);
  bitmap_layer_destroy(bluetooth_layer);
  bitmap_layer_destroy(start_bitmap_layer);
}

static void init() {
  s_main_window = window_create();

  window_set_background_color(s_main_window, GColorBlack );
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });

	// handlers
    battery_state_service_subscribe(&update_battery_state);
    bluetooth_connection_service_subscribe(&toggle_bluetooth_icon);
	tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
	
    window_stack_push(s_main_window, true);
	
	// update the battery on launch
    update_battery_state(battery_state_service_peek());
	toggle_bluetooth_icon(bluetooth_connection_service_peek());

}

static void deinit() {
  window_destroy(s_main_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}