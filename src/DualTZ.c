#include "pebble.h"

#define DATE_STYLE 0
#define BT_VIBRATE 1

Window *window;

TextLayer *text_date_layer;
TextLayer *text_date2_layer;
TextLayer *text_time_layer;
TextLayer *text_time2_layer;
TextLayer *text_location_layer;
TextLayer *text_location2_layer;

Layer        *BatteryLineLayer;

GFont        fontHelvNewLight15;
GFont        fontMonaco13;
GFont		     fontRobotoCondensed21;
GFont        fontRobotoBoldSubset40;
GFont        fontRobotoBoldSubset45;

static int BTConnected = 1;
static int BTVibesDone = 0;
static char VibOnBTLoss[] = "0"; //From Config Page


static int batterychargepct;
static int BatteryVibesDone = 0;
static int batterycharging = 0;

static int FirstTime = 0;

static char date_type[]="us  ";
static char time_text[] = "00:00";
static char time2_text[] = "00:00x";
static char date_text[] = "Mon 05/08/48";
static char date2_text[] = "Mon 05/08/48";
static char seconds_text[] = "00";
static char date_format[]="%a %m/%d/%y";
static char date2_format[]="%a %m/%d/%y";
static char text_location[18];
static char text_location2[18];

time_t rawtime;
struct tm *gmtinfo;

GColor TextColorHold;
GColor BGColorHold;


void handle_battery(BatteryChargeState charge_state) {
  batterychargepct = charge_state.charge_percent;

  if (charge_state.is_charging) {
    batterycharging = 1;
  } else {
    batterycharging = 0;
  }

  // Reset if Battery > 20% ********************************
  if (batterychargepct > 20) {
     if (BatteryVibesDone == 1) {     //OK Reset to normal
         BatteryVibesDone = 0;
     }
  
  }

  //
  if (batterychargepct < 30) {
     if (BatteryVibesDone == 0) {            // Do Once
         BatteryVibesDone = 1;
         vibes_long_pulse();
      }
  }
    layer_mark_dirty(BatteryLineLayer);
}

void battery_line_layer_update_callback(Layer *BatteryLineLayer, GContext* ctx) { 
     graphics_context_set_fill_color(ctx, TextColorHold);
     graphics_fill_rect(ctx, layer_get_bounds(BatteryLineLayer), 3, GCornersAll);

     if (batterycharging == 1) {
       #ifdef PBL_COLOR
          graphics_context_set_fill_color(ctx, GColorBlue);
       #else
          graphics_context_set_fill_color(ctx, GColorBlack);
       #endif

       graphics_fill_rect(ctx, GRect(2, 1, 100, 4), 3, GCornersAll);

     } else if (batterychargepct > 20) {
       #ifdef PBL_COLOR
          graphics_context_set_fill_color(ctx, GColorGreen);
       #else
          graphics_context_set_fill_color(ctx, GColorBlack);
       #endif
         
       graphics_fill_rect(ctx, GRect(2, 1, batterychargepct, 4), 3, GCornersAll);
       
     } else {
      #ifdef PBL_COLOR
          graphics_context_set_fill_color(ctx, GColorRed);
       #else
          graphics_context_set_fill_color(ctx, GColorBlack);
       #endif
         
       graphics_fill_rect(ctx, GRect(2, 1, batterychargepct, 4),3, GCornersAll);
     }
}
void handle_bluetooth(bool connected) {
      if (connected) {
         BTConnected = 1;     // Connected
         BTVibesDone = 0;

    } else {
         BTConnected = 0;      // Not Connected

         if ((BTVibesDone == 0) && (strcmp(VibOnBTLoss,"0") == 0)) {
             BTVibesDone = 1;
             vibes_long_pulse();
         }
    }
 
}


