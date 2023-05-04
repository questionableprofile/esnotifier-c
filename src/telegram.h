#ifndef TELEGRAM_H_HEADER
#define TELEGRAM_H_HEADER

#include <telebot.h>
#include "main.h"
#include "util.h"
#include "eso.h"

#define TG_STATE_RUN 1
#define TG_STATE_PAUSE 2
#define TG_STATE_INACTIVE 3

typedef struct tg_bot_params {
    long long int owner;
	bool chat_mod_enabled;
} tg_bot_params_t;

typedef struct tg_context {
    global_ctx_t* global_ctx;
	char* token;
	telebot_handler_t handle;
	telebot_user_t bot;
	pthread_t worker;
	volatile int state;
	tg_bot_params_t* bot_params;
} tg_context_t;

void process_update (tg_context_t* ctx, telebot_update_t* update);

telebot_error_e tg_init_context (global_ctx_t* global_ctx, tg_context_t* ctx, const char* token);
telebot_error_e tg_free_context (tg_context_t* ctx);
tg_bot_params_t* init_tg_bot_params();
void tg_free_bot_params (tg_bot_params_t* params);

pthread_t tg_run_worker (tg_context_t* ctx);
void tg_pause_worker (tg_context_t* ctx);

void tg_send_message (tg_context_t* ctx, long long int recipient, const char* text, bool silent);
void tg_send_owner (tg_context_t* ctx, const char* text, bool silent);
void print_bot_info (const tg_context_t* ctx);
string_builder_t* tg_format_eso_event (eso_event_t* event);

event_handler_t* tg_event_handler_create ();
void tg_event_handler_free (event_handler_t* handler);

#endif