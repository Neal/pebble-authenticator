#include <pebble.h>
#include "configuration.h"
#include "sha1.h"

static Window *window;
static TextLayer *label_layer;
static TextLayer *token_layer;
static TextLayer *ticker_layer;

static int current_token;
static bool current_token_changed;
static float timezone = DEFAULT_TIME_ZONE;

enum {
	KEY_TIMEZONE,
	KEY_VIB_WARN,
	KEY_VIB_RENEW,
	KEY_CURRENT_TOKEN
};

float stof(const char* s) {
	float rez = 0, fact = 1;
	if (*s == '-') {
		s++;
		fact = -1;
	}
	for (int point_seen = 0; *s; s++) {
		if (*s == '.') {
			point_seen = 1; 
			continue;
		}
		int d = *s - '0';
		if (d >= 0 && d <= 9) {
			if (point_seen) fact /= 10.0f;
			rez = rez * 10.0f + (float)d;
		}
	}
	return rez * fact;
}

void set_timezone() {
	if (persist_exists(KEY_TIMEZONE)) {
		char tz[7];
		persist_read_string(KEY_TIMEZONE, tz, sizeof(tz));
		timezone = stof(tz);
	}
}

static void in_received_handler(DictionaryIterator *iter, void *context) {
	Tuple *timezone_tuple = dict_find(iter, KEY_TIMEZONE);
	Tuple *vib_warn_tuple = dict_find(iter, KEY_VIB_WARN);
	Tuple *vib_renew_tuple = dict_find(iter, KEY_VIB_RENEW);

	if (timezone_tuple) {
		persist_write_string(KEY_TIMEZONE, timezone_tuple->value->cstring);
		set_timezone();
	}
	if (vib_warn_tuple) {
		persist_write_bool(KEY_VIB_WARN, vib_warn_tuple->value->uint8);
	}
	if (vib_renew_tuple) {
		persist_write_bool(KEY_VIB_RENEW, vib_renew_tuple->value->uint8);
	}
}

void in_dropped_handler(AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "incoming message from Pebble dropped");
}

void vibration_handler(int current_seconds) {
	if (current_seconds == 5 && persist_exists(KEY_VIB_WARN) && persist_read_bool(KEY_VIB_WARN)) {
		vibes_double_pulse();
	}
	if (current_seconds == 30 && persist_exists(KEY_VIB_RENEW) && persist_read_bool(KEY_VIB_RENEW)) {
		vibes_short_pulse();
	}
}

uint32_t get_token() {
	sha1nfo s;
	uint8_t ofs;
	uint32_t otp;
	char sha1_time[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

	// TOTP uses seconds since epoch in the upper half of an 8 byte payload
	// TOTP is HOTP with a time based payload
	// HOTP is HMAC with a truncation function to get a short decimal key
	uint32_t unix_time = time(NULL);
	int adjustment = 3600 * timezone;
	unix_time = unix_time - adjustment;
	unix_time /= 30;

	sha1_time[4] = (unix_time >> 24) & 0xFF;
	sha1_time[5] = (unix_time >> 16) & 0xFF;
	sha1_time[6] = (unix_time >> 8) & 0xFF;
	sha1_time[7] = unix_time & 0xFF;

	// First get the HMAC hash of the time payload with the shared key
	sha1_initHmac(&s, otp_keys[current_token], otp_sizes[current_token]);
	sha1_write(&s, sha1_time, 8);
	sha1_resultHmac(&s);

	// Then do the HOTP truncation. HOTP pulls its result from a 31-bit byte
	// aligned window in the HMAC result, then lops off digits to the left
	// over 6 digits.
	ofs = s.state.b[HASH_LENGTH-1] & 0xf;
	otp = 0;
	otp = ((s.state.b[ofs] & 0x7f) << 24) |
		((s.state.b[ofs + 1] & 0xff) << 16) |
		((s.state.b[ofs + 2] & 0xff) << 8) |
		(s.state.b[ofs + 3] & 0xff);
	otp %= 1000000;

	return otp;
}

void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
	int current_seconds = 30 - (tick_time->tm_sec % 30);

	vibration_handler(current_seconds);

	if (current_token_changed || current_seconds == 30) {
		current_token_changed = false;

		static char token_text[] = "000000";
		snprintf(token_text, sizeof(token_text), "%06lu", get_token());

		text_layer_set_text(label_layer, otp_labels[current_token]);
		text_layer_set_text(token_layer, token_text);
	}

	static char ticker_text[] = "00";
	snprintf(ticker_text, sizeof(ticker_text), "%d", current_seconds);
	text_layer_set_text(ticker_layer, ticker_text);
}

