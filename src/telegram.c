#include <pthread.h>
#include <stdlib.h>

#include "util.h"
#include "telegram.h"
#include "config.h"

telebot_error_e tg_init_context (global_ctx_t* global_ctx, tg_context_t* ctx, const char* token) {
    ctx->global_ctx = global_ctx;
    ctx->token = strdup(token);
    ctx->worker = NULL;
    ctx->state = TG_STATE_INACTIVE;
    ctx->bot_params = init_tg_bot_params();

    int result;
    if ((result = telebot_create(&ctx->handle, ctx->token)) != TELEBOT_ERROR_NONE) {
        printf("error creating telebot handle %d\n", result);
        free(ctx->token);
        tg_free_bot_params(ctx->bot_params);
        return result;
    }

    if ((result = telebot_get_me(ctx->handle, &ctx->bot)) != TELEBOT_ERROR_NONE) {
        printf("error getting bot information\n");
        free(ctx->token);
        tg_free_bot_params(ctx->bot_params);
        return result;
    }

    const char* owner_str = config_get_value(global_ctx->config, "owner");
    if (owner_str == NULL)
        ctx->bot_params->owner = 0;
    else {
        ctx->bot_params->owner = 1;
        ctx->bot_params->owner = strtoll(owner_str, NULL, 10);
    }

    print_bot_info(ctx);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e tg_free_context (tg_context_t* ctx) {
    REQ_NON_NULL(ctx, TELEBOT_ERROR_INVALID_PARAMETER);
    telebot_put_me(&(ctx->bot));
    telebot_destroy(ctx->handle);
    tg_free_bot_params(ctx->bot_params);

    return TELEBOT_ERROR_NONE;
}

tg_bot_params_t* init_tg_bot_params () {
    tg_bot_params_t* params = MALLOC_STRUCT(tg_bot_params_t);
    params->chat_mod_enabled = true;
    params->owner = 0;
    // @TODO check malloc result
    return params;
}

void tg_free_bot_params (tg_bot_params_t* params) {
    free(params);
}

void* run (void* _ctx) {
    tg_context_t* ctx = (tg_context_t*)_ctx;
    telebot_update_type_e allowed_updates[] = {TELEBOT_UPDATE_TYPE_MESSAGE};
    telebot_handler_t handle = ctx->handle;

    int offset = 0, count;

    while (ctx->state == TG_STATE_RUN) {
        telebot_update_t* updates;

        telebot_error_e poll_error =
                telebot_get_updates(handle, offset, 20, 5, allowed_updates, 0, &updates, &count);

        if (poll_error != TELEBOT_ERROR_NONE)
            continue;

        for (int i = 0; i < count; i++) {
            telebot_update_t update = updates[i];
            offset = update.update_id + 1;
            process_update(ctx, &update);
        }
        telebot_put_updates(updates, count);
        sleep(1);
    }
    return NULL;
}

void process_update (tg_context_t* ctx, telebot_update_t* update) {
    telebot_message_t message = update->message;

    char* text = strdup(message.text);
//    printf("tg text: %s\n", text);
    eso_command_t* cmd = NULL;
    ref_counter_t* ref = ref_counter_create();
    ref_counter_add(ref, text);

    if (STREQUAL(text, "/start")) {
        if (ctx->bot_params->owner == 0) {
            ctx->bot_params->owner = message.from->id;
            string_builder_t* temp = string_builder_printf("%ld", ctx->bot_params->owner);
            config_set_value(ctx->global_ctx->config, "owner", temp->value);
            config_rewrite(ctx->global_ctx->config);
            tg_send_owner(ctx, "Готово! Вы сохранены как владелец бота.", false);
            string_builder_free(temp);
        }
        else if (ctx->bot_params->owner != message.from->id)
            printf("account %s \"%s %s\" tried to access bot\n", message.from->username, message.from->first_name, message.from->last_name);
        else
            tg_send_owner(ctx, "Вы уже владеете этим ботом.", false);
    }
    else if (STREQUAL(text, "/reconnect")) {
        cmd = eso_command_create(ESO_CMD_RECONNECT, NULL, ref);
        tg_send_owner(ctx, "Reconnecting", false);
    }
    else if (STREQUAL(text, "/chat")) {
        if (ctx->bot_params->chat_mod_enabled) {
            ctx->bot_params->chat_mod_enabled = false;
            tg_send_owner(ctx, "Chat mode disabled", false);
        }
        else {
            ctx->bot_params->chat_mod_enabled = true;
            tg_send_owner(ctx, "Chat mode enabled", false);
        }
    }
    else if (ctx->bot_params->chat_mod_enabled) {
        eso_send_message_command_t* send_message_data = MALLOC_STRUCT(eso_send_message_command_t);
        send_message_data->text = text;
        ref_counter_add(ref, send_message_data);
        cmd = eso_command_create(ESO_CMD_SEND_MESSAGE, send_message_data, ref);
    }

    if (cmd != NULL)
        command_queue_add(ctx->global_ctx->command_queue, cmd);
    else
        ref_counter_free(ref);
}

void tg_pause_worker (tg_context_t* ctx) {
    ctx->state = TG_STATE_PAUSE;
}

pthread_t* tg_run_worker (tg_context_t* ctx) {
    pthread_t* thread = calloc(1, sizeof(pthread_t));
    ctx->state = TG_STATE_RUN;
    int result = pthread_create(thread, NULL, &run, (void*)ctx);
    if (result != 0)
        return NULL;

    ctx->worker = thread;

    return thread;
}

void tg_send_message (tg_context_t* ctx, long long int recipient, const char* text, bool silent) {
    telebot_send_message(ctx->handle, recipient, text, "HTML", true, silent, 0, "");
}

void tg_send_owner (tg_context_t* ctx, const char* text, bool silent) {
    tg_send_message(ctx, ctx->bot_params->owner, text, silent);
}

void print_bot_info (const tg_context_t* ctx) {
    const telebot_user_t* bot_info = &ctx->bot;
    printf("bot: %d @%s ", bot_info->id, bot_info->username);
    if (bot_info->first_name)
        printf("%s ", bot_info->first_name);
    if (bot_info->last_name)
        printf("%s", bot_info->last_name);
    printf("\n");
}

void tg_handle_event (global_ctx_t* ctx, eso_event_t* eso_event) {
    string_builder_t* formatted = tg_format_eso_event(eso_event);
    printf("%s\n", string_builder_as_cstring(formatted));
    tg_send_owner(ctx->tg_ctx, string_builder_as_cstring(formatted), true);
    string_builder_free(formatted);
}

event_handler_t* tg_event_handler_create () {
    event_handler_t* event_handler = MALLOC_STRUCT(event_handler_t);
    event_handler->event = &tg_handle_event;
    return event_handler;
}

void tg_event_handler_free (event_handler_t* handler) {
    free(handler);
}

string_builder_t* tg_format_eso_event (eso_event_t* event) {
    string_builder_t *result = string_builder_printf("[%s] ", event->game_data.node);

    string_builder_t* str;
    switch(event->event_type) {
        case ESO_EVENT_CHAT: {
            eso_event_chat_t *chat = (eso_event_chat_t *) event->data;
            str = string_builder_printf("[%d] %s: %s", event->actor.id, event->actor.name, chat->message);
            break;
        }
        case ESO_EVENT_BROADCAST: {
            eso_event_broadcast_t *broadcast = (eso_event_broadcast_t *) event->data;
            str = string_builder_printf("Блютекст: %s", broadcast->message);
            break;
        }
        case ESO_EVENT_TRY: {
            eso_event_try_t *try = (eso_event_try_t *) event->data;
            str = string_builder_printf("[%d] [try] %s: %s %s",
                                        event->actor.id, event->actor.name, try->success ? "успешно" : "безуспешно",
                                        try->message);
            break;
        }
        case ESO_EVENT_ROLL: {
            eso_event_roll_t *roll = (eso_event_roll_t *) event->data;
            str = string_builder_printf("[%d] [roll] %s rolled %d", event->actor.id, event->actor.name, roll->num);
            break;
        }
        case ESO_EVENT_DICE: {
            eso_event_dice_t* dice_data = (eso_event_dice_t*)event->data;
            string_builder_t* dice_string = string_builder_create(64);
            for (size_t i = 0; i < list_size(dice_data->list); i++) {
                eso_dice_t* dice = list_get(dice_data->list, i, eso_dice_t*);
                string_builder_t* s = string_builder_printf("%d/%d ", dice->num, dice->sides);
                string_builder_append(dice_string, string_builder_as_cstring(s));
                string_builder_free(s);
            }
            str = string_builder_printf("[%d] [dice] %s rolled %s", event->actor.id, event->actor.name,
                                                               string_builder_as_cstring(dice_string));
            string_builder_free(dice_string);
            break;
        }
        case ESO_EVENT_MEDIA_TRACK: {
            eso_media_track_t* media_event = (eso_media_track_t*) event->data;
            string_builder_t * video_link = string_builder_create(128);
            char* type = media_event->type;
            char* id = media_event->id;
            if (STREQUAL(type, "youtube")) {
                string_builder_append(video_link, "https://www.youtube.com/watch?v=");
                string_builder_append(video_link, id);
            }
            else
                string_builder_append(video_link, id);
            str = string_builder_printf("[%d] [yt-play] %s played %s", event->actor.id, event->actor.name,
                                        string_builder_as_cstring(video_link));
            string_builder_free(video_link);
            break;
        }
        case ESO_EVENT_DISCONNECT: {
            str = string_builder_copy("DISCONNECT");
            break;
        }
        default: {
            str = string_builder_printf("unimplemented event \'%s\'", event->event_code);
        }
    }
    const char* s = string_builder_as_cstring(str);
    string_builder_append(result, s);
    string_builder_free(str);
    return result;
}