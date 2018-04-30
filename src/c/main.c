

// void layer_set_update_proc(Layer * layer, LayerUpdateProc update_proc) when layer is dirtied function update_proc us called.
//     layer_mark_dirty(myCanvas); marks 'myCanvas' for update. 


#include <pebble.h>
#include "simple_analog.h"

//need to place hour and min hands on seperate layers 1) as possible fix for glitch bug and 2) to prevent transparency

static Window* s_main_window;
static Layer* myCanvas;
static TextLayer *time_layer;
static Layer* motherLayer;
static GPath *tic_marks[NUM_TICS];
static GPath *min_hand = NULL;
static GPath *hour_hand = NULL;
int n = sizeof(ANALOG_BG_POINTS)/sizeof(ANALOG_BG_POINTS[0]);
    
// instantiate useful values;
int seconds_in_hour = 60*60;
int seconds_in_min = 60;
float seconds_in_decimal_sec = 0.864;
int dec_seconds_in_dec_day = 1*10*100*100;
int dec_seconds_in_dec_hour = 1*100*100;
int dec_seconds_in_dec_min = 100;



struct decTm {
    int dec_sec;
    int dec_min;
    int dec_hr;
};



void getDecTime(struct decTm* decTime, struct tm* timeData) {

    int offset = 0;
    int hr = timeData->tm_hour + offset;
    int min = timeData->tm_min;
    int sec = timeData->tm_sec;
    int sinceStart = (hr*seconds_in_hour) + (min*seconds_in_min) + (sec);
  
    decTime->dec_sec =  sinceStart;
    decTime->dec_sec /= seconds_in_decimal_sec;
    
    //convert dec seconds into dec hours
    decTime->dec_hr = decTime->dec_sec/dec_seconds_in_dec_hour;
    
    //divide the remaining seconds after converting to dec hours hex mins
    decTime->dec_min = (decTime->dec_sec%(decTime->dec_hr*dec_seconds_in_dec_hour));
    decTime->dec_min /= dec_seconds_in_dec_min;

}




static void updateGame(Layer *layer, GContext *ctx)
{
  GRect bounds = layer_get_bounds(layer);
  
  // Fill and stroke the paths for hour and min hands:
  graphics_context_set_fill_color(ctx, GColorWhite);

  // Fill the tic paths
  for(int i = 0; i<n; i++) {
    gpath_draw_filled(ctx, tic_marks[i]);    
  } 
  
  gpath_draw_filled(ctx, min_hand);
  gpath_draw_outline(ctx, min_hand); 
  gpath_draw_filled(ctx, hour_hand);
  gpath_draw_outline(ctx, hour_hand);
  
  // dot in the middle
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(73, 85, 3, 3), 0, GCornerNone);
}


void create_layout(int minute_hand_rotation, int hour_hand_rotation) {
     GFont time_font =  fonts_get_system_font(FONT_KEY_GOTHIC_28);
//      GRect bounds = layer_get_bounds(motherLayer);
//      GPoint center = grect_center_point(&bounds);  
     myCanvas = layer_create(GRect(0, 0, 143, 180));
     motherLayer = window_get_root_layer(s_main_window);
     layer_add_child(motherLayer, myCanvas);
     layer_set_update_proc(myCanvas, updateGame);
  
      time_layer = text_layer_create(GRect(30,105,90,50));
      text_layer_set_background_color(time_layer, GColorClear);
      text_layer_set_text_color(time_layer, GColorWhite);
      layer_add_child(motherLayer, text_layer_get_layer(time_layer));
      text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
      text_layer_set_font(time_layer, time_font);

  
  
  // create the hour and min hands from data from simple_analog.h
     min_hand = gpath_create(&MINUTE_HAND_POINTS);    
     hour_hand = gpath_create(&HOUR_HAND_POINTS); 


  // create tic marks on clock based on data from simple_analog h and stroke and fill path info from "update game.""
      for(int i = 0; i<n; i++) {
         tic_marks[i] = gpath_create(&ANALOG_BG_POINTS[i]); 
      }

  
    //move hour and min hands into place
      gpath_move_to(min_hand, GPoint (73, 85));
      gpath_rotate_to(min_hand, minute_hand_rotation);

  
      gpath_move_to(hour_hand, GPoint (72, 85));
      gpath_rotate_to(hour_hand, hour_hand_rotation);


}



void update_hands(int minute_hand_rotation, int hour_hand_rotation) {
    //move hour and min hands into place
      gpath_move_to(min_hand, GPoint (73, 85));
      gpath_rotate_to(min_hand, minute_hand_rotation);

      gpath_move_to(hour_hand, GPoint (72, 85));
      gpath_rotate_to(hour_hand, hour_hand_rotation);

      layer_mark_dirty(myCanvas);
  
}





static void update_time() {
   int minute_hand_rotation = 0;
   int hour_hand_rotation = 0;
   
  
   // Get a tm structure
  time_t currentTime;
  time(&currentTime);
    
  //time struct for local time (important for getting local day/date).
  struct tm* timeDataLocal = localtime(&currentTime);
  
// get hex time
  struct decTm decTest;
  struct decTm* decTime = &decTest;
  decTime->dec_hr=0;
  decTime->dec_min=0;
  decTime->dec_sec=0;
  getDecTime(decTime, timeDataLocal);
  

// calculate rotations for hour and min hands
  minute_hand_rotation = (TRIG_MAX_ANGLE * decTime->dec_min / 100);
  hour_hand_rotation = (TRIG_MAX_ANGLE * (((double)decTime->dec_hr * (double)dec_seconds_in_dec_hour) + ((double)decTime->dec_min * (double)dec_seconds_in_dec_min))/(double)dec_seconds_in_dec_day);   
  
//   create_layout(minute_hand_rotation, hour_hand_rotation);
  update_hands(minute_hand_rotation, hour_hand_rotation);
  

  // Write the current hours and minutes into a buffer
     static char* s_buffer = "";
  
      if (decTime->dec_min < 10) {
          APP_LOG(APP_LOG_LEVEL_DEBUG, "its smaller!");
          snprintf(s_buffer, 6, "%d:0%d\n", decTime->dec_hr, decTime->dec_min); 
      }
      if (decTime->dec_min >= 10) {
         APP_LOG(APP_LOG_LEVEL_DEBUG, "its bigger!");
         snprintf(s_buffer, 6, "%d:%d\n", decTime->dec_hr, decTime->dec_min);        
      }

//      snprintf(s_buffer, 5, "%d:%d\n", decTime->dec_hr, decTime->dec_min);        
  
    // Display this time on the TextLayer
     text_layer_set_text(time_layer, s_buffer); 
}



static void tick_handler(struct tm *timeData, TimeUnits units_changed) {
  update_time();
//   APP_LOG(APP_LOG_LEVEL_DEBUG, "Heap Available: %d", heap_bytes_free());
}


static void main_window_load(Window *window) {
  // Create Layers
  create_layout(0, 0);
}


static void main_window_unload(Window *window) {
  // Destroy Layers
}


static void init() {
  
  // Register with TickTimerService
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  
  s_main_window = window_create();
  
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_set_background_color(s_main_window, GColorBlack);
  window_stack_push(s_main_window, true);
  
  // Make sure the time is displayed from the start
  update_time();
}

static void deinit() {
  // Destroy Window

  gpath_destroy(min_hand);
  gpath_destroy(hour_hand);

  for (int i = 0; i < n-1; ++i) {
    gpath_destroy(tic_marks[i]);
  }

  tick_timer_service_unsubscribe();
  window_destroy(s_main_window);
}


int main(void) {
  init();
  app_event_loop();
  deinit();
}