#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#include "resource_ids.auto.h"


#define MY_UUID { 0x18, 0x36, 0x44, 0x34, 0xB8, 0x65, 0x42, 0xC3, 0x95, 0x4E, 0x41, 0x2B, 0xFE, 0xC3, 0x39, 0x13 }
#define SCREEN_WIDTH  144
#define SCREEN_HEIGHT  168
#define SEC_LAYER_COUNT 12
#define MIN_BOTTOM_COUNT 10
#define HOUR_LAYER_COUNT 24
#define MIN_TOP_COUNT 6
#define DOW_LAYER_COUNT 7
#define FOREGROUND_COLOR GColorWhite
#define BACKGROUND_COLOR GColorBlack
#define USE_VIBRATION true

#define HR_START_X  28
#define SEC_START_X 80
#define MIN_START_Y 48
#define MIN_TOP_START_X 68 
#define MIN_BOT_START_X 90
#define DOW_START_X 100
#define DOW_START_Y 100

#define GPATH_INIT(PATH, POINTS) ({ \
GPathInfo __info = { sizeof(POINTS) / sizeof(*POINTS), POINTS }; \
gpath_init(PATH, &__info); \
})
	
PBL_APP_INFO(MY_UUID,
             "Belt Drive", "Tickitty Tock",
             1, 2, /* App version */
             RESOURCE_ID_MENU_ICON,
             APP_INFO_WATCH_FACE);

Window window;

typedef struct BeltLayer {
	Layer layer;
	TextLayer textLayer;
	GPoint newOrigin;
	char text[4];
	GFont font;
	GPoint orig_point;
	GTextAlignment text_alignment:2;
	GColor foreground_color:2;
} BeltLayer;

int current_HOUR_LAYER_COUNT;


Layer m_sec_bg_Layer;
BeltLayer m_sec_Layer[SEC_LAYER_COUNT];
Animation prop_ani;
AnimationImplementation m_Sec_Animation;
int sec_animation_duration;
GPoint m_sec_total_movement;
GRect m_sec_Bounds;
int m_curSec;
int m_reqSec;

Layer m_hr_bg_Layer;
bool m_hr_init;
BeltLayer m_hr_Layer[HOUR_LAYER_COUNT];
Animation prop_ani_hr;
int hr_animation_duration;
GPoint m_hr_total_movement;
AnimationImplementation m_HR_Animation;
GRect m_hr_Bounds;
int m_curHR;
int m_reqHR;

Layer m_min_bg_Layer;
BeltLayer m_min_bottom_Layer[MIN_BOTTOM_COUNT];
BeltLayer m_min_top_Layer[MIN_TOP_COUNT];
AnimationImplementation m_MinTop_Animation, m_MinBottom_Animation;
Animation prop_ani_bmin, prop_ani_tmin;
int min_top_animation_duration, min_bottom_animation_duration;
GPoint m_min_top_total_movement, m_min_bot_total_movement;
GRect m_min_top_Bounds, m_min_bottom_Bounds;
int m_curMinBot, m_curMinTop;
int m_reqMinBot, m_reqMinTop;

Layer m_dow_bg_Layer;
bool m_dow_init;
BeltLayer m_dow_Layer[DOW_LAYER_COUNT];
Animation prop_ani_dow;
int dow_animation_duration;
AnimationImplementation m_dow_Animation;
GRect m_dow_Bounds;
int m_curdow;
int m_reqdow;


Layer m_overlay;

void hr_bg_display_layer_update_callback(Layer *me, GContext* ctx)
{
	graphics_context_set_fill_color(ctx, BACKGROUND_COLOR);
	graphics_fill_rect(ctx, me->bounds, 0, GCornersAll);
    graphics_context_set_stroke_color(ctx, FOREGROUND_COLOR);
	graphics_draw_line(ctx, GPoint(0, 0), GPoint(me->bounds.size.w, 0));
	graphics_draw_line(ctx, GPoint(0, me->bounds.size.h -1), GPoint(me->bounds.size.w, me->bounds.size.h-1));
}

void sec_bg_display_layer_update_callback(Layer *me, GContext* ctx)
{
	graphics_context_set_fill_color(ctx, BACKGROUND_COLOR);
	graphics_fill_rect(ctx, me->bounds, 0, GCornersAll);
    graphics_context_set_stroke_color(ctx, FOREGROUND_COLOR);
	graphics_draw_line(ctx, GPoint(0, 0), GPoint(me->bounds.size.w, 0));
	graphics_draw_line(ctx, GPoint(0, me->bounds.size.h -1), GPoint(me->bounds.size.w, me->bounds.size.h-1));
}

