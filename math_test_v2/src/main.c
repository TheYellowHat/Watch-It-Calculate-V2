#include <pebble.h>

#define KEY_COLOR_RED     0
#define KEY_COLOR_GREEN   1
#define KEY_COLOR_BLUE    2
#define KEY_HIGH_CONTRAST 3

//pointer/variable declaration
static Window *s_main_window;
static TextLayer *s_backdate_layer, *s_backtime_layer, *s_time_layer, *s_date_layer, *s_uptime_layer, *s_downtime_layer, *s_plus_layer, *s_minus_layer;
static GFont s_time_font, s_date_font;
static int s_battery_level;
static Layer *s_battery_layer;
static bool ticket = false;
static bool gray = true;
static int animated = 0;
static char s_buffer[8], check[8], blank[5];
static GColor bg_color;

//checks variables added
static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  // High contrast selected?
  Tuple *high_contrast_t = dict_find(iter, KEY_HIGH_CONTRAST);
  if(high_contrast_t && high_contrast_t->value->int8 > 0) {  // Read boolean as an integer
    // Persist value
    text_layer_set_text_color(s_backtime_layer, GColorDarkGray);
  	text_layer_set_text_color(s_backdate_layer, GColorDarkGray);
    gray = true;
    persist_write_bool(KEY_HIGH_CONTRAST, true);
  } else {
    text_layer_set_text_color(s_backtime_layer, GColorClear);
  	text_layer_set_text_color(s_backdate_layer, GColorClear);
    gray = false;
    persist_write_bool(KEY_HIGH_CONTRAST, false);
  }
  
  // Color scheme?
  Tuple *color_red_t = dict_find(iter, KEY_COLOR_RED);
  Tuple *color_green_t = dict_find(iter, KEY_COLOR_GREEN);
  Tuple *color_blue_t = dict_find(iter, KEY_COLOR_BLUE);
    int red = color_red_t->value->int32;
    int green = color_green_t->value->int32;
    int blue = color_blue_t->value->int32;
  
    // Persist values
    persist_write_int(KEY_COLOR_RED, red);
    persist_write_int(KEY_COLOR_GREEN, green);
    persist_write_int(KEY_COLOR_BLUE, blue);
  
    window_set_background_color(s_main_window, GColorBlack);
  
    bg_color = GColorFromRGB(red, green, blue);
    text_layer_set_text_color(s_time_layer, bg_color);
    text_layer_set_text_color(s_date_layer, bg_color);
    text_layer_set_text_color(s_uptime_layer, bg_color);
    text_layer_set_text_color(s_downtime_layer, bg_color);
    text_layer_set_text_color(s_plus_layer, bg_color);
    text_layer_set_text_color(s_minus_layer, bg_color);
}
  ////////////////////////////////////////////////////////////////////////////////////////////////////

//make blank array empty
static void empty_blank(){
  for(int i=0; i<5; i++){
    blank[i] = ' ';
  }
}

//getting battery percentage
static void battery_callback(BatteryChargeState state){
  //record new battery level
  s_battery_level = state.charge_percent;
  
  //update meter
  layer_mark_dirty(s_battery_layer);
}

//update battery layer
static void battery_update_proc(Layer *layer, GContext *ctx){
  GRect bounds = layer_get_bounds(layer);
  
  //find width of bar
  int height = 49.0F - (int)(float)(((float)s_battery_level / 100.0F) * 49.0F);
  
  //draw background
  graphics_context_set_fill_color(ctx, bg_color);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  
  //draw bar
  graphics_context_set_fill_color(ctx, GColorDarkGray);
  graphics_fill_rect(ctx, GRect(0,0,bounds.size.w,height), 0, GCornerNone);
}

void animate_layer(Layer *layer, GRect *start, GRect *finish, int duration, int delay);

//animation finished handler
void on_animation_stopped(Animation *animation, bool finished, void *context){
  //if add/minus was animated
  if(animated != 2){
    //change plus/minus to blank
    text_layer_set_text(s_uptime_layer, blank);
    text_layer_set_text(s_downtime_layer, blank);
    animated++;
  }
  if(animated == 2){
    //get info about window
    Layer *window_layer = window_get_root_layer(s_main_window);
    GRect bounds = layer_get_bounds(window_layer);
    
    //box for time display
    GRect time_ani = GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 50);
    
    //animation
    animate_layer(text_layer_get_layer(s_time_layer), &time_ani, &time_ani, 1000, 1000);
    
    //change check value
    for (int i=0;i<5;i++){
      check[i] = s_buffer[i];
    }
    //display time on textlayer
    text_layer_set_text(s_time_layer, check);
    animated = 0;
  }
  
  //destory animations
  property_animation_destroy((PropertyAnimation*) animation);
}

