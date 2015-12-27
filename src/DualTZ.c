#include "pebble.h"

#define LOCAL_BG_COLOR_KEY      0
#define TZ2_BG_COLOR_KEY        1
#define LOCAL_TEXT_COLOR_KEY    2
#define TZ2_TEXT_COLOR_KEY      3
#define DATE_FORMAT_KEY         4
#define BT_LOSS_KEY             5
#define LOW_BATTERY_KEY         6
#define UTC_OFFSET_KEY          7
#define LOCATION_NAME_KEY       8
#define WEATHER_TEMPERATURE_KEY 9
#define WEATHER_CITY_KEY        10

Window *window;

TextLayer *text_local_layer;
TextLayer *text_TZ2_layer;
TextLayer *text_date_layer;
TextLayer *text_date2_layer;
TextLayer *text_time_layer;
TextLayer *text_time2_layer;
TextLayer *text_location_layer;
TextLayer *text_location2_layer;
TextLayer *text_degrees_layer;

Layer     *BatteryLineLayer;
Layer     *BTLayer;

GFont     fontMonaco13;
GFont     fontRobotoBoldSubset30;
GFont     fontRobotoBoldSubset35;

GPoint     Linepoint;

static int BTConnected = 1;
static int BTVibesDone = 0;

static int  batterychargepct;
static int  BatteryVibesDone = 0;
static int  batterycharging = 0;
static int  FirstTime = 0;
static int  ProcessHexRetcode = 0;
static int  WxLocationCall = 0;
static int  intUTCAdjust = 0;
static int  intUTCoffsethrs;
static int  intUTCoffsetmin;
static int  intRed     = 0;
static int  intBlue    = 0;
static int  intGreen   = 0;

static char UTCOffsetConfig[]   = "+00:00";
static char strSign[]           = "+";
static char time_text[]         = "00:00am";
static char time2_text[]        = "00:00x";
static char date_text[]         = "  Dec 26, 2015";
static char date2_text[]        = "  Dec 26, 2015";
static char seconds_text[]      = "00";
static char date_format[]       = "  %a %m/%d/%y";
static char text_location[18];
static char text_location2[18];
static char text_degrees[]      = "====";
static char HexIn[]             = "A";
static char DoubleHexIn[]       = "AA";
static char hexColorHold[]      = "0xFF0000";

static char PersistLocalBG[]    = "0x0000FF";
static char PersistTZ2BG[]      = "0xFFFFFF";
static char PersistLocalText[]  = "0xFFFFFF";
static char PersistTZ2Text[]    = "0xFFFFFF";
static int  PersistDateFormat   = 0;
static int  PersistBTLoss       = 0;
static int  PersistLow_Batt     = 0;
static char PersistUTCOffset[]  = "+00:00";
static char PersistLocationName[19];

GColor TextColorHold1;
GColor TextColorHold2;
GColor BGColorHold2;
GColor ColorHold;
   
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

  
  if ((batterychargepct < 30) && (PersistLow_Batt == 1) &&  (BatteryVibesDone == 0)) {          
         BatteryVibesDone = 1;
         vibes_long_pulse();
      }
  
    layer_mark_dirty(BatteryLineLayer);
}

void battery_line_layer_update_callback(Layer *BatteryLineLayer, GContext* batctx) { 
     graphics_context_set_fill_color(batctx, GColorWhite);
  
     graphics_fill_rect(batctx, layer_get_bounds(BatteryLineLayer), 3, GCornersAll);

     if (batterycharging == 1) {
          #ifdef PBL_PLATFORM_APLITE
             graphics_context_set_fill_color(batctx, GColorBlack);
          #else
             graphics_context_set_fill_color(batctx, GColorBlue);
          #endif
       
          graphics_fill_rect(batctx, GRect(2, 1, 100, 4), 3, GCornersAll);
     } else if (batterychargepct > 20) {
          #ifdef PBL_PLATFORM_APLITE
             graphics_context_set_fill_color(batctx, GColorBlack);
          #else
             graphics_context_set_fill_color(batctx, GColorGreen);
          #endif 
       
          graphics_fill_rect(batctx, GRect(2, 1, batterychargepct, 4), 3, GCornersAll);
     } else {   // Battery 20% or less 
          #ifdef PBL_PLATFORM_APLITE
             graphics_context_set_fill_color(batctx, GColorDarkGray);
          #else
             graphics_context_set_fill_color(batctx, GColorRed);
          #endif
       
          graphics_fill_rect(batctx, GRect(2, 1, batterychargepct, 4),3, GCornersAll);
     }
  
  //Battery % Markers
      #ifdef PBL_PLATFORM_APLITE
         if(batterycharging == 1) {
            graphics_context_set_fill_color(batctx, GColorBlack);
         } else {
            graphics_context_set_fill_color(batctx, GColorWhite);
         }
      #else
         graphics_context_set_fill_color(batctx, GColorBlack);
      #endif
  
      graphics_fill_rect(batctx, GRect(89, 1, 3, 4), 3, GCornerNone);
      graphics_fill_rect(batctx, GRect(79, 1, 3, 4), 3, GCornerNone);
      graphics_fill_rect(batctx, GRect(69, 1, 3, 4), 3, GCornerNone);
      graphics_fill_rect(batctx, GRect(59, 1, 3, 4), 3, GCornerNone);
      graphics_fill_rect(batctx, GRect(49, 1, 3, 4), 3, GCornerNone);
      graphics_fill_rect(batctx, GRect(39, 1, 3, 4), 3, GCornerNone);
      graphics_fill_rect(batctx, GRect(29, 1, 3, 4), 3, GCornerNone);
      graphics_fill_rect(batctx, GRect(19, 1, 3, 4), 3, GCornerNone);
      graphics_fill_rect(batctx, GRect(9,  1, 3, 4), 3, GCornerNone);
}

