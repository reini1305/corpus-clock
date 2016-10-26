#include <pebble.h>

static Window *s_window;
static Layer *s_background_layer;
static int s_second_angle;
static int s_minute_angle;
static int s_hour_angle;
static Animation *s_animation;
static int s_animation_percent;

static void implementation_update(Animation *animation,
                                  const AnimationProgress progress) {
  // Animate some completion variable
  s_animation_percent = ((int)progress * 100) / ANIMATION_NORMALIZED_MAX;

  layer_mark_dirty(s_background_layer);
}

static void start_animation(void) {
  s_animation = animation_create();
  animation_set_duration(s_animation, 500);

  // Create the AnimationImplementation
  static const AnimationImplementation implementation = {
    .update = implementation_update
  };
  animation_set_implementation(s_animation, &implementation);

  animation_schedule(s_animation);

}

static void handle_tick(struct tm *tick_time, TimeUnits changed) {
  s_second_angle =  TRIG_MAX_ANGLE * tick_time->tm_sec / 60;
  s_minute_angle =  TRIG_MAX_ANGLE * tick_time->tm_min / 60;
  s_hour_angle =  (TRIG_MAX_ANGLE * (((tick_time->tm_hour % 12) * 6) + (tick_time->tm_min / 10))) / (12 * 6);

  start_animation();
}

static void background_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  #if PBL_API_EXISTS(layer_get_unobstructed_bounds)
    uint8_t offset_from_bottom = 0;
    GRect unobstructed_bounds = layer_get_unobstructed_bounds(layer);
    // move everything up by half the obstruction
    offset_from_bottom = (bounds.size.h - unobstructed_bounds.size.h) / 2;
    bounds.origin.y-=offset_from_bottom;
  #endif
  const GRect second_inset = grect_inset(bounds, GEdgeInsets(10));
  const GRect minute_inset = grect_inset(second_inset, GEdgeInsets(20));
  const GRect hour_inset = grect_inset(minute_inset, GEdgeInsets(30));

  GPoint pos_inner, pos_outer;

  // clear background
  graphics_fill_rect(ctx,bounds,0,GCornerNone);

  graphics_context_set_stroke_color(ctx,GColorPictonBlue);
  graphics_context_set_stroke_width(ctx,4);

  // draw second
  int curr_angle = s_second_angle + (TRIG_MAX_ANGLE * s_animation_percent) / 100;
  pos_inner = gpoint_from_polar(second_inset, GOvalScaleModeFitCircle, curr_angle);
  pos_outer = gpoint_from_polar(bounds, GOvalScaleModeFitCircle, curr_angle);
  graphics_draw_line(ctx,pos_inner,pos_outer);

  //draw minute
  curr_angle = s_minute_angle;
  if(s_second_angle == 0)
    curr_angle += (TRIG_MAX_ANGLE * s_animation_percent) / 100;
  pos_inner = gpoint_from_polar(minute_inset, GOvalScaleModeFitCircle, curr_angle);
  pos_outer = gpoint_from_polar(second_inset, GOvalScaleModeFitCircle, curr_angle);
  graphics_draw_line(ctx,pos_inner,pos_outer);

  pos_inner = gpoint_from_polar(hour_inset, GOvalScaleModeFitCircle, s_hour_angle);
  pos_outer = gpoint_from_polar(minute_inset, GOvalScaleModeFitCircle, s_hour_angle);
  graphics_draw_line(ctx,pos_inner,pos_outer);
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_background_layer = layer_create(bounds);
  layer_set_update_proc(s_background_layer, background_update_proc);
  layer_add_child(window_layer, s_background_layer);

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  handle_tick(t, SECOND_UNIT);

  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
}

static void prv_window_unload(Window *window) {

}

static void prv_init(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  const bool animated = true;
  window_stack_push(s_window, animated);
}

static void prv_deinit(void) {
  window_destroy(s_window);
}

int main(void) {
  prv_init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);

  app_event_loop();
  prv_deinit();
}