void min_bg_display_layer_update_callback(Layer *me, GContext* ctx)
{
	static GPath minute_path;
	static GPoint minute_points[] = {
    { 0, SCREEN_HEIGHT },
    { 75, -7 },
    { 99, -7 },
    { 24, SCREEN_HEIGHT },
    { 0, SCREEN_HEIGHT }
	};
	
	GPATH_INIT(&minute_path, minute_points);
	graphics_context_set_fill_color(ctx, BACKGROUND_COLOR);
    gpath_draw_filled(ctx, &minute_path);
    graphics_context_set_stroke_color(ctx, FOREGROUND_COLOR);
    gpath_draw_outline(ctx, &minute_path);

	static GPath minute_path2;
	static GPoint minute_points2[] = {
    { 24, SCREEN_HEIGHT },
    { 99, -7 },
    { 122, -7 },
    { 47, SCREEN_HEIGHT },
    { 24, SCREEN_HEIGHT }
	};
	
	GPATH_INIT(&minute_path2, minute_points2);
    gpath_draw_filled(ctx, &minute_path2);
    gpath_draw_outline(ctx, &minute_path2);
}

void beltlayer_update_callback(BeltLayer *me, GContext* ctx)
{
	/*
	//Code for debugging, leave it here for now.
	GRect rect = layer_get_frame(&(me->layer));
	GRect rect2 = GRect(0, 0, rect.size.w, rect.size.h);
	graphics_context_set_stroke_color(ctx, FOREGROUND_COLOR);
	graphics_draw_line(ctx, rect2.origin, GPoint(rect2.size.w, rect2.size.h));								 
	*/
	graphics_context_set_text_color(ctx, me->foreground_color);
	
/*
	//Code for debugging, leave it here for now.
	text_layer_init(&(me->textLayer), window.layer.frame);
  text_layer_set_text_color(&(me->textLayer), me->foreground_color);
  text_layer_set_background_color(&(me->textLayer), GColorClear);
  layer_set_frame(&(me->textLayer).layer, me->layer.bounds);
  text_layer_set_font(&(me->textLayer), me->font);
  layer_add_child(&window.layer, &(me->layer));
text_layer_set_text(&(me->textLayer), me->text);	
*/	

	graphics_text_draw(ctx, 
					   me->text, 
					   me->font, 
					   me->layer.bounds, 
					   GTextOverflowModeWordWrap, 
					   me->text_alignment, 
					   NULL);
					   
}

float tween(float t, float x, float d){
	t/=d;
	float ts = t*t;
	float tc =ts*t;
	return x*(-2*tc + 3*ts);
}

void animate_Hrs(Animation* animation, uint32_t timenormalized)
{
	int xDist = 0;
	//find distance belt has moved thus far.
	if (animation->curve == AnimationCurveEaseInOut )
	{
		//add tweening
		//tweeen should return the position we want right now.
		xDist = tween((float) timenormalized, m_hr_total_movement.x, (float) ANIMATION_NORMALIZED_MAX);
	}
	else
	{
		//just move linearly.
		xDist = ((int16_t)(((float)(timenormalized * m_hr_total_movement.x))/ (float) ANIMATION_NORMALIZED_MAX));
	}

	for(int index = 0; index < current_HOUR_LAYER_COUNT; index++)
	{
		GRect rect = layer_get_frame(&m_hr_Layer[index].layer);
		int xStart = rect.origin.x;
		//subtract belt movement from original postion to find new position.
		rect.origin.x = m_hr_Layer[index].orig_point.x - xDist;
		if ( rect.origin.x < m_hr_Bounds.origin.x)
		{
			//if we're off the bounds of the belt...need to go to the end.
			rect.origin.x = m_hr_Bounds.size.w + rect.origin.x;
		}
	
		m_hr_Layer[index].newOrigin.x = rect.origin.x;
		if (rect.origin.x >= SCREEN_WIDTH || rect.origin.x < -1 * rect.size.w)
		{
			//if we're off the edge of the screen, lets not move 
			if (xStart < 0)
			{
				//we need to move this to be ready for the next move.
				rect.origin.x = SCREEN_WIDTH;
				layer_set_frame(&m_hr_Layer[index].layer, rect);
			}
		}
		else
		{
			layer_set_frame(&m_hr_Layer[index].layer, rect);
		}
	}
}