void handle_bluetooth(bool connected) {
      if (connected) {
         BTConnected = 1;     // Connected
         BTVibesDone = 0;
    } else {
         BTConnected = 0;      // Not Connected
    }
  
    layer_mark_dirty(BTLayer);
}

//BT Logo Callback;
void BTLine_update_callback(Layer *BTLayer, GContext* BT1ctx) {

       GPoint BTLinePointStart;
       GPoint BTLinePointEnd;
      
      if ((BTConnected == 0) && (PersistBTLoss == 1)) {
              BTVibesDone = 1;
              vibes_long_pulse();
      }
      if(BTConnected == 0) {   
          graphics_context_set_stroke_color(BT1ctx, GColorRed);
          graphics_context_set_fill_color(BT1ctx, GColorWhite);
          graphics_fill_rect(BT1ctx, layer_get_bounds(BTLayer), 0, GCornerNone);
      
        // "X"" Line 1
          BTLinePointStart.x = 1;
          BTLinePointStart.y = 1;
  
          BTLinePointEnd.x = 20;
          BTLinePointEnd.y = 20;
          graphics_draw_line(BT1ctx, BTLinePointStart, BTLinePointEnd);   
          
         // "X"" Line 2
          BTLinePointStart.x = 1;
          BTLinePointStart.y =20;
  
          BTLinePointEnd.x = 20;
          BTLinePointEnd.y = 1;
          graphics_draw_line(BT1ctx, BTLinePointStart, BTLinePointEnd);
         
      } else {   
       BTVibesDone = 0; 
      
       graphics_context_set_stroke_color(BT1ctx, TextColorHold2);
       graphics_context_set_fill_color(BT1ctx, BGColorHold2);
  

       //Line 1
       BTLinePointStart.x = 10;
       BTLinePointStart.y = 1;
  
       BTLinePointEnd.x = 10;
       BTLinePointEnd.y = 20;
       graphics_draw_line(BT1ctx, BTLinePointStart, BTLinePointEnd);
         
       //Line 1a
       BTLinePointStart.x = 11;
       BTLinePointStart.y = 1;
  
       BTLinePointEnd.x = 11;
       BTLinePointEnd.y = 20;
       graphics_draw_line(BT1ctx, BTLinePointStart, BTLinePointEnd);
   
       //Line 2
       BTLinePointStart.x = 10;
       BTLinePointStart.y = 1;
  
       BTLinePointEnd.x = 17;
       BTLinePointEnd.y = 6;
       graphics_draw_line(BT1ctx, BTLinePointStart, BTLinePointEnd);
      
       //Line 2a
       BTLinePointStart.x = 11;
       BTLinePointStart.y = 1;
  
       BTLinePointEnd.x = 17;
       BTLinePointEnd.y = 5;
       graphics_draw_line(BT1ctx, BTLinePointStart, BTLinePointEnd);
 
       //Line 3
       BTLinePointStart.x = 4;
       BTLinePointStart.y = 5;
  
       BTLinePointEnd.x = 17;
       BTLinePointEnd.y = 15;
       graphics_draw_line(BT1ctx, BTLinePointStart, BTLinePointEnd);
  
       //Line 3a
       BTLinePointStart.x = 4;
       BTLinePointStart.y = 6;
  
       BTLinePointEnd.x = 17;
       BTLinePointEnd.y = 16;
       graphics_draw_line(BT1ctx, BTLinePointStart, BTLinePointEnd);

       //Line 4
       BTLinePointStart.x = 4;
       BTLinePointStart.y = 15;
  
       BTLinePointEnd.x = 17;
       BTLinePointEnd.y = 5;
       graphics_draw_line(BT1ctx, BTLinePointStart, BTLinePointEnd);
  
       //Line 4a
       BTLinePointStart.x = 4;
       BTLinePointStart.y = 16;
  
       BTLinePointEnd.x = 17;
       BTLinePointEnd.y = 6;
       graphics_draw_line(BT1ctx, BTLinePointStart, BTLinePointEnd);
  
       //Line 5
       BTLinePointStart.x = 10;
       BTLinePointStart.y = 20;
  
       BTLinePointEnd.x = 17;
       BTLinePointEnd.y = 15;
       graphics_draw_line(BT1ctx, BTLinePointStart, BTLinePointEnd);
  
       //Line 5a
       BTLinePointStart.x = 11;
       BTLinePointStart.y = 20;
  
       BTLinePointEnd.x = 17;
       BTLinePointEnd.y = 16;
       graphics_draw_line(BT1ctx, BTLinePointStart, BTLinePointEnd);   
      }            
}    

void handle_appfocus(bool in_focus){
    if (in_focus) {
        FirstTime = 0;
    }
}