void handle_appfocus(bool in_focus){
    if (in_focus) {
        handle_bluetooth(bluetooth_connection_service_peek());
    }
}
//<============================================================================================
void handle_tick(struct tm *tick_time, TimeUnits units_changed) {

  char time_format[] = "%I:%M";
  
 time_t seconds_since_epoch;
 seconds_since_epoch = time(NULL);
  
  //adjust for gmtime 
  seconds_since_epoch = seconds_since_epoch + (-5 * 3600);
  
  //convert seconds since epoch into structure for gmt time
  struct tm* gmtinfo = gmtime(&seconds_since_epoch);

  strftime(seconds_text, sizeof(seconds_text), "%S", tick_time);

  if (clock_is_24h_style()) {
    strcpy(time_format,"%R");
  } else {
    strcpy(time_format,"%I:%M");
  }

  strftime(time_text, sizeof(time_text), time_format, tick_time);

  strftime(time2_text, sizeof(time2_text), time_format, gmtinfo);
    
  // Kludge to handle lack of non-padded hour format string
  // for twelve hour clock.
  if (!clock_is_24h_style() && (time_text[0] == '0')) {
    memmove(time_text, &time_text[1], sizeof(time_text) - 1);
  }
  
  if (!clock_is_24h_style() && (time2_text[0] == '0')) {
    memmove(time2_text, &time2_text[1], sizeof(time2_text) - 1);
  }

  if((strcmp(seconds_text,"00") == 0) || (FirstTime == 0)) {
     strftime(date_text,    sizeof(date_text), date_format, tick_time);
    
     strftime(date2_text,    sizeof(date2_text), date_format, gmtinfo);

     text_layer_set_text(text_date_layer, date_text);
     text_layer_set_text(text_date2_layer, date2_text);
  }
     if (units_changed & DAY_UNIT) {
        // Only update the date when it's changed.
        text_layer_set_text(text_date_layer, date_text);
        text_layer_set_text(text_date2_layer, date2_text);
     }

     if((strcmp(seconds_text,"00") == 0) || (FirstTime == 0)) {
     text_layer_set_text(text_time_layer,  time_text);
     text_layer_set_text(text_time2_layer, time2_text);  
     }
  strcpy(text_location, "San Antonio, TX");
  strcpy(text_location2, "West Simsbury, CT");

  FirstTime = 1;
  
}  

//Receive Input from Config html page:
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  char BTVibeConfig[] = "0";

  APP_LOG(APP_LOG_LEVEL_INFO, "In Inbox received callback");

  FirstTime = 0;

  // Read first item
  Tuple *t = dict_read_first(iterator);

  // For all items
  while(t != NULL) {

    // Which key was received?
    switch(t->key) {
    case 0:
      strcpy(date_type, t->value->cstring);

      if (strcmp(date_type, "us") == 0) {
         strcpy(date_format, "%b %e, %Y");
      } else {
         strcpy(date_format, "%e %b %Y");
      }
      text_layer_set_text(text_date_layer, date_text);
      break;

    case 1:
      strcpy(BTVibeConfig, t->value->cstring);
      if (strcmp(BTVibeConfig, "0") == 0) {
         strcpy(VibOnBTLoss,"0");
      } else {
         strcpy(VibOnBTLoss,"1");
      }
      break;

    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }

    // Look for next item
    t = dict_read_next(iterator);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Inbox Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

void handle_deinit(void) {
  tick_timer_service_unsubscribe();

  persist_write_string(DATE_STYLE, date_type);
  persist_write_string(BT_VIBRATE, VibOnBTLoss);

  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  app_focus_service_unsubscribe();

  text_layer_destroy(text_time_layer);
  text_layer_destroy(text_time2_layer);
  text_layer_destroy(text_date_layer);
  text_layer_destroy(text_date2_layer);
  text_layer_destroy(text_location_layer);
  text_layer_destroy(text_location2_layer);

  fonts_unload_custom_font(fontHelvNewLight15);
  fonts_unload_custom_font(fontMonaco13);
  fonts_unload_custom_font(fontRobotoBoldSubset40);
  fonts_unload_custom_font(fontRobotoBoldSubset45);

  window_destroy(window);
}

void handle_init(void) {

  GColor BGCOLOR   = COLOR_FALLBACK(GColorDukeBlue, GColorBlack);
  BGColorHold = BGCOLOR;

  GColor TEXTCOLOR = COLOR_FALLBACK(GColorWhite, GColorWhite);
  TextColorHold = TEXTCOLOR;

  window = window_create();
  window_stack_push(window, true /* Animated */);
  window_set_background_color(window, BGCOLOR);

  fontMonaco13           = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_MONACO_13));
  fontHelvNewLight15     = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_HELV_NEW_LIGHT_15));
  fontRobotoBoldSubset40 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_40));
  fontRobotoBoldSubset45 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_45));
  Layer *window_layer = window_get_root_layer(window);

  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
 
  //Location
  text_location_layer = text_layer_create(GRect(1, 1, 144, 17)); 
  text_layer_set_text_alignment(text_location_layer, GTextAlignmentCenter);		
  text_layer_set_text(text_location_layer, text_location); 
  text_layer_set_font(text_location_layer, fontMonaco13);
  text_layer_set_background_color(text_location_layer, GColorKellyGreen);
  text_layer_set_text_color(text_location_layer, TEXTCOLOR);
  layer_add_child(window_layer, text_layer_get_layer(text_location_layer));
  
  