void animate_Hrs_teardown(Animation* animation)
{
	animate_Hrs(animation, ANIMATION_NORMALIZED_MAX);
	m_curHR = m_reqHR;
}

void update_Hr_Pos(int curHR)
{
	if (curHR >= current_HOUR_LAYER_COUNT)
	{
		curHR -= current_HOUR_LAYER_COUNT;
	}
	int hrDiff = curHR - m_reqHR;
	if (hrDiff < 0) hrDiff = 1; // we're going to assume that this is the 11 o'clock -> midnight transition...still only a 1 hr move.
	int xMoveDist = hrDiff;
	
	//we're going to assume there is at least 1 hour layer...find the width
	int framewidth = m_hr_Layer[0].layer.bounds.size.w;
	xMoveDist *= framewidth; // multiply by framewidth/num hrs per layer  should be (pixels/layer)/(hrs/layer) = pixels/hrs...will come out with # of pixels to move

	if (xMoveDist > 0 && !animation_is_scheduled(&prop_ani_hr))
	{
		if (!animation_is_scheduled(&prop_ani_hr))
		{
			for(int i=0; i<current_HOUR_LAYER_COUNT;i++)
			{
				//we're going to schedule the animation...lets set the requested hour now.
				m_hr_Layer[i].orig_point.x = m_hr_Layer[i].newOrigin.x;
				m_hr_Layer[i].orig_point.y = m_hr_Layer[i].newOrigin.y;
			}
			m_hr_total_movement = GPoint(xMoveDist, 0);
			animation_set_duration(&prop_ani_hr, hrDiff * hr_animation_duration / ((hrDiff>1)?2:1) ); //if hr diff > 1 then we're in catchup mode...should take 1/2 the time.
			animation_schedule(&prop_ani_hr);
		}
		else
		{
			m_hr_total_movement.x += xMoveDist;
		}
		m_reqHR = curHR;
		if (USE_VIBRATION && m_hr_init)
		{
			vibes_double_pulse();
		}
		else
		{
			m_hr_init = true;
		}
	}
}

void animate_Sec(Animation* animation, uint32_t timenormalized)
{
	int xDist = 0;
	//find distance belt has moved thus far.
	if (animation->curve == AnimationCurveEaseInOut )
	{
		//add tweening
		//tweeen should return the position we want right now.
		xDist = tween((float) timenormalized, m_sec_total_movement.x, (float) ANIMATION_NORMALIZED_MAX);
		
	}
	else
	{
		//just move linearly.
		xDist = ((int16_t)(((float)(timenormalized * m_sec_total_movement.x))/ (float) ANIMATION_NORMALIZED_MAX));
	}

	for(int index = 0; index < SEC_LAYER_COUNT; index++)
	{
		GRect rect = layer_get_frame(&m_sec_Layer[index].layer);
		int xStart = rect.origin.x;
		//subtract belt movement from original postion to find new position.
		rect.origin.x = m_sec_Layer[index].orig_point.x - xDist;
		if ( rect.origin.x < m_sec_Bounds.origin.x)
		{
			//if we're off the bounds of the belt...need to go to the end.
			rect.origin.x = m_sec_Bounds.size.w + rect.origin.x;
		}
	
		m_sec_Layer[index].newOrigin.x = rect.origin.x;
		if (rect.origin.x >= SCREEN_WIDTH || rect.origin.x < -1 * rect.size.w)
		{
			//if we're off the edge of the screen, lets not move 
			if (xStart < 0)
			{
				//we need to move this to be ready for the next move.
				rect.origin.x = SCREEN_WIDTH;
				layer_set_frame(&m_sec_Layer[index].layer, rect);
			}
		}
		else
		{
			layer_set_frame(&m_sec_Layer[index].layer, rect);
		}
	}
}

void animate_Sec_teardown(Animation* animation)
{
	animate_Sec(animation, ANIMATION_NORMALIZED_MAX);

	m_curSec = m_reqSec;
}