int ConvertSingleHextoDecimal() {
  int intChar = 0;
  
  ProcessHexRetcode = 0;
  
  if (strcmp(HexIn, "0") == 0)
     {
        intChar = 0;
     } 
     else if (strcmp(HexIn, "1") == 0) 
     {
        intChar = 1;
     }   
     else if (strcmp(HexIn, "2") == 0) 
     {
        intChar = 2;
     }        
     else if (strcmp(HexIn, "3") == 0) 
     {
        intChar = 3;
     }  
     else if (strcmp(HexIn, "4") == 0) 
     {
        intChar = 4;
     }   
     else if (strcmp(HexIn, "5") == 0) 
     {
        intChar = 5;
     }   
     else if (strcmp(HexIn, "6") == 0) 
     {
        intChar = 6;
     }   
     else if (strcmp(HexIn, "7") == 0) 
     {
        intChar = 7;
     }   
     else if (strcmp(HexIn, "8") == 0) 
     {
        intChar = 8;
     }   
     else if (strcmp(HexIn, "9") == 0) 
     {
        intChar = 9;
     }   
     else if (strcmp(HexIn, "A") == 0) 
     {
        intChar = 10;
     }   
     else if (strcmp(HexIn, "B") == 0) 
     {
        intChar = 11;
     }   
     else if (strcmp(HexIn, "C") == 0) 
     {
        intChar = 12;
      }   
     else if (strcmp(HexIn, "D") == 0) 
     {
        intChar = 13;
     }   
     else if (strcmp(HexIn, "E") == 0) 
     {
        intChar = 14;
     }   
     else if (strcmp(HexIn, "F") == 0) 
     {
        intChar = 15;
     }     
     else 
     {
        APP_LOG(APP_LOG_LEVEL_ERROR, "Invalid Char %s passed to Convert SingleHextoDecimal", HexIn);
        ProcessHexRetcode = 1;
     }
  return (intChar); 
  } 

int ConvertHextoDecimal() {
  int  intChar1   = 0;
  int  intChar2   = 0;
  int  intOutput  = 0;
  
  memmove(HexIn, &DoubleHexIn[0], 1);
  intChar1 = ConvertSingleHextoDecimal();
  
  if(ProcessHexRetcode == 0) {
      memmove(HexIn, &DoubleHexIn[1], 1);
  } 
   
  if(ProcessHexRetcode == 0) { 
      intChar2 = ConvertSingleHextoDecimal();
  }   
   
  intOutput = ((intChar1 * 16) + intChar2);    
    
  return (intOutput);
}  

void ProcessHexColor() {
  char strRed[]   = "00";
  char strBlue[]  = "00 ";
  char strGreen[] = "00 ";
 
  
  // Takes hexColorHold in -> ColorHold Out
  memmove(strRed,   &hexColorHold[2], 2);
  memmove(strBlue,  &hexColorHold[4], 2);
  memmove(strGreen, &hexColorHold[6], 2);
  
 // APP_LOG(APP_LOG_LEVEL_INFO, "strRed = %s, strBlue = %s, strGreen = %s", strRed, strBlue, strGreen);
  strcpy(DoubleHexIn, strRed);
  intRed = ConvertHextoDecimal();
  //APP_LOG(APP_LOG_LEVEL_INFO, "intRed = %d", intRed);
  
  if(ProcessHexRetcode == 0) {
     strcpy(DoubleHexIn, strBlue);
     intBlue = ConvertHextoDecimal();
   //  APP_LOG(APP_LOG_LEVEL_INFO, "intBlue = %d", intBlue);
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error in creating intRed");
  }  
   
  if(ProcessHexRetcode == 0) {           
     strcpy(DoubleHexIn, strGreen);
     intGreen = ConvertHextoDecimal();
     // APP_LOG(APP_LOG_LEVEL_INFO, "intGreen = %d", intGreen);
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error in creating intBlue");
  }  

  if(ProcessHexRetcode == 0) {
    ColorHold =  GColorFromRGB(intRed, intBlue, intGreen);
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error in creating intGreen");
    ColorHold =  GColorFromRGB(7, 7, 7); // Fake #
  }  
}

void ProcessTimeZone() {
  char strHours[]="12";
  char strMin[] = "30";
  
  memmove(strSign,  &PersistUTCOffset[0], 1);
  memmove(strHours, &PersistUTCOffset[1], 2);
  memmove(strMin,   &PersistUTCOffset[4], 2);
  
  intUTCoffsethrs = atoi(strHours);
  intUTCoffsetmin = atoi(strMin);
  
  if(strcmp(strSign, "+")) {
    intUTCAdjust = (intUTCoffsethrs * -3600) - (intUTCoffsetmin * 60);
  } else {
    intUTCAdjust = (intUTCoffsethrs * 3600) + (intUTCoffsetmin * 60);
  }  
}