//animation start handler
void animate_layer(Layer *layer, GRect *start, GRect *finish, int duration, int delay){
  //Declare animation
  PropertyAnimation *anim = property_animation_create_layer_frame(layer, start, finish);
 
  //Set characteristics
  animation_set_duration((Animation*) anim, duration);
  animation_set_delay((Animation*) anim, delay);
 
  //Set stopped handler to free memory
  AnimationHandlers handlers = {
      //The reference to the stopped handler is the only one in the array
      .stopped = (AnimationStoppedHandler) on_animation_stopped
  };
  animation_set_handlers((Animation*) anim, handlers, NULL);
 
  //Start animation!
  animation_schedule((Animation*) anim);
}

//changing the time
static void time_change(char *buffer){
  //write adding stuff to the uptime and downtime layers
  //plus and minus char arrays
  static char plus[8];
  static char minus[8];
  
  for(int i=0; i<5; i++){
    plus[i] = ' ';
    minus[i] = ' ';
  }
  
  //1 minute change
  if(buffer[4] != check[4]){
    //10 minute change
    if(buffer[3] != check[3]){
      //1 hour change
      if(buffer[1] != check[1]){
        //10 hour change
        if(buffer[0] != check[0]){
          //complete 12/24 hour flip
          if(buffer[0] == '0' && buffer[1] == '0'){
            //12 hour flip
            if(check[0] == '1' && buffer[1] == '1'){
              minus[0] = '1';
              minus[1] = '1';
              minus[3] = '5';
              minus[4] = '9';
            //24 hour flip
            }else{
              minus[0] = '2';
              minus[1] = '3';
              minus[3] = '5';
              minus[4] = '9';
            }
          //10's hour changed only
          }else{
            plus[0] = '1';
            minus[1] = '9';
            minus[3] = '5';
            minus[4] = '9';
          }
        //1's hour changed only
        }else{
          plus[1] = '1';
          minus[3] = '5';
          minus[4] = '9';
        }
      //10's minute changed only
      }else{
        plus[3] = '1';
        minus[4] = '9';
      }
    //one's minute changed only
    }else{
      plus[4] = '1';
    }
  }
  
  //display plus/minus
  text_layer_set_text(s_uptime_layer, plus);
  text_layer_set_text(s_downtime_layer, minus);
  
  //get info about window
  Layer *window_layer = window_get_root_layer(s_main_window);
  GRect bounds = layer_get_bounds(window_layer);
  
  //start/finish end frams for animation
  GRect plus_ani = GRect(0 , PBL_IF_ROUND_ELSE(23, 17), bounds.size.w, 50);
  GRect minus_ani = GRect(0 , PBL_IF_ROUND_ELSE(93, 87), bounds.size.w, 50);
  
  //animate layers
  animate_layer(text_layer_get_layer(s_uptime_layer), &plus_ani, &plus_ani, 1000, 0);
  animate_layer(text_layer_get_layer(s_downtime_layer), &minus_ani, &minus_ani, 1000, 0);
}