void update_Sec_Pos(int curSec)
{
	int secDiff = curSec - m_reqSec;
	if (secDiff < 0) secDiff = 1; // we're going to assume that this is the 11 o'clock -> midnight transition...still only a 1 sec move.
	int xMoveDist = secDiff;
	
	//we're going to assume there is at least 1 second layer...find the width
	int framewidth = m_sec_Layer[0].layer.bounds.size.w;
	xMoveDist *= framewidth /(60/SEC_LAYER_COUNT); // multiply by framewidth/num sec per layer  should be (pixels/layer)/(sec/layer) = pixels/sec...will come out with # of pixels to move

	if (xMoveDist > 0)
	{
		if (!animation_is_scheduled(&prop_ani))
		{
			for(int i=0; i<SEC_LAYER_COUNT;i++)
			{
				//we're going to schedule the animation...lets set the requested hour now.
				m_sec_Layer[i].orig_point.x = m_sec_Layer[i].newOrigin.x;
				m_sec_Layer[i].orig_point.y = m_sec_Layer[i].newOrigin.y;
			}
			m_sec_total_movement = GPoint(xMoveDist, 0);
			animation_set_duration(&prop_ani, secDiff * sec_animation_duration / ((secDiff>1)?2:1) ); //if sec diff > 1 then we're in catchup mode...should take 1/2 the time.
			animation_schedule(&prop_ani);
		}
		else
		{
			m_sec_total_movement.x += xMoveDist;
		}
		m_reqSec = curSec;
	}
}

void animate_Min_Bot(Animation* animation, uint32_t timenormalized)
{
	int xDist = 0, yDist = 0;
	//find distance belt has moved thus far.
	if (animation->curve == AnimationCurveEaseInOut )
	{
		//add tweening
		//tweeen should return the position we want right now.
		xDist = tween((float) timenormalized, m_min_bot_total_movement.x, (float) ANIMATION_NORMALIZED_MAX);
		yDist = tween((float) timenormalized, m_min_bot_total_movement.y, (float) ANIMATION_NORMALIZED_MAX);
		
	}
	else
	{
		//just move linearly.
		xDist = ((int16_t)(((float)(timenormalized * m_min_bot_total_movement.x))/ (float) ANIMATION_NORMALIZED_MAX));
		yDist = ((int16_t)(((float)(timenormalized * m_min_bot_total_movement.y))/ (float) ANIMATION_NORMALIZED_MAX));
	}
		
	for(int index = 0; index < MIN_BOTTOM_COUNT; index++)
	{
		GRect rect = layer_get_frame(&m_min_bottom_Layer[index].layer);

		//subtract belt movement from original postion to find new position.
		rect.origin.x = m_min_bottom_Layer[index].orig_point.x - xDist;
		rect.origin.y = m_min_bottom_Layer[index].orig_point.y + yDist;

		if ( rect.origin.x < m_min_bottom_Bounds.origin.x)
		{
			rect.origin.x = m_min_bottom_Bounds.size.w + rect.origin.x;
		}
		if ( rect.origin.y > (m_min_bottom_Bounds.origin.y + m_min_bottom_Bounds.size.h))
		{
			//if we're off the bounds of the belt...need to go to the end.
			rect.origin.y = rect.origin.y - m_min_bottom_Bounds.size.h;
		}
	
		m_min_bottom_Layer[index].newOrigin.x = rect.origin.x;
		m_min_bottom_Layer[index].newOrigin.y = rect.origin.y;
		{
			layer_set_frame(&m_min_bottom_Layer[index].layer, rect);
		}
	}
}


void animate_Min_Bot_teardown(Animation* animation)
{
	animate_Min_Bot(animation, ANIMATION_NORMALIZED_MAX);
	m_curMinBot = m_reqMinBot;
}


void animate_Min_Top(Animation* animation, uint32_t timenormalized)
{
	int xDist = 0, yDist = 0;
	//find distance belt has moved thus far.
	if (animation->curve == AnimationCurveEaseInOut )
	{
		//add tweening
		//tweeen should return the position we want right now.
		xDist = tween((float) timenormalized, m_min_top_total_movement.x, (float) ANIMATION_NORMALIZED_MAX);
		yDist = tween((float) timenormalized, m_min_top_total_movement.y, (float) ANIMATION_NORMALIZED_MAX);
		
	}
	else
	{
		//just move linearly.
		xDist = ((int16_t)(((float)(timenormalized * m_min_top_total_movement.x))/ (float) ANIMATION_NORMALIZED_MAX));
		yDist = ((int16_t)(((float)(timenormalized * m_min_top_total_movement.y))/ (float) ANIMATION_NORMALIZED_MAX));
	}
		
	for(int index = 0; index < MIN_TOP_COUNT; index++)
	{
		GRect rect = layer_get_frame(&m_min_top_Layer[index].layer);

		//subtract belt movement from original postion to find new position.
		rect.origin.x = m_min_top_Layer[index].orig_point.x - xDist;
		rect.origin.y = m_min_top_Layer[index].orig_point.y + yDist;

		if ( rect.origin.x < m_min_top_Bounds.origin.x)
		{
			rect.origin.x = m_min_top_Bounds.size.w + rect.origin.x;
		}
		if ( rect.origin.y > (m_min_top_Bounds.origin.y + m_min_top_Bounds.size.h))
		{
			//if we're off the bounds of the belt...need to go to the end.
			rect.origin.y = rect.origin.y - m_min_top_Bounds.size.h;
		}
	
		m_min_top_Layer[index].newOrigin.x = rect.origin.x;
		m_min_top_Layer[index].newOrigin.y = rect.origin.y;
		{
			layer_set_frame(&m_min_top_Layer[index].layer, rect);
		}
	}

	
}