//************************************************************************************************************
void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  char time_format[] = "%I:%M";
  
  time_t seconds_since_epoch;
  seconds_since_epoch = time(NULL);

  strftime(seconds_text, sizeof(seconds_text), "%S", tick_time);

  //******************************************************************
  if((strcmp(seconds_text,"00") == 0) || (FirstTime == 0)) {
     FirstTime = 1;
       
     // Adjust for gmtime 
     ProcessTimeZone(); //convert from character 12:32 to nunmber of seconds to adjust

    if(strcmp(strSign, "+")) {
          seconds_since_epoch = seconds_since_epoch + intUTCAdjust;
     } else {
          seconds_since_epoch = seconds_since_epoch - intUTCAdjust;
     }
    
     //convert seconds since epoch into structure for gmt time
     struct tm* gmtinfo = gmtime(&seconds_since_epoch);
 
     if (clock_is_24h_style()) {
        strcpy(time_format,"%R"); //24 hour HH:MM
     } else {
        strcpy(time_format,"%I:%M"); //12 hour HH:MM   %p = AM/PM
     }

     strftime(time_text, sizeof(time_text),   time_format, tick_time);

     strftime(time2_text, sizeof(time2_text), time_format, gmtinfo);
     
     // Kludge to handle lack of non-padded hour format string
     // for twelve hour clock.
     if (!clock_is_24h_style() && (time_text[0] == '0')) {
         memmove(time_text, &time_text[1], sizeof(time_text) - 1);
     }
  
     if (!clock_is_24h_style() && (time2_text[0] == '0')) {
        memmove(time2_text, &time2_text[1], sizeof(time2_text) - 1);
     }
  
     strftime(date_text,  sizeof(date_text), date_format, tick_time);
    
     strftime(date2_text, sizeof(date2_text), date_format, gmtinfo);
     
     
     text_layer_set_text(text_date_layer,  date_text);
     text_layer_set_text(text_date2_layer, date2_text);
   
     text_layer_set_text(text_time_layer,  time_text);
     text_layer_set_text(text_time2_layer, time2_text);  
    
  }  // End of First Time or 00 seconds

     // Get weather:  
  if((tick_time->tm_min % 15 == 0)  && (tick_time->tm_sec == 0)) { // Only update wx/location every 15 minutes
      APP_LOG(APP_LOG_LEVEL_ERROR, "In 15 minute wx Processing");
      WxLocationCall = 1;   //minHold, intDoWx
        
      // Begin dictionary
      DictionaryIterator *iter;
      app_message_outbox_begin(&iter);

      // Add a key-value pair
      dict_write_uint8(iter, 0, 0);

      // Send the message!
      APP_LOG(APP_LOG_LEVEL_INFO, "before outbox send in wx processing");
      app_message_outbox_send();
    }
}  

