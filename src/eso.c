#include <json.h>
#include "eso.h"

eso_command_t* eso_command_create (int command_type, void* data, ref_counter_t* ref_counter) {
    eso_command_t* command = MALLOC_STRUCT(eso_command_t);
    command->type = command_type;
    command->data = data;
    command->ref = ref_counter;
    return command;
}

void eso_command_free (eso_command_t* command) {
    if (command->ref != NULL)
        ref_counter_free(command->ref);
    free(command);
}

void eso_command_list_free (list_t* command_list) {
    for (size_t i = 0; i < list_size(command_list); i++)
        eso_command_free(list_get(command_list, i, eso_command_t*));
}

string_builder_t* eso_command_list_to_json (list_t* command_list) {
    json_object* list_obj = json_object_new_array();
    for (size_t i = 0; i < list_size(command_list); i++) {
        json_object* cmd_obj = json_object_new_object();
        eso_command_t* cmd = list_get(command_list, i, eso_command_t*);
        if (cmd->type == ESO_CMD_SEND_MESSAGE) {
            eso_send_message_command_t* msg = (eso_send_message_command_t*) cmd->data;
            json_object_object_add(cmd_obj, "type", json_object_new_string(ESO_CMD_SEND_MESSAGE_NAME));
            json_object* msg_data = json_object_new_object();
            json_object_object_add(msg_data, "text", json_object_new_string(msg->text));
            json_object_object_add(cmd_obj, "data", msg_data);
        }
        else if (cmd->type == ESO_CMD_RECONNECT)
            json_object_object_add(cmd_obj, "type", json_object_new_string(ESO_CMD_RECONNECT_NAME));
        else
            json_object_put(cmd_obj);
        json_object_array_add(list_obj, cmd_obj);
    }
    json_object* root = json_object_new_object();
    json_object_object_add(root, "commands", list_obj);
    const char* export = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PLAIN);
    string_builder_t* result = string_builder_copy(export);
    json_object_put(root);
    return result;
}


eso_event_t* eso_event_create () {
    eso_event_t* event = MALLOC_STRUCT(eso_event_t);
    event->event_code = NULL;
    event->event_type = ESO_EVENT_UNEXPECTED;
    event->data = NULL;
    event->rc = ref_counter_create();
    return event;
}

void eso_event_free (eso_event_t* event) {
    ref_counter_free(event->rc);
    free(event);
}

int match_event_name_to_constant (const char* name) {
    if (STREQUAL(name, ESO_EVENT_NAME_CHAT))
        return ESO_EVENT_CHAT;
    else if (STREQUAL(name, ESO_EVENT_NAME_BROADCAST))
        return ESO_EVENT_BROADCAST;
    else if (STREQUAL(name, ESO_EVENT_NAME_TRY))
        return ESO_EVENT_TRY;
    else if (STREQUAL(name, ESO_EVENT_NAME_ROLL))
        return ESO_EVENT_ROLL;
    else if (STREQUAL(name, ESO_EVENT_NAME_DICE))
        return ESO_EVENT_DICE;
    else if (STREQUAL(name, ESO_EVENT_NAME_MEDIA_TRACK))
        return ESO_EVENT_MEDIA_TRACK;
    else if (STREQUAL(name, ESO_EVENT_NAME_DISCONNECT))
        return ESO_EVENT_DISCONNECT;
    else
        return ESO_EVENT_UNEXPECTED;
}

void* eso_parse_chat_event (eso_event_t* event, json_object* event_data) {
    json_object* message_obj = json_object_object_get(event_data, "message");
    eso_event_chat_t* chat = MALLOC_STRUCT(eso_event_chat_t);
    chat->message = strdup(json_object_get_string(message_obj));
    ref_counter_add(event->rc, chat);
    ref_counter_add(event->rc, chat->message);
    return (void*)chat;
}

void* eso_parse_try_event (eso_event_t* event, json_object* event_data) {
    json_object* message_obj = json_object_object_get(event_data, "message");
    json_object* success_obj = json_object_object_get(event_data, "success");
    eso_event_try_t* try = MALLOC_STRUCT(eso_event_try_t);
    try->message = strdup(json_object_get_string(message_obj));
    try->success = json_object_get_boolean(success_obj);
    ref_counter_add(event->rc, try);
    ref_counter_add(event->rc, try->message);
    return (void*)try;
}