static void click_handler(ClickRecognizerRef recognizer, Window *window) {
	switch ((int)click_recognizer_get_button_id(recognizer)) {
		case BUTTON_ID_UP:
			current_token = (current_token - 1 + NUM_SECRETS) % NUM_SECRETS;
			current_token_changed = true;
			break;
		case BUTTON_ID_DOWN:
			current_token = (current_token + 1) % NUM_SECRETS;
			current_token_changed = true;
			break;
	}
	time_t now = time(NULL);
	handle_second_tick(gmtime(&now), SECOND_UNIT);
}

static void click_config_provider(void *context) {
	const uint16_t repeat_interval_ms = 100;
	window_single_repeating_click_subscribe(BUTTON_ID_UP, repeat_interval_ms, (ClickHandler) click_handler);
	window_single_repeating_click_subscribe(BUTTON_ID_DOWN, repeat_interval_ms, (ClickHandler) click_handler);
}

static void window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);

	label_layer = text_layer_create((GRect) { .origin = { 0, 20 }, .size = bounds.size });
	text_layer_set_text_color(label_layer, GColorWhite);
	text_layer_set_background_color(label_layer, GColorClear);
	text_layer_set_font(label_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_text_alignment(label_layer, GTextAlignmentCenter);

	token_layer = text_layer_create((GRect) { .origin = { 0, 60 }, .size = bounds.size });
	text_layer_set_text_color(token_layer, GColorWhite);
	text_layer_set_background_color(token_layer, GColorClear);
	text_layer_set_font(token_layer, fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS));
	text_layer_set_text_alignment(token_layer, GTextAlignmentCenter);

	ticker_layer = text_layer_create((GRect) { .origin = { 0, 120 }, .size = bounds.size });
	text_layer_set_text_color(ticker_layer, GColorWhite);
	text_layer_set_background_color(ticker_layer, GColorClear);
	text_layer_set_font(ticker_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	text_layer_set_text_alignment(ticker_layer, GTextAlignmentCenter);

	layer_add_child(window_layer, text_layer_get_layer(label_layer));
	layer_add_child(window_layer, text_layer_get_layer(token_layer));
	layer_add_child(window_layer, text_layer_get_layer(ticker_layer));

	tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick);
}

static void window_unload(Window *window) {
	tick_timer_service_unsubscribe();
	text_layer_destroy(label_layer);
	text_layer_destroy(token_layer);
	text_layer_destroy(ticker_layer);
}

static void app_message_init(void) {
	app_message_open(64 /* inbound_size */, 0 /* outbound_size */);
	app_message_register_inbox_received(in_received_handler);
	app_message_register_inbox_dropped(in_dropped_handler);
}

static void init(void) {
	app_message_init();

	current_token = persist_exists(KEY_CURRENT_TOKEN) ? persist_read_int(KEY_CURRENT_TOKEN) : 0;
	current_token_changed = true;

	window = window_create();
	window_set_click_config_provider(window, click_config_provider);
	window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload,
	});
	window_stack_push(window, true /* animated */);
	window_set_background_color(window, GColorBlack);

	set_timezone();

	time_t now = time(NULL);
	handle_second_tick(gmtime(&now), SECOND_UNIT);
}

static void deinit(void) {
	persist_write_int(KEY_CURRENT_TOKEN, current_token);
	window_destroy(window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}
