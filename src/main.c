#include <pebble.h>

#define TIME_OFFSET_KEY 1
#define DEFAULT_TIME_OFFSET 0

static Window *s_window;
static GBitmap *s_background_bitmap;
static BitmapLayer *s_background_layer;
static GBitmap *s_cursor_bitmap;
static BitmapLayer *s_cursor_layer;

TextLayer *layer_date_text;
TextLayer *time_layer;
TextLayer *layer_ampm_text;

static int cursor_x_position = 0;
static int time_offset = DEFAULT_TIME_OFFSET;

static GFont *time_font;
static GFont *batt_font;

static void check_time() {
  time_t current_time = time(NULL) + (time_offset*60) + 43200;
  current_time = current_time % 86400;
  cursor_x_position = 142-((current_time/608+2)%142);
  layer_set_frame(bitmap_layer_get_layer(s_cursor_layer), GRect(cursor_x_position, 0, 6, 168));
  layer_mark_dirty(bitmap_layer_get_layer(s_cursor_layer));
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Message received!");
  Tuple *t = dict_read_first(iterator);

  // Process all pairs present
  while (t != NULL) {
    // Process this pair's key
    switch (t->key) {
      case TIME_OFFSET_KEY:
        APP_LOG(APP_LOG_LEVEL_INFO, "Received time zone offset: %d", (int)(t->value->int32));
        time_offset = (int)(t->value->int32);
        check_time();
        break;
    }

    // Get next pair, if any
    t = dict_read_next(iterator);
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

static void tick_handler(struct tm *tick_time, TimeUnits units_changes) {
  check_time();
	
	static char time_buffer[] = "00:00";
    static char date_text[] = "Mon, 24 September";
    static char ampm_text[] = "pm";

   strftime(date_text, sizeof(date_text), "%a, %e %B", tick_time);
   text_layer_set_text(layer_date_text, date_text);
	
  if(clock_is_24h_style()){
    strftime(time_buffer, sizeof(time_buffer), "%R", tick_time);
  }
  else{
    strftime(time_buffer, sizeof(time_buffer), "%l:%M", tick_time);
	  
    strftime(ampm_text, sizeof(ampm_text), "%P", tick_time);
    text_layer_set_text(layer_ampm_text, ampm_text);	  
  }
  text_layer_set_text(time_layer, time_buffer);
	
}

static void window_load(Window *window) {
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_WORLD_BACKGROUND);
  s_cursor_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CURSOR);
  s_background_layer = bitmap_layer_create(GRect(0,0,144,168));
  s_cursor_layer = bitmap_layer_create(GRect(cursor_x_position,0,6,168));
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  bitmap_layer_set_bitmap(s_cursor_layer, s_cursor_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_cursor_layer));
	
  time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CAVIARDREAMS_36));
  batt_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CAVIARDREAMS_14));

  time_layer = text_layer_create(GRect(0, -5, 100, 168));
  text_layer_set_font(time_layer, time_font);
  text_layer_set_text_color(time_layer, GColorWhite);
  text_layer_set_background_color(time_layer, GColorClear);
  text_layer_set_text_alignment(time_layer, GTextAlignmentRight);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(time_layer));

  layer_date_text = text_layer_create(GRect(8, 33, 144, 30));
  text_layer_set_text_color(layer_date_text, GColorWhite);
  text_layer_set_background_color(layer_date_text, GColorClear);
  text_layer_set_font(layer_date_text, batt_font);
  text_layer_set_text_alignment(layer_date_text, GTextAlignmentLeft);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(layer_date_text));	
	
  layer_ampm_text = text_layer_create(GRect(100, -4, 30, 30));
  text_layer_set_text_color(layer_ampm_text, GColorWhite);
  text_layer_set_background_color(layer_ampm_text, GColorClear);
  text_layer_set_font(layer_ampm_text, batt_font);
  text_layer_set_text_alignment(layer_ampm_text, GTextAlignmentLeft);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(layer_ampm_text));		
	
  struct tm *tick_time;
  time_t temp;        
  temp = time(NULL);        
  tick_time = localtime(&temp);

  tick_handler(tick_time, MINUTE_UNIT);
}

static void window_unload(Window *window) {
  gbitmap_destroy(s_background_bitmap);
  gbitmap_destroy(s_cursor_bitmap);
  bitmap_layer_destroy(s_background_layer);
  bitmap_layer_destroy(s_cursor_layer);
	
  text_layer_destroy( time_layer );
  text_layer_destroy( layer_date_text );
  text_layer_destroy( layer_ampm_text );
  	  
  fonts_unload_custom_font(time_font);
  fonts_unload_custom_font(batt_font);
	
}

static void init() {
  s_window = window_create();
  
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  
  window_stack_push(s_window, true);
  
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  
  tick_timer_service_subscribe(MINUTE_UNIT, (TickHandler)tick_handler);
  
  if (persist_exists(TIME_OFFSET_KEY)) {
    time_offset = persist_read_int(TIME_OFFSET_KEY);
  }
  
  check_time();
}

static void deinit() {
  persist_write_int(TIME_OFFSET_KEY, time_offset);
	
  window_destroy(s_window);

}

int main(void) {
  init();
  app_event_loop();
  deinit();
}