void animate_Min_Top_teardown(Animation* animation)
{
	animate_Min_Top(animation, ANIMATION_NORMALIZED_MAX);
	m_curMinTop = m_reqMinTop;
}

void updateMinGen(BeltLayer bl[], int layerCount, int duration, Animation* anim, int diff, GPoint* movement)
{
	if (diff < 0) diff = 1; // we're going to assume that this is moving from 9 => 0.

	int xMoveDist = diff;
	int yMoveDist = diff;
	
	//we're going to assume there is at least 1 hour layer...find the width
	int framewidth = bl[0].layer.bounds.size.w;
	int frameheight = bl[0].layer.bounds.size.h;
	xMoveDist *= framewidth; 
	yMoveDist *= frameheight;

	if (xMoveDist > 0)
	{
		if (!animation_is_scheduled(anim))
		{
			for(int i=0; i<layerCount;i++)
			{
				//we're going to schedule the animation...lets set the requested hour now.
				bl[i].orig_point.x = bl[i].newOrigin.x;
				bl[i].orig_point.y = bl[i].newOrigin.y;
			}
			*movement = GPoint(xMoveDist, yMoveDist);
			animation_set_duration(anim, diff * duration / ((diff>1)?2:1) ); //if hr diff > 1 then we're in catchup mode...should take 1/2 the time.
			animation_schedule(anim);
		}
		else
		{
			for(int i=0; i<MIN_BOTTOM_COUNT;i++)
			{
				//we're already animating...just increase the total movement space.
				movement->x += xMoveDist;
				movement->y += yMoveDist;
			}
		}
	}
}


void update_Min_Pos(int curMin)
{
	int curMinBot = curMin%10;
	int curMinTop = curMin/10;
	
	if (curMinTop >=6)
	{
		curMinTop = 0;
	}

	updateMinGen(m_min_bottom_Layer, MIN_BOTTOM_COUNT, min_bottom_animation_duration, &prop_ani_bmin, curMinBot - m_reqMinBot, &m_min_bot_total_movement);
	m_reqMinBot = curMinBot;

	updateMinGen(m_min_top_Layer, MIN_TOP_COUNT, min_top_animation_duration, &prop_ani_tmin, curMinTop - m_reqMinTop, &m_min_top_total_movement);
	m_reqMinTop = curMinTop;
}

void handle_seconds_tick(AppContextRef ctx, PebbleTickEvent *t) {
	(void)t; //these need to be called so they are used somewhere in the method.
	(void)ctx;
	
	PblTm tm;
	get_time(&tm);

	//update_Min_Pos(tm.tm_sec);
	//update_Hr_Pos(m_curHR+1);	
	if (m_curSec != tm.tm_sec) update_Sec_Pos(tm.tm_sec);
 	if ((m_curMinBot + m_curMinTop*10) != tm.tm_min) update_Min_Pos(tm.tm_min);
	if (m_curHR != tm.tm_hour) 
	{
		update_Hr_Pos(tm.tm_hour);
	}
}

void beltLayer_init(BeltLayer *layer, GRect rect, char* text, GFont font, GColor forground, GTextAlignment ta)
{
	layer_init(&(layer->layer), rect);
	layer->newOrigin = GPoint(rect.origin.x, rect.origin.y);

	for(int i=0; i <= (int)strlen(text); i++)
	{	
		layer->text[i] = text[i];
	}
	layer->font = font;
	layer->foreground_color = forground;
	layer->layer.update_proc = (LayerUpdateProc) &beltlayer_update_callback;
	layer->text_alignment = ta;
	layer_add_child(&window.layer, &(layer->layer));
}