// Date
  #ifdef PBL_PLATFORM_CHALK
     text_date_layer = text_layer_create(GRect(1, 18, 180, 28));
  #else
     text_date_layer = text_layer_create(GRect(1, 18, 144, 22));
  #endif

  text_layer_set_text_color(text_date_layer, TEXTCOLOR);
  text_layer_set_background_color(text_date_layer, GColorKellyGreen);
  text_layer_set_text_alignment(text_date_layer, GTextAlignmentCenter);;
  text_layer_set_font(text_date_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
  layer_add_child(window_layer, text_layer_get_layer(text_date_layer));

  
  //Persistent Value Date Format:
  if (persist_exists(DATE_STYLE)) {
     persist_read_string(DATE_STYLE, date_type, sizeof(date_type));
  }  else {
     strcpy(date_type, "us");
  }

  if (strcmp(date_type, "us") == 0) {
      strcpy(date_format, "%a %m/%d/%y");
  } else {
      strcpy(date_format, "%a %d/%m/%y");
  }

  //Persistent Value VibOnBTLoss
  if(persist_exists(BT_VIBRATE)) {
     persist_read_string(BT_VIBRATE, VibOnBTLoss, sizeof(VibOnBTLoss));
  }  else {
     strcpy(VibOnBTLoss, "0"); // Default
  }

  // Time of Day
  #ifdef PBL_PLATFORM_CHALK
      text_time_layer = text_layer_create(GRect(1, 120, 180, 45));
      text_layer_set_font(text_time_layer,fontRobotoBoldSubset40);
  #else
      text_time_layer = text_layer_create(GRect(1, 40, 144, 40));
      text_layer_set_font(text_time_layer,fontRobotoBoldSubset40);
  #endif

  text_layer_set_text_color(text_time_layer, TEXTCOLOR);
  text_layer_set_background_color(text_time_layer, GColorKellyGreen);
  text_layer_set_text_alignment(text_time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_time_layer));

   //Location2
  text_location2_layer = text_layer_create(GRect(1, 92, 144, 17)); 
  text_layer_set_text_alignment(text_location2_layer, GTextAlignmentCenter);		
  text_layer_set_text(text_location2_layer, text_location2); 
  text_layer_set_font(text_location2_layer, fontMonaco13);
  text_layer_set_background_color(text_location2_layer, BGCOLOR);
  text_layer_set_text_color(text_location2_layer, TEXTCOLOR);
  layer_add_child(window_layer, text_layer_get_layer(text_location2_layer));
  
   // Battery Line
  GRect battery_line_frame = GRect(22, 84, 104, 6);
  BatteryLineLayer = layer_create(battery_line_frame);
  layer_set_update_proc(BatteryLineLayer, battery_line_layer_update_callback);
  layer_add_child(window_layer, BatteryLineLayer);

  // Date2
  #ifdef PBL_PLATFORM_CHALK
     text_date2_layer = text_layer_create(GRect(1, 18, 180, 28));
  #else
     text_date2_layer = text_layer_create(GRect(1, 106, 144, 22));
  #endif

  text_layer_set_text_color(text_date2_layer, TEXTCOLOR);
  text_layer_set_background_color(text_date2_layer, BGCOLOR);
  text_layer_set_text_alignment(text_date2_layer, GTextAlignmentCenter);;
  text_layer_set_font(text_date2_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
  layer_add_child(window_layer, text_layer_get_layer(text_date2_layer));

/*   // Time of Day2
  #ifdef PBL_PLATFORM_CHALK
      text_time_layer = text_layer_create(GRect(1, 120, 180, 45));
      text_layer_set_font(text_time_layer,fontRobotoBoldSubset40);
  #else */
      text_time2_layer = text_layer_create(GRect(1, 127, 144, 40));
      text_layer_set_font(text_time2_layer,fontRobotoBoldSubset40);
//  #endif

  text_layer_set_text_color(text_time2_layer, TEXTCOLOR);
  text_layer_set_background_color(text_time2_layer, BGCOLOR);
  text_layer_set_text_alignment(text_time2_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_time2_layer)); 

  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);

  bluetooth_connection_service_subscribe(&handle_bluetooth);

  battery_state_service_subscribe(&handle_battery);
  bluetooth_connection_service_subscribe(&handle_bluetooth);
  app_focus_service_subscribe(&handle_appfocus);

  handle_battery(battery_state_service_peek());
  handle_bluetooth(bluetooth_connection_service_peek());
 
}



int main(void) {
   handle_init();

   app_event_loop();

   handle_deinit();
}
