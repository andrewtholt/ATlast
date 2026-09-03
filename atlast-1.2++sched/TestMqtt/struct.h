/*
 * struct
    64 chars field cmnd-topic
    64 chars field topic    \ <- Index.
    32 chars field msg 
     1 chars field msg-type
     1 chars field dirty
     8 chars field truth
endstruct /mqtt

*/


typedef struct {
    char cmnd_topic[64];
    char topic[64];
    char msg[32];
    char msg_type;
    char dirty;
    char truth[8]
} MQTT;