void updateStringFromNum(char* str, int num, bool pad)
{
	if (pad || num >= 10)
	{
		//2 digit number
		str[0] = '0' + ((num)/10);
		str[1] = '0' + ((num)%10);
	}
	else
	{
		str[0] = '0' + num;
		str[1] = 0;
	}
}

void InitHours()
{
	m_HR_Animation.update = &animate_Hrs;
	m_HR_Animation.teardown = &animate_Hrs_teardown;
	m_reqHR = m_curHR = 0; //default to midnight
	
	//hours band
	GRect rhrRect = GRect(0, 36, SCREEN_WIDTH, 48);
	layer_init(&m_hr_bg_Layer, rhrRect);
	m_hr_bg_Layer.update_proc = &hr_bg_display_layer_update_callback;
	layer_add_child(&window.layer, &m_hr_bg_Layer);

	m_hr_Bounds = GRect(HR_START_X - 30 * current_HOUR_LAYER_COUNT/2, 0, 30 * current_HOUR_LAYER_COUNT, 48);
	
	m_hr_init = false;
	for(int i = 0; i < current_HOUR_LAYER_COUNT; i++)
	{
		char * temp = "00";
		updateStringFromNum(temp, ((i == 0 && !clock_is_24h_style()) ? current_HOUR_LAYER_COUNT : i), false);
		GRect rect;
		int x = HR_START_X + i*30;
		if (x > (m_hr_Bounds.size.w + m_hr_Bounds.origin.x))
		{
			rect =  GRect(x - m_hr_Bounds.size.w , 42 +5, 30, 48);
		}
		else
		{
		 rect =  GRect(x, 42 +5, 30, 48);
		}
		beltLayer_init(&m_hr_Layer[i], rect, temp, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_KEY_HOURS_20)), FOREGROUND_COLOR, GTextAlignmentCenter);
		m_hr_total_movement = GPoint(0,0);
	}
	hr_animation_duration = 700;
	animation_init(&prop_ani_hr);
	animation_set_implementation(&prop_ani_hr, &m_HR_Animation);
	animation_set_curve(&prop_ani_hr, AnimationCurveEaseInOut);
}

void InitSeconds()
{
	m_Sec_Animation.update = &animate_Sec;
	m_Sec_Animation.teardown = &animate_Sec_teardown;
	m_reqSec = m_curSec = 0; //default to midnight

	//seconds bands
	GRect rsecRect = GRect(0, 132, SCREEN_WIDTH, 24);
	layer_init(&m_sec_bg_Layer, rsecRect);
	m_sec_bg_Layer.update_proc = &sec_bg_display_layer_update_callback;
	layer_add_child(&window.layer, &m_sec_bg_Layer);

	m_sec_Bounds = GRect(SEC_START_X - 30 * SEC_LAYER_COUNT/2, 132, 30 * SEC_LAYER_COUNT, 48);
	
	for(int i = 0; i < SEC_LAYER_COUNT; i++)
	{
		char *temp = "00";
		updateStringFromNum(temp, i*(60/SEC_LAYER_COUNT), true);
		int x = SEC_START_X + i*30;
		GRect rect;
		if (x >= (m_sec_Bounds.size.w + m_sec_Bounds.origin.x))
		{
			rect =  GRect(x - m_sec_Bounds.size.w , 135 +2, 30, 24);
		}
		else
		{
		 rect =  GRect(x, 135 +2, 30, 24);
		}

		
		beltLayer_init(&m_sec_Layer[i], rect, temp, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_KEY_SEC_10)), FOREGROUND_COLOR, GTextAlignmentCenter);
	}

	m_sec_total_movement = GPoint(0,0);

	sec_animation_duration = 200;
	animation_init(&prop_ani);
	animation_set_implementation(&prop_ani, &m_Sec_Animation);
	animation_set_curve(&prop_ani, AnimationCurveEaseInOut);
}

