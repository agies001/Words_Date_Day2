#include "num2words.h"

static GColor FGCOLOR=GColorBlack;
static GColor BGCOLOR=GColorWhite;
static const int INVERTED=1;

#define TIME_SLOT_ANIMATION_DURATION 700
#define NUM_LAYERS 4

enum layer_names {
	HOURS,
	MINUTES,
	TENS,
	DATE,
};

Window *window;
TextLayer *text_date_layer;

typedef struct CommonWordsData {
  TextLayer *textLayer;
  Layer *layer;
  PropertyAnimation *in_animation;
  PropertyAnimation *out_animation;
  void (*update) (struct tm *t, char *words);
  char buffer[BUFFER_SIZE];
  GFont font;
} CommonWordsData;

static GFont font_gothic_bold;
static GFont font_bitham_bold;
static GFont font_bitham_light;

static struct CommonWordsData layers[NUM_LAYERS] =
{{ .update = &fuzzy_hours_to_words },
 { .update = &fuzzy_sminutes_to_words },
 { .update = &fuzzy_minutes_to_words },
 { .update = &fuzzy_dates_to_words },
};

static bool tens_update[60] = {
	true, true, false, false, false, false, false, false, false, false, 
	true, true, true, true, true, true, true, true, true, true, 
	true, false, false, false, false, false, false, false, false, false, 
	true, false, false, false, false, false, false, false, false, false, 
	true, false, false, false, false, false, false, false, false, false, 
	true, false, false, false, false, false, false, false, false, false, 
};

static bool minutes_update[60] = {
	true, true, true, true, true, true, true, true, true, true, 
	true, false, false, false, true, true, true, true, true, true, 
	true, true, true, true, true, true, true, true, true, true, 
	true, true, true, true, true, true, true, true, true, true, 
	true, true, true, true, true, true, true, true, true, true, 
	true, true, true, true, true, true, true, true, true, true, 
};

void slide_out_animation_stopped(Animation *slide_out_animation, bool finished,
                                  void *context) {
  CommonWordsData *layerComponents = (CommonWordsData *)context;
  	time_t now = time(NULL);
 	struct tm *t = localtime(&now);
	  text_layer_set_font(layerComponents->textLayer, layerComponents->font);
  layerComponents->update(t, layerComponents->buffer);
  text_layer_set_text(layerComponents->textLayer, layerComponents->buffer);
  animation_schedule(&layerComponents->in_animation->animation);
}

void update_layer(CommonWordsData *layerComponents) {
  //slide_out(&layer->out_animation, layer);
  animation_set_handlers(&layerComponents->out_animation->animation, (AnimationHandlers){
    .stopped = (AnimationStoppedHandler)slide_out_animation_stopped
  }, (void *) layerComponents);
  animation_schedule(&layerComponents->out_animation->animation);
}

static void handle_minute_tick(struct tm* tick_time, TimeUnits units_changed) {
  struct tm *t = tick_time;
  if((units_changed & MINUTE_UNIT) == MINUTE_UNIT) {
    if (minutes_update[t->tm_min]) {
      update_layer(&layers[MINUTES]);
    }
	if (tens_update[t->tm_min]) {
		layers[TENS].font = font_bitham_light;
		update_layer(&layers[TENS]);
    }
  }
  if ((units_changed & HOUR_UNIT) == HOUR_UNIT ||
        ((t->tm_hour == 00 || t->tm_hour == 12) && t->tm_min == 01)) {
    update_layer(&layers[HOURS]);
  }
  if ((units_changed & DAY_UNIT) == DAY_UNIT) {
		layers[DATE].font = font_gothic_bold;
	  	update_layer(&layers[DATE]);
  }
}

void init_layer(CommonWordsData *layerComponents, int y, int h, GFont font) {
	  Layer * rootLayer = window_get_root_layer(window);
      GRect rootFrame = layer_get_frame(rootLayer);
	  GRect layerRect = GRect(0, y, rootFrame.size.w, h);
	  layerComponents->textLayer = text_layer_create(layerRect);
	  layerComponents->layer = text_layer_get_layer(layerComponents->textLayer);
	  layerComponents->font = font;
	  text_layer_set_background_color(layerComponents->textLayer, BGCOLOR);
	  text_layer_set_text_color(layerComponents->textLayer, FGCOLOR);
	  text_layer_set_font(layerComponents->textLayer, font);
	  layer_add_child(rootLayer, layerComponents->layer);
	  
      GRect layerFrame = layer_get_frame(layerComponents->layer);
  	  GRect to_frame = GRect(-rootFrame.size.w, layerFrame.origin.y, rootFrame.size.w, layerFrame.size.h);
	  layerComponents->out_animation = property_animation_create_layer_frame(layerComponents->layer, &layerFrame, &to_frame);
  	  animation_set_duration(&layerComponents->out_animation->animation, TIME_SLOT_ANIMATION_DURATION);
  	  animation_set_curve(&layerComponents->out_animation->animation, AnimationCurveEaseIn);
	
	  GRect from_frame = GRect(2 * rootFrame.size.w, layerFrame.origin.y, rootFrame.size.w, layerFrame.size.h);	  
	  layerComponents->in_animation = property_animation_create_layer_frame(layerComponents->layer, &from_frame, &layerFrame);
  	  animation_set_duration(&layerComponents->in_animation->animation, TIME_SLOT_ANIMATION_DURATION);
  	  animation_set_curve(&layerComponents->in_animation->animation, AnimationCurveEaseOut);
}

void show_layer(CommonWordsData *layerComponents, struct tm *t) {
    text_layer_set_font(layerComponents->textLayer, layerComponents->font);
    layerComponents->update(t, layerComponents->buffer);
    animation_schedule(&layerComponents->in_animation->animation);
}

void handle_init() {

 	const bool animated = true;
	window = window_create();
	window_stack_push(window, animated);
//        persist_write_bool(INVERTED, false);
//        if (persist_read_bool(INVERTED)){
//           BGCOLOR=GColorWhite;
//           FGCOLOR=GColorBlack;
//        }
//        else {
//           FGCOLOR=GColorWhite;
//           BGCOLOR=GColorBlack;
//        }
	window_set_background_color(window, BGCOLOR);

	font_bitham_light = fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT);
	font_bitham_bold = fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD);
	font_gothic_bold = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);

  init_layer(&layers[MINUTES], 42*2-8, 42+8, font_bitham_light);
  init_layer(&layers[TENS], 42-6, 42+8, font_bitham_light);
  init_layer(&layers[HOURS], 0-8, 42+6, font_bitham_bold);
  init_layer(&layers[DATE], 42*3-2, 42+4, font_gothic_bold);

  	time_t now = time(NULL);
 	struct tm *t = localtime(&now);

  for (int i = 0; i < NUM_LAYERS; ++i)
  {
    text_layer_set_font(layers[i].textLayer, layers[i].font);
    layers[i].update(t, layers[i].buffer);
//    slide_in(&layers[i].in_animation, &layers[i]);
  	text_layer_set_text(layers[i].textLayer, layers[i].buffer);
    animation_schedule(&layers[i].in_animation->animation);
  }
	tick_timer_service_subscribe(MINUTE_UNIT, &handle_minute_tick);

}

static void handle_deinit()
{
	tick_timer_service_unsubscribe();
	for (int i = 0; i < NUM_LAYERS; ++i)
	{
		property_animation_destroy(layers[i].in_animation);
		property_animation_destroy(layers[i].out_animation);
		text_layer_destroy(layers[i].textLayer);
	}
	window_destroy(window);
}

int main() {
	handle_init();
	app_event_loop();
	handle_deinit();
}