//<============================================================================================
//Receive Input from Config html page and location/temp:
void inbox_received_callback(DictionaryIterator *iterator, void *context) {
   char UTCTempHold[] = ":";   
  
    FirstTime = 0;  //Reset so changes will update immediately
    
  //****************
  
    APP_LOG(APP_LOG_LEVEL_WARNING, "In Inbox received callback*****************************************\n");
  
       Tuple *Local_BG_Color = dict_find(iterator, LOCAL_BG_COLOR_KEY);     
    
         if((Local_BG_Color) && (strncmp(Local_BG_Color->value->cstring, "0x", 2) == 0)) {  // config value exists & is valid
               strcpy(PersistLocalBG,Local_BG_Color->value->cstring);
               APP_LOG(APP_LOG_LEVEL_WARNING, "    Added Config Local BG Color: %s", PersistLocalBG);   
            } else { //Check for Persist
               if(persist_exists(LOCAL_BG_COLOR_KEY)) {
                 persist_read_string(LOCAL_BG_COLOR_KEY, PersistLocalBG, sizeof(PersistLocalBG));
                 APP_LOG(APP_LOG_LEVEL_WARNING, "    Added Persistant Local BG Color = %s", PersistLocalBG);
               }  else {
                 strcpy(PersistLocalBG, "0x0000FF"); // Set Default
                 APP_LOG(APP_LOG_LEVEL_WARNING, "    Added Default Local BG Color %s", PersistLocalBG);
               }
            }
         
         persist_write_string(LOCAL_BG_COLOR_KEY,   PersistLocalBG);
          
         strcpy(hexColorHold, PersistLocalBG);

         ProcessHexColor();
        
         text_layer_set_background_color(text_local_layer,    ColorHold);
         text_layer_set_background_color(text_location_layer, ColorHold);
         text_layer_set_background_color(text_degrees_layer,  ColorHold);
         text_layer_set_background_color(text_date_layer,     ColorHold);
         text_layer_set_background_color(text_time_layer,     ColorHold);
 //****************

         Tuple *TZ2_BG_Color =  dict_find(iterator, TZ2_BG_COLOR_KEY);
         
         if((TZ2_BG_Color) && (strncmp(TZ2_BG_Color->value->cstring, "0x", 2) == 0)) {  // config value exists & is valid
            strcpy(PersistTZ2BG,TZ2_BG_Color->value->cstring);
            APP_LOG(APP_LOG_LEVEL_WARNING, "    Added Config Value TZ2 BG Color: %s", hexColorHold); 
         } else { //Check for Persist
            if(persist_exists(TZ2_BG_COLOR_KEY)) {
               persist_read_string(TZ2_BG_COLOR_KEY, PersistTZ2BG, sizeof(PersistTZ2BG));
               APP_LOG(APP_LOG_LEVEL_WARNING, "    Added Persistant TZ2 BG Color = %s", PersistTZ2BG);
            } else {
               strcpy(PersistTZ2BG, "0x00AA00"); // Set Default
               APP_LOG(APP_LOG_LEVEL_WARNING, "    Added Default TZ2 BG Color %s", PersistTZ2BG);
            }         
         }  
         
         persist_write_string(TZ2_BG_COLOR_KEY,     PersistTZ2BG);
         
         strcpy(hexColorHold, PersistTZ2BG);
     
         ProcessHexColor();
        
         text_layer_set_background_color(text_TZ2_layer,       ColorHold);
         text_layer_set_background_color(text_location2_layer, ColorHold);
         text_layer_set_background_color(text_date2_layer,     ColorHold);
         text_layer_set_background_color(text_time2_layer,     ColorHold);


  
  //****************

         Tuple *Local_Text_Color = dict_find(iterator, LOCAL_TEXT_COLOR_KEY); 
         if((Local_Text_Color) && (strncmp(Local_Text_Color->value->cstring, "0x", 2) == 0)) {  // config value exists & is valid
            strcpy(PersistLocalText,Local_Text_Color->value->cstring);
            APP_LOG(APP_LOG_LEVEL_WARNING, "    Added Config Value Local Text Color: %s", PersistLocalText); 
         } else { //Check for Persist
             if(persist_exists(LOCAL_TEXT_COLOR_KEY)) {
               persist_read_string(LOCAL_TEXT_COLOR_KEY, PersistLocalText, sizeof(PersistLocalText));
               APP_LOG(APP_LOG_LEVEL_WARNING, "    Added Persistant Local Text Color = %s", PersistLocalText);
            } else {
               strcpy(PersistLocalText, "0xFFFFFF"); // Set Default
               APP_LOG(APP_LOG_LEVEL_WARNING, "    Added Default Local Text Color %s", PersistLocalText);
            } 
         }  
  
         persist_write_string(LOCAL_TEXT_COLOR_KEY, PersistLocalText);
  
         strcpy(hexColorHold, PersistLocalText);
         
         ProcessHexColor();
       
         text_layer_set_text_color(text_local_layer,    ColorHold);
         text_layer_set_text_color(text_location_layer, ColorHold);
         text_layer_set_text_color(text_degrees_layer,  ColorHold);
         text_layer_set_text_color(text_date_layer,     ColorHold);
         text_layer_set_text_color(text_time_layer,     ColorHold);
  
  //****************

         Tuple *TZ2_Text_Color = dict_find(iterator, TZ2_TEXT_COLOR_KEY);
         
         if((TZ2_Text_Color) && (strncmp(TZ2_Text_Color->value->cstring, "0x", 2) == 0)) {  // config value exists & is valid
            strcpy(PersistTZ2Text,TZ2_Text_Color->value->cstring);
            APP_LOG(APP_LOG_LEVEL_WARNING, "    Added Config Value TZ2 Text Color: %s", PersistTZ2Text); 
         } else {
            if(persist_exists(TZ2_TEXT_COLOR_KEY)) {
               persist_read_string(TZ2_TEXT_COLOR_KEY, PersistTZ2Text, sizeof(PersistTZ2Text));
               APP_LOG(APP_LOG_LEVEL_WARNING, "    Added Persistant TZ2 Text Color = %s", PersistTZ2Text);
            } else {
               strcpy(PersistTZ2Text, "0xFFFFFF"); // Set Default
               APP_LOG(APP_LOG_LEVEL_WARNING, "    Added Default TZ2 Text Color %s", PersistTZ2Text);
            }         
         }

         persist_write_string(TZ2_TEXT_COLOR_KEY,   PersistTZ2Text);
 
         strcpy(hexColorHold, PersistTZ2Text);
         
         ProcessHexColor();
  
         TextColorHold2 = ColorHold; //Save for BT callback
         text_layer_set_text_color(text_TZ2_layer,       ColorHold);
         text_layer_set_text_color(text_location2_layer, ColorHold);
         text_layer_set_text_color(text_date2_layer,     ColorHold);
         text_layer_set_text_color(text_time2_layer,     ColorHold);
  


  //****************
        Tuple *Date_Format = dict_find(iterator, DATE_FORMAT_KEY);
        
        if((Date_Format) && ((Date_Format->value->uint8 == 0) || (Date_Format->value->uint8 == 1))) {
            PersistDateFormat = Date_Format->value->uint8;
            APP_LOG(APP_LOG_LEVEL_WARNING, "    Added Config Value Date Format = %d, 1=US, 0=Intl", PersistDateFormat);
         } else {
            if(persist_exists(DATE_FORMAT_KEY)) {
               PersistDateFormat = persist_read_int(DATE_FORMAT_KEY);
               APP_LOG(APP_LOG_LEVEL_WARNING, "    Added Persistant Date Format = %d, (1=US, 0=Intl)", PersistDateFormat);
            } else {
               PersistDateFormat = 1; // US Default
               APP_LOG(APP_LOG_LEVEL_WARNING, "    Added Default Date Format (US) = %d", PersistDateFormat);
            }             
         }
          persist_write_int(DATE_FORMAT_KEY, PersistDateFormat);

         #ifdef PBL_PLATFORM_CHALK
             if (PersistDateFormat == 1) { // US
                 strcpy(date_format, "  %b %e, %Y");
             } else {
                 strcpy(date_format, "  %e %b %Y");   //Intl
             }
         #else
             if (PersistDateFormat == 1) { // US
                 strcpy(date_format, "%b %e, %Y");
             } else {
                 strcpy(date_format, "%e %b %Y");   //Intl
             }     
         #endif
  
        text_layer_set_text(text_date_layer, date_text);

  //****************

        Tuple *BT_LossVib = dict_find(iterator, BT_LOSS_KEY); 
  
       if((BT_LossVib) && ((BT_LossVib->value->uint8 == 0) || (BT_LossVib->value->uint8 == 1))) {  
           PersistBTLoss = BT_LossVib->value->uint8; //Vibe on loss
           APP_LOG(APP_LOG_LEVEL_WARNING, "    Added Config BT Loss Key = %d (0=No Vib, 1=Vib)", PersistBTLoss);
       } else {
         if(persist_exists(BT_LOSS_KEY)) {
               PersistBTLoss = persist_read_int(BT_LOSS_KEY);
               APP_LOG(APP_LOG_LEVEL_WARNING, "    Added Persistant BT Loss = %d, (0 = No Vib, 1 = Vib)", PersistBTLoss);
            } else {
               PersistBTLoss = 0; // No Vib on BT Loss
               APP_LOG(APP_LOG_LEVEL_WARNING, "    Added Default BT Loss Key = %d (No Vibrate on Loss)", PersistBTLoss);
            }
       }
        persist_write_int(BT_LOSS_KEY, PersistBTLoss);

 //******************
        Tuple *Low_Battery_Vib = dict_find(iterator, LOW_BATTERY_KEY);
      
        if(PersistLow_Batt) { 
           PersistLow_Batt = Low_Battery_Vib->value->uint8;
        
           switch(PersistLow_Batt) {
       
           case 0:   
              APP_LOG(APP_LOG_LEVEL_WARNING, "    Added Config Low Battery Key = 0, No Vibration");
              break;
           
           case 1:
              APP_LOG(APP_LOG_LEVEL_WARNING, "    Added Config Low Battery Key = 1, Vibration"); 
              break;
      
           default:
             PersistLow_Batt = 0; // No Vib on Low Batt
             APP_LOG(APP_LOG_LEVEL_WARNING, "    Added Default Low Battery Key = %d (No Vibrate on Loss)", PersistBTLoss);
           }
        } else {      
             if(persist_exists(LOW_BATTERY_KEY)) {
                 PersistLow_Batt = persist_read_int(LOW_BATTERY_KEY);
                 APP_LOG(APP_LOG_LEVEL_WARNING, "    Added Persistant Low Battery = %d, (0 = No Vib, 1 = Vib)", PersistBTLoss);
            } else {
                 PersistLow_Batt = 0; // No Vib on Low Batt
                 APP_LOG(APP_LOG_LEVEL_WARNING, "    Added Default Low Battery Key = %d (No Vibrate on Loss)", PersistBTLoss);
            }      
        }
        persist_write_int(LOW_BATTERY_KEY, PersistLow_Batt);

        
  //******************
        Tuple *UTC_Offset = dict_find(iterator, UTC_OFFSET_KEY);
              
        if(UTC_Offset) {  // config value exists
           strcpy(UTCOffsetConfig, UTC_Offset->value->cstring);
           memmove(UTCTempHold, &UTCOffsetConfig[3], 1);
           APP_LOG(APP_LOG_LEVEL_WARNING,    "    UTC Test Char = %s", UTCTempHold);  
          
           if(strcmp(UTCTempHold, ":") == 0) {
              strcpy(PersistUTCOffset, UTCOffsetConfig); 
              APP_LOG(APP_LOG_LEVEL_WARNING, "    Added Config UTC Offset = %s", PersistUTCOffset);
           } else { 
              if(persist_exists(UTC_OFFSET_KEY)) {
                 persist_read_string(UTC_OFFSET_KEY, PersistUTCOffset, sizeof(PersistUTCOffset));
                 APP_LOG(APP_LOG_LEVEL_WARNING, "    Added Persistant UTC Offset = %s", PersistUTCOffset);
              } else {
               strcpy(PersistUTCOffset, "+00:00"); // Default to UTC
               APP_LOG(APP_LOG_LEVEL_WARNING, "    Added Default Persistant UTC Offset = %s", PersistUTCOffset);
              }
           }  
        } 
          
        persist_write_string(UTC_OFFSET_KEY, PersistUTCOffset);

  //******************
        
         Tuple *Location_name = dict_find(iterator, LOCATION_NAME_KEY);
         
         if(Location_name) {
            strcpy(PersistLocationName, Location_name->value->cstring);
            APP_LOG(APP_LOG_LEVEL_WARNING, "    Added Config TZ2 Location:  %s\n", PersistLocationName);
         } else {
            if(persist_exists(LOCATION_NAME_KEY)) {
               persist_read_string(LOCATION_NAME_KEY, PersistLocationName, sizeof(PersistLocationName));
               APP_LOG(APP_LOG_LEVEL_WARNING, "    Added Persistant TZ2 Location = %s", PersistLocationName);
            } else {
               strcpy(PersistLocationName, "UTC"); // Default to UTC
               APP_LOG(APP_LOG_LEVEL_WARNING, "    Added Default TZ2 Location = %s", PersistLocationName);
            }
         }  
         text_layer_set_text(text_location2_layer, PersistLocationName);
         
         persist_write_string(LOCATION_NAME_KEY,   PersistLocationName);
         
        //******************

        if(WxLocationCall == 1) {
            APP_LOG(APP_LOG_LEVEL_WARNING, "Processing Temp...");
       
            Tuple *Temperature = dict_find(iterator, WEATHER_TEMPERATURE_KEY);
            
            strcpy(text_degrees,(Temperature->value->cstring));
            if (strcmp((text_degrees), "N/A") != 0) {
                int tempint = 100;
                APP_LOG(APP_LOG_LEVEL_WARNING, "    Degrees C = %s", text_degrees);
                tempint = atoi(text_degrees);
  
                tempint = ((tempint * 9) / 5) + 32;
  
                // Assemble full string and display
             snprintf(text_degrees, 5, "%dF ", tempint);       
           } else {
                strcpy(text_degrees, "N/A");  
           } 
        
           text_layer_set_text(text_degrees_layer, text_degrees); 
           APP_LOG(APP_LOG_LEVEL_WARNING, "    Degrees F = %s\n", text_degrees);        
       
   //******************
       APP_LOG(APP_LOG_LEVEL_WARNING, "Processing Wx Location Name...");
      // if(WxLocationCall == 1) {
          Tuple *Wx_City = dict_find(iterator, WEATHER_CITY_KEY);
          strcpy(text_location,Wx_City->value->cstring) ;      
          text_layer_set_text(text_location_layer, text_location);
          WxLocationCall = 0;
          APP_LOG(APP_LOG_LEVEL_WARNING, "    Location = %s\n", text_location);
      
   WxLocationCall = 0;   
 }     
  
}