void InitMin()
{
	m_reqMinBot = m_curMinBot = m_reqMinTop = m_curMinTop = 0; //default to midnight

	//minute bands
	GRect rminRect = GRect(12, 0, 128, SCREEN_HEIGHT);
	layer_init(&m_min_bg_Layer, rminRect);
	m_min_bg_Layer.update_proc = &min_bg_display_layer_update_callback;
	layer_add_child(&window.layer, &m_min_bg_Layer);
		
	
	//bottom min
	m_MinBottom_Animation.update = &animate_Min_Bot;
	m_MinBottom_Animation.teardown = &animate_Min_Bot_teardown;

	m_min_bottom_Bounds = GRect(MIN_BOT_START_X - 17 * MIN_BOTTOM_COUNT/2, MIN_START_Y - 40 * MIN_BOTTOM_COUNT/2, 17 * MIN_BOTTOM_COUNT, 40 * MIN_BOTTOM_COUNT);

	GFont gfont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_KEY_MIN_12));
	
	int yOffset = 4;
	
	for(int i=0; i < MIN_BOTTOM_COUNT; i++)
	{
		char * temp = "0";
		updateStringFromNum(temp, i, false);

		int x = MIN_BOT_START_X + i * 17;
		int y = MIN_START_Y - i * 40 + yOffset;
		if (x >= (m_min_bottom_Bounds.size.w + m_min_bottom_Bounds.origin.x))
		{
			x = x - m_min_bottom_Bounds.size.w;
		}
		if (y <= m_min_bottom_Bounds.origin.y)
		{
			y = y + m_min_bottom_Bounds.size.h;
		}
		GRect rect = GRect(x, y, 17, 40);;

		beltLayer_init(&m_min_bottom_Layer[i], rect, temp, gfont, FOREGROUND_COLOR, GTextAlignmentLeft);
	}

	m_min_bot_total_movement = GPoint(0,0);
	min_bottom_animation_duration = 500;
	animation_init(&prop_ani_bmin);
	animation_set_implementation(&prop_ani_bmin, &m_MinBottom_Animation);
	animation_set_curve(&prop_ani_bmin, AnimationCurveEaseInOut);

	//top mins
	m_MinTop_Animation.update = &animate_Min_Top;
	m_MinTop_Animation.teardown = &animate_Min_Top_teardown;

	m_min_top_Bounds = GRect(MIN_TOP_START_X - 30 * MIN_TOP_COUNT/2, MIN_START_Y - 70 * MIN_TOP_COUNT/2, 30 * MIN_TOP_COUNT, 70 * MIN_TOP_COUNT);

	for(int i=0; i < MIN_TOP_COUNT; i++)
	{
		char * temp = "0";
		updateStringFromNum(temp, i, false);

		int x = MIN_TOP_START_X + i * 30;
		int y = MIN_START_Y - i * 70 + yOffset;
		GRect rect;
		if (x >= (m_min_top_Bounds.size.w + m_min_top_Bounds.origin.x))
		{
			rect =  GRect(x - m_min_top_Bounds.size.w , y + m_min_top_Bounds.size.h, 30, 70);
		}
		else
		{
		 rect =  GRect(x, y, 30, 70);
		}

		beltLayer_init(&m_min_top_Layer[i], rect, temp, gfont, FOREGROUND_COLOR, GTextAlignmentLeft);
	}

	m_min_top_total_movement = GPoint(0,0);
	min_top_animation_duration = 900;
	animation_init(&prop_ani_tmin);
	animation_set_implementation(&prop_ani_tmin, &m_MinTop_Animation);
	animation_set_curve(&prop_ani_tmin, AnimationCurveEaseInOut);
}