void* eso_parse_broadcast_event (eso_event_t* event, json_object* event_data) {
    json_object* message_obj = json_object_object_get(event_data, "message");
    eso_event_broadcast_t* broadcast = MALLOC_STRUCT(eso_event_broadcast_t);
    broadcast->message = strdup(json_object_get_string(message_obj));
    ref_counter_add(event->rc, broadcast);
    ref_counter_add(event->rc, broadcast->message);
    return (void*)broadcast;
}

void* eso_parse_media_track_event (eso_event_t* event, json_object* event_data) {
    json_object* track_obj = json_object_object_get(event_data, "track");
    json_object* id_obj = json_object_object_get(track_obj, "id");
    json_object* type_obj = json_object_object_get(track_obj, "type");
    eso_media_track_t* track = MALLOC_STRUCT(eso_media_track_t);
    track->id = strdup(json_object_get_string(id_obj));
    track->type = strdup(json_object_get_string(type_obj));
    ref_counter_add(event->rc, track);
    ref_counter_add(event->rc, track->type);
    ref_counter_add(event->rc, track->id);
    return (void*)track;
}

void* eso_parse_roll_event (eso_event_t* event, json_object* event_data) {
    json_object* message_obj = json_object_object_get(event_data, "num");
    eso_event_roll_t* roll = MALLOC_STRUCT(eso_event_roll_t);
    roll->num = json_object_get_int(message_obj);
    ref_counter_add(event->rc, roll);
    return (void*)roll;
}

void* eso_parse_dice_event (eso_event_t* event, json_object* event_data) {
    json_object* dices_obj = json_object_object_get(event_data, "rolls");
    eso_event_dice_t* dices = MALLOC_STRUCT(eso_event_dice_t);
    size_t length = json_object_array_length(dices_obj);
    void* memory = malloc(sizeof(eso_dice_t) * length);
    list_t* dice_list = list_create_sized(eso_dice_t*, length);
    for (size_t i = 0; i < length; i++) {
        json_object* dice_obj = json_object_array_get_idx(dices_obj, i);
        json_object* num_obj = json_object_object_get(dice_obj, "num");
        json_object* sides_obj = json_object_object_get(dice_obj, "sides");

        eso_dice_t* dice = memory + i * sizeof(eso_dice_t);
        dice->num = json_object_get_int(num_obj);
        dice->sides = json_object_get_int(sides_obj);
        list_push(dice_list, dice);
    }
    dices->list = dice_list;

    ref_counter_add(event->rc, dices);
    ref_counter_add(event->rc, dice_list->values);
    ref_counter_add(event->rc, dice_list);
    ref_counter_add(event->rc, memory);
    return (void*)dices;
}

void* parse_event_data (eso_event_t* event, json_object* event_data) {
    switch (event->event_type) {
        case ESO_EVENT_CHAT:
            return eso_parse_chat_event(event, event_data);
        case ESO_EVENT_TRY:
            return eso_parse_try_event(event, event_data);
        case ESO_EVENT_BROADCAST:
            return eso_parse_broadcast_event(event, event_data);
        case ESO_EVENT_MEDIA_TRACK:
            return eso_parse_media_track_event(event, event_data);
        case ESO_EVENT_ROLL:
            return eso_parse_roll_event(event, event_data);
        case ESO_EVENT_DICE:
            return eso_parse_dice_event(event, event_data);
        case ESO_EVENT_DISCONNECT:
            return NULL;
    }
    return NULL;
}

eso_event_t* parse_eso_event (char* text) {
    json_object *root = json_tokener_parse(text);
    if (!root)
        return NULL;

    eso_event_t* event = eso_event_create();

    json_object* code_obj = json_object_object_get(root, "code");
    json_object* data_obj = json_object_object_get(root, "data");
    json_object* game_data_obj = json_object_object_get(data_obj, "gameData");
    json_object* node_code_obj = json_object_object_get(game_data_obj, "node");
    json_object* actor_obj = json_object_object_get(data_obj, "actor");
    json_object* actor_id_obj = json_object_object_get(actor_obj, "id");
    json_object* actor_name_obj = json_object_object_get(actor_obj, "name");
    json_object* event_data_obj = json_object_object_get(data_obj, "eventData");

    const char* event_code = strdup(json_object_get_string(code_obj));
    event->event_type = match_event_name_to_constant(event_code);
    event->actor.id = json_object_get_int(actor_id_obj);
    event->actor.name = strdup(json_object_get_string(actor_name_obj));
    event->game_data.node = strdup(json_object_get_string(node_code_obj));

    ref_counter_add(event->rc, event_code);
    ref_counter_add(event->rc, event->actor.name);
    ref_counter_add(event->rc, event->game_data.node);

    event->data = parse_event_data(event, event_data_obj);

    json_object_put(root);

    return event;
}