void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Inbox Message dropped!");
}

void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

void handle_deinit(void) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  app_focus_service_unsubscribe();
  app_message_deregister_callbacks();
  
  text_layer_destroy(text_TZ2_layer);
  text_layer_destroy(text_time_layer);
  text_layer_destroy(text_time2_layer);
  text_layer_destroy(text_date_layer);
  text_layer_destroy(text_date2_layer);
  text_layer_destroy(text_location_layer);
  text_layer_destroy(text_location2_layer);
  text_layer_destroy(text_degrees_layer);
   
  layer_destroy(BatteryLineLayer);   
  layer_destroy(BTLayer);

  fonts_unload_custom_font(fontMonaco13);
  fonts_unload_custom_font(fontRobotoBoldSubset30);
  fonts_unload_custom_font(fontRobotoBoldSubset35);

  window_destroy(window);
}

void handle_init(void) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Enterng INIT************************************************************");
  APP_LOG(APP_LOG_LEVEL_ERROR, "*************************************************************************");

  FirstTime = 0;
  WxLocationCall = 1;
  
 #ifdef PBL_PLATFORM_APLITE 
     GColor BGCOLOR1 = GColorBlack;
  
     GColor BGCOLOR2 = GColorDarkGray;
     BGColorHold2    = GColorDarkGray;

     GColor TEXTCOLOR = GColorWhite;
     TextColorHold1   = GColorWhite;
     TextColorHold2   = GColorWhite;
  #else
     GColor BGCOLOR1 = GColorBlue;
  
     GColor BGCOLOR2 = GColorDarkGreen;
     BGColorHold2 = GColorDarkGreen;

     GColor TEXTCOLOR = GColorWhite;
     TextColorHold1   = GColorWhite;
     TextColorHold2   = GColorWhite;
 #endif
  
  window = window_create();
  window_stack_push(window, true /* Animated */);
  window_set_background_color(window, BGCOLOR1);

  fontMonaco13           = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_MONACO_13));
  fontRobotoBoldSubset30 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_30));
  fontRobotoBoldSubset35 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_35));
  
  Layer *window_layer = window_get_root_layer(window);
  
  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);

  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // Open AppMessage
  app_message_open(256, 256);

  //Local Time Layer
  #ifdef PBL_PLATFORM_CHALK
    text_local_layer = text_layer_create(GRect(1, 1, 180, 80)); 
  #else
    text_local_layer = text_layer_create(GRect(1, 1, 144, 92)); 
  #endif 
  
  text_layer_set_background_color(text_local_layer, BGCOLOR1);
  layer_add_child(window_layer, text_layer_get_layer(text_local_layer));
  
  //TZ2 Layer
  #ifdef PBL_PLATFORM_CHALK
    text_TZ2_layer = text_layer_create(GRect(1, 127, 180, 53)); 
  #else
    text_TZ2_layer = text_layer_create(GRect(1, 127, 144, 41));   
  #endif
 
  text_layer_set_background_color(text_TZ2_layer, BGCOLOR2);
  layer_add_child(window_layer, text_layer_get_layer(text_TZ2_layer));

  //Location 1 - Local
  #ifdef PBL_PLATFORM_CHALK
    text_location_layer = text_layer_create(GRect(1, 44, 180, 17)); 
  #else
    text_location_layer = text_layer_create(GRect(1, 1, 144, 17)); 
  #endif
  
  text_layer_set_text_alignment(text_location_layer, GTextAlignmentCenter);		
  text_layer_set_text(text_location_layer, text_location); 
  text_layer_set_font(text_location_layer, fontMonaco13);
  text_layer_set_background_color(text_location_layer, BGCOLOR1);
  text_layer_set_text_color(text_location_layer, TEXTCOLOR);
  layer_add_child(window_layer, text_layer_get_layer(text_location_layer));
  
  // Date 1
  #ifdef PBL_PLATFORM_CHALK
    text_date_layer = text_layer_create(GRect(1, 57, 180, 22));
    text_layer_set_text_alignment(text_date_layer, GTextAlignmentLeft);;
  #else
    text_date_layer = text_layer_create(GRect(1, 18, 144, 22));
    text_layer_set_text_alignment(text_date_layer, GTextAlignmentCenter);; 
  #endif
  
  text_layer_set_text_color(text_date_layer, TEXTCOLOR);
  text_layer_set_background_color(text_date_layer, BGCOLOR1);
  text_layer_set_font(text_date_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
  layer_add_child(window_layer, text_layer_get_layer(text_date_layer));

  //Temperature
  #ifdef PBL_PLATFORM_CHALK
     text_degrees_layer = text_layer_create(GRect(130, 65, 40, 17)); 
  #else
     text_degrees_layer = text_layer_create(GRect(104, 55, 40, 17));
  #endif
 
  text_layer_set_text_alignment(text_degrees_layer, GTextAlignmentRight);		
  text_layer_set_text(text_degrees_layer, text_degrees); 
  text_layer_set_font(text_degrees_layer, fontMonaco13);
  text_layer_set_background_color(text_degrees_layer, BGCOLOR1);
  text_layer_set_text_color(text_degrees_layer, TEXTCOLOR);
  layer_add_child(window_layer, text_layer_get_layer(text_degrees_layer));
  
  
  // Time of Day 1
  #ifdef PBL_PLATFORM_CHALK
     text_time_layer = text_layer_create(GRect(1, 6, 180, 40)); // 
     text_layer_set_font(text_time_layer, fontRobotoBoldSubset30);
  #else
     text_time_layer = text_layer_create(GRect(1, 40, 104, 40)); 
     text_layer_set_font(text_time_layer, fontRobotoBoldSubset35);
  #endif
  
  text_layer_set_text_color(text_time_layer, TEXTCOLOR);
  text_layer_set_background_color(text_time_layer, BGCOLOR1);
  text_layer_set_text_alignment(text_time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_time_layer));
  
   // Battery Line
  #ifdef PBL_PLATFORM_CHALK
      GRect battery_line_frame = GRect(38, 84, 104, 6);
  #else
      GRect battery_line_frame = GRect(22, 84, 104, 6);
  #endif
  
  BatteryLineLayer = layer_create(battery_line_frame);
  layer_set_update_proc(BatteryLineLayer, battery_line_layer_update_callback);
  layer_add_child(window_layer, BatteryLineLayer);
  
  // Date 2
  #ifdef PBL_PLATFORM_CHALK
     text_date2_layer = text_layer_create(GRect(1, 94, 180, 29));
     text_layer_set_text_alignment(text_date2_layer, GTextAlignmentLeft);;
  #else
     text_date2_layer = text_layer_create(GRect(1, 106, 144, 22));
     text_layer_set_text_alignment(text_date2_layer, GTextAlignmentCenter);;
  #endif
  
  text_layer_set_text_color(text_date2_layer, TEXTCOLOR);
  text_layer_set_background_color(text_date2_layer, BGCOLOR2);
  text_layer_set_font(text_date2_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
  layer_add_child(window_layer, text_layer_get_layer(text_date2_layer));

  // Time 2
  #ifdef PBL_PLATFORM_CHALK
     text_time2_layer = text_layer_create(GRect(1, 135, 180, 40));
     text_layer_set_font(text_time2_layer,fontRobotoBoldSubset30);
  #else
    text_time2_layer = text_layer_create(GRect(1, 127, 104, 40));
    text_layer_set_font(text_time2_layer,fontRobotoBoldSubset35);
  #endif
  
  text_layer_set_text_color(text_time2_layer, TEXTCOLOR);
  text_layer_set_background_color(text_time2_layer, BGCOLOR2);
  text_layer_set_text_alignment(text_time2_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_time2_layer)); 

  //Location 2
  #ifdef PBL_PLATFORM_CHALK
      text_location2_layer = text_layer_create(GRect(1, 123, 180, 19)); 
  #else
      text_location2_layer = text_layer_create(GRect(1, 92, 144, 19));     
  #endif
  
  text_layer_set_text_alignment(text_location2_layer, GTextAlignmentCenter);		
  text_layer_set_text(text_location2_layer, text_location2); 
  text_layer_set_font(text_location2_layer, fontMonaco13);
  text_layer_set_background_color(text_location2_layer, BGCOLOR2);
  text_layer_set_text_color(text_location2_layer, TEXTCOLOR);
  layer_add_child(window_layer, text_layer_get_layer(text_location2_layer));
  
  //Bluetooth Logo Setup area
  #ifdef PBL_PLATFORM_CHALK
     GRect BTArea = GRect(135, 96, 20, 20);
  #else
     GRect BTArea = GRect(110, 138, 20, 20);
  #endif
  
  BTLayer = layer_create(BTArea);

  layer_add_child(window_layer, BTLayer);
  layer_set_update_proc(BTLayer, BTLine_update_callback);
    
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