void drawOverlay(Layer *me, GContext* ctx)
{
	graphics_context_set_fill_color(ctx, FOREGROUND_COLOR);
	graphics_context_set_stroke_color(ctx, FOREGROUND_COLOR);

	//Draw Hours overlay
	for (int i = 0; i < 3; i++)
	{
		GRect hoursOver = GRect(HR_START_X-i +2, 36-i, 30+(i*2), 48+(i*2));
		graphics_draw_round_rect(ctx, hoursOver, 0);

		GRect secOver = GRect(SEC_START_X -i, 132-i, 31+(i*2), 24+(i*2));
		graphics_draw_round_rect(ctx, secOver, 0);

		GRect minOver = GRect(MIN_TOP_START_X-6 -i, MIN_START_Y-4 -i, 42+(i*2), 30+(i*2));
		graphics_draw_round_rect(ctx, minOver, 0);
	}
	
	//draw Hours Arrows
	static GPath topHRarrowPath;
	static GPoint arrow_points1[] = {
    { HR_START_X + 15, 44 },
    { HR_START_X + 8, 36 },
    { HR_START_X + 22, 36 },
    { HR_START_X + 15, 44 }
	};
	GPATH_INIT(&topHRarrowPath, arrow_points1);
    gpath_draw_filled(ctx, &topHRarrowPath);

	static GPath botHRarrowPath;
	static GPoint arrow_points2[] = {
    { HR_START_X + 16, 75 },
    { HR_START_X + 9, 84 },
    { HR_START_X + 23, 84 },
    { HR_START_X + 16, 75 }
	};
	GPATH_INIT(&botHRarrowPath, arrow_points2);
    gpath_draw_filled(ctx, &botHRarrowPath);
	
	//Draw min arrows
	static GPath topMinarrowPath;
	static GPoint arrow_points3[] = {
    { MIN_TOP_START_X + 3, MIN_START_Y +2 },
    { MIN_TOP_START_X -1, MIN_START_Y -4},
    { MIN_TOP_START_X + 9, MIN_START_Y -4},
    { MIN_TOP_START_X + 3,  MIN_START_Y +2}
	};
	GPATH_INIT(&topMinarrowPath, arrow_points3);
    gpath_draw_filled(ctx, &topMinarrowPath);

	static GPath botMinarrowPath;
	static GPoint arrow_points4[] = {
    { MIN_TOP_START_X + 3, MIN_START_Y +20 },
    { MIN_TOP_START_X -1, MIN_START_Y +26},
    { MIN_TOP_START_X + 9, MIN_START_Y +26},
    { MIN_TOP_START_X + 3,  MIN_START_Y +20}
	};
	GPATH_INIT(&botMinarrowPath, arrow_points4);
    gpath_draw_filled(ctx, &botMinarrowPath);

	static GPath topMinarrowPath2;
	static GPoint arrow_points5[] = {
    { MIN_BOT_START_X + 3, MIN_START_Y +2 },
    { MIN_BOT_START_X -1, MIN_START_Y -4},
    { MIN_BOT_START_X + 9, MIN_START_Y -4},
    { MIN_BOT_START_X + 3,  MIN_START_Y +2}
	};
	GPATH_INIT(&topMinarrowPath2, arrow_points5);
    gpath_draw_filled(ctx, &topMinarrowPath2);

	static GPath botMinarrowPath2;
	static GPoint arrow_points6[] = {
    { MIN_BOT_START_X + 3, MIN_START_Y +20 },
    { MIN_BOT_START_X -1, MIN_START_Y +26},
    { MIN_BOT_START_X + 9, MIN_START_Y +26},
    { MIN_BOT_START_X + 3,  MIN_START_Y +20}
	};
	GPATH_INIT(&botMinarrowPath2, arrow_points6);
    gpath_draw_filled(ctx, &botMinarrowPath2);

	//draw seconds overlay
	static GPath topSecArrowPath;
	static GPoint arrow_points7[] = {
    { SEC_START_X+15, 137 },
    { SEC_START_X+10, 132},
    { SEC_START_X+20, 132},
    { SEC_START_X+15,  137}
	};
	GPATH_INIT(&topSecArrowPath, arrow_points7);
    gpath_draw_filled(ctx, &topSecArrowPath);

	static GPath botSecArrowPath;
	static GPoint arrow_points8[] = {
    { SEC_START_X+15, 151 },
    { SEC_START_X+10, 156},
    { SEC_START_X+20, 156},
    { SEC_START_X+15,  151}
	};
	GPATH_INIT(&botSecArrowPath, arrow_points8);
    gpath_draw_filled(ctx, &botSecArrowPath);
}

void handle_init(AppContextRef ctx) {
	(void)ctx;
	
	current_HOUR_LAYER_COUNT = clock_is_24h_style() ? 24 : 12;

	window_init(&window, "Belt Drivin");
	window_stack_push(&window, true /* Animated */);
	window_set_background_color(&window, BACKGROUND_COLOR);	

	resource_init_current_app(&APP_RESOURCES);

	InitHours();
	
	InitMin();
		
	InitSeconds();

	//add overlay
	GRect overlaysize = GRect(0, 0 , SCREEN_WIDTH, SCREEN_HEIGHT);
	layer_init(&m_overlay, overlaysize);
	m_overlay.update_proc = &drawOverlay;
	layer_add_child(&window.layer, &m_overlay);
}

void handle_deinit(AppContextRef ctx)
{
	(void)ctx;
}

void pbl_main(void *params) {
	PebbleAppHandlers handlers = {
		.init_handler = &handle_init,
		.deinit_handler = &handle_deinit,
		.tick_info = {
			.tick_handler = &handle_seconds_tick,
			.tick_units = SECOND_UNIT
		}
	};
	app_event_loop(params, &handlers);
}
