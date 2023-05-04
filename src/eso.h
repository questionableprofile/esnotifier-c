#ifndef NOTIFIER_ESO_H_HEADER
#define NOTIFIER_ESO_H_HEADER

#include "util.h"
#include "main.h"

#define ESO_EVENT_NAME_CHAT "chat"
#define ESO_EVENT_NAME_BROADCAST "serverBroadcast"
#define ESO_EVENT_NAME_TRY "tryMessage"
#define ESO_EVENT_NAME_ROLL "userRoll"
#define ESO_EVENT_NAME_DICE "diceResult"
#define ESO_EVENT_NAME_MEDIA_TRACK "youtubePlaying"
#define ESO_EVENT_NAME_DISCONNECT "esoDisconnected"

#define ESO_EVENT_UNEXPECTED   -1
#define ESO_EVENT_CHAT          1
#define ESO_EVENT_BROADCAST     2
#define ESO_EVENT_TRY           3
#define ESO_EVENT_ROLL          4
#define ESO_EVENT_DICE          5
#define ESO_EVENT_MEDIA_TRACK   6
#define ESO_EVENT_DISCONNECT    7

#define ESO_CMD_SEND_MESSAGE    1
#define ESO_CMD_RECONNECT       2

#define ESO_CMD_SEND_MESSAGE_NAME   "sendMessage"
#define ESO_CMD_RECONNECT_NAME      "reconnect"

typedef struct eso_command {
    int type;
    void* data;
    ref_counter_t* ref;
} eso_command_t;

typedef struct eso_send_message_command {
    char* text;
} eso_send_message_command_t;

typedef struct eso_event_game_data {
    char* node;
} eso_event_game_data_t;

typedef struct eso_event_actor {
    int id;
    char* name;
} eso_event_actor_t;

typedef struct eso_event {
    char* event_code;
    int event_type;
    void* data;
    eso_event_game_data_t game_data;
    eso_event_actor_t actor;
    ref_counter_t* rc;
} eso_event_t;

typedef struct eso_media_track {
    char* type;
    char* id;
} eso_media_track_t;

typedef struct eso_event_chat {
    char* message;
} eso_event_chat_t;

typedef struct eso_event_try {
    char* message;
    bool success;
} eso_event_try_t;

typedef struct eso_event_roll {
    int num;
} eso_event_roll_t;

typedef struct eso_dice {
    int num;
    int sides;
} eso_dice_t;

typedef struct eso_event_dice {
    list_t* list;
} eso_event_dice_t;

typedef struct eso_event_broadcast {
    char* message;
} eso_event_broadcast_t;

typedef struct eso_event_disconnect eso_event_disconnect_t;

eso_command_t* eso_command_create (int command_type, void* data, ref_counter_t* ref_counter);
void eso_command_free (eso_command_t* command);
void eso_command_list_free (list_t* command_list);
string_builder_t* eso_command_list_to_json (list_t* command_list);

eso_event_t* eso_event_create ();
void eso_event_free (eso_event_t* event);

eso_event_t* parse_eso_event (char* text);

/*typedef struct eso_event_media_player {
    eso_media_track_t track;
} eso_event_media_player_t;*/

#endif