//telling time
static void update_time(){
  //get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  //copy date into buffer from tm structure
  static char date_buffer[16];
  strftime(date_buffer, sizeof(date_buffer), "%a %d %b", tick_time);
  
  //write current hrs and mins into a buffer
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
  
  if(ticket == false){
    strftime(check, sizeof(check), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
    text_layer_set_text(s_time_layer, check);
    ticket = true;
  }
  
  //if time changed
  if(s_buffer[4] != check[4]){
    time_change(s_buffer);
  }
  
  //show date on textlayer
  text_layer_set_text(s_date_layer, date_buffer);
}

//used for telling the time
static void tick_handler(struct tm *tick_time, TimeUnits units_changed){
  update_time();
}

//what is loaded onto the window by default
static void main_window_load(Window *window){
  //get info about window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  //create textlayer with specific bounds
  s_time_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 50));
  s_backtime_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 50));
  s_uptime_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(23, 17), bounds.size.w, 50));
  s_downtime_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(93, 87), bounds.size.w, 50));
  s_plus_layer = text_layer_create(GRect(PBL_IF_ROUND_ELSE(15,-1), PBL_IF_ROUND_ELSE(23, 17), 25, 50));
  s_minus_layer = text_layer_create(GRect(PBL_IF_ROUND_ELSE(15,-1), PBL_IF_ROUND_ELSE(93, 87), 25, 50));
  
  //create gfont
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITAL_48));
  s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITAL_20));
  
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  // Use background color setting
  int red = persist_read_int(KEY_COLOR_RED);
  int green = persist_read_int(KEY_COLOR_GREEN);
  int blue = persist_read_int(KEY_COLOR_BLUE);

  // Persist values
  persist_write_int(KEY_COLOR_RED, red);
  persist_write_int(KEY_COLOR_GREEN, green);
  persist_write_int(KEY_COLOR_BLUE, blue);

  bg_color = GColorFromRGB(red, green, blue);
  window_set_background_color(s_main_window, GColorBlack);
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  
  //create date textlayer
  s_date_layer = text_layer_create(GRect(PBL_IF_ROUND_ELSE(0,2),146, PBL_IF_ROUND_ELSE(180,144),20));
  text_layer_set_text_color(s_date_layer, bg_color);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_alignment(s_date_layer, PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft));
  text_layer_set_font(s_date_layer, s_date_font);
  
  //create battery meter layer
  s_battery_layer = layer_create(GRect(PBL_IF_ROUND_ELSE(24,8),PBL_IF_ROUND_ELSE(66,60),4,49));
  layer_set_update_proc(s_battery_layer, battery_update_proc);
  
  //time layer
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, bg_color);
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  
  if(gray == true){
  	//backdate layer
  	s_backdate_layer = text_layer_create(GRect(PBL_IF_ROUND_ELSE(0,2),146, PBL_IF_ROUND_ELSE(180,144),20));
    text_layer_set_text_color(s_backdate_layer, GColorDarkGray);
  	text_layer_set_background_color(s_backdate_layer, GColorClear);
  	text_layer_set_text(s_backdate_layer, "000 00 000");
  	text_layer_set_text_alignment(s_backdate_layer, PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft));
  	text_layer_set_font(s_backdate_layer, s_date_font);
  	  
  	//backtime layer
  	text_layer_set_background_color(s_backtime_layer, GColorClear);
  	text_layer_set_text_color(s_backtime_layer, GColorDarkGray);
  	text_layer_set_text(s_backtime_layer, "00 00");
  	text_layer_set_font(s_backtime_layer, s_time_font);
  	text_layer_set_text_alignment(s_backtime_layer, GTextAlignmentCenter);
  }
  
  //uptime layer
  text_layer_set_background_color(s_uptime_layer, GColorClear);
  text_layer_set_text_color(s_uptime_layer, bg_color);
  text_layer_set_font(s_uptime_layer, s_time_font);
  text_layer_set_text_alignment(s_uptime_layer, GTextAlignmentCenter);
  
  //downtime layer
  text_layer_set_background_color(s_downtime_layer, GColorClear);
  text_layer_set_text_color(s_downtime_layer, bg_color);
  text_layer_set_font(s_downtime_layer, s_time_font);
  text_layer_set_text_alignment(s_downtime_layer, GTextAlignmentCenter);
  
  //plus layer
  text_layer_set_background_color(s_plus_layer, GColorClear);
  text_layer_set_text_color(s_plus_layer, bg_color);
  text_layer_set_text(s_plus_layer, "+");
  text_layer_set_font(s_plus_layer, s_time_font);
  
  //minus layer
  text_layer_set_background_color(s_minus_layer, GColorClear);
  text_layer_set_text_color(s_minus_layer, bg_color);
  text_layer_set_text(s_minus_layer, "-");
  text_layer_set_font(s_minus_layer, s_time_font);
  
  //add battery meter to window
  layer_add_child(window_layer, s_battery_layer);
  
  if(gray == true){
    //add backtime layer to window
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_backtime_layer));
    //add back date layer to window
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_backdate_layer));
  }
  
  //add uptime layer to window
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_uptime_layer));
  
  //add downtime layer to window
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_downtime_layer));
  
  //add plus layer to window
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_plus_layer));
  
  //add minus layer to window
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_minus_layer));
  
  //add time layer to window
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
  
  //add date layer to window
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
}

//what is unloaded from the window if need be
static void main_window_unload(Window *window){
  //destory textlayer
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_backtime_layer);
  text_layer_destroy(s_backdate_layer);
  text_layer_destroy(s_uptime_layer);
  text_layer_destroy(s_downtime_layer);
  text_layer_destroy(s_plus_layer);
  text_layer_destroy(s_minus_layer);
  
  //unload gfont
  fonts_unload_custom_font(s_time_font);
  fonts_unload_custom_font(s_date_font);
  
  //destory battery layer
  layer_destroy(s_battery_layer);
}

//creates app stuff
static void init(){
  //make blank array empty
  empty_blank();
  
  //create main window element and assign to pointer
  s_main_window = window_create();
  
  //set window background to black
  window_set_background_color(s_main_window, GColorBlack);
  
  //set handlers to manage the elements inside window
  window_set_window_handlers(s_main_window, (WindowHandlers){
    .load = main_window_load,
    .unload = main_window_unload
  });
    
  //show window on watch, with animated = true
  window_stack_push(s_main_window, true);
  
  //getting variables from inbox
  app_message_register_inbox_received(inbox_received_handler);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  
  //make sure time is displayer from start
  update_time();
  
  //register for battery level updates
  battery_state_service_subscribe(battery_callback);
  
  //register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  //ensure battery level is displayed from start
  battery_callback(battery_state_service_peek());
}

//destorys app stuff
static void deinit(){
  //destory window
  window_destroy(s_main_window);
}

int main(void){
  //called to create everything for the app
  init();
  
  //called to loop the app
  app_event_loop();
  
  //called to destroy everything once app closes
  deinit();
}
