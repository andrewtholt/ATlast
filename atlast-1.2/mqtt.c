#include <stdio.h>
#include <mosquitto.h>
#include <string.h>

#include "atldef.h"

struct cbMqttMessage {
    uint8_t msgFlag;
    char topic[64];
    char payload[32];
} ;

struct cbMqttMessage mqttMessage;

void messageCallback(struct mosquitto *mosq, void *obj,const struct mosquitto_message *message) {
    static char *buffer;
    static bool firstTime=true;


//    printf("================\n");
//    atl_eval(".s cr");
//    Push=obj;
    printf ("Rx topic  : %s\n", (char *)message->topic);
    printf ("Rx payload: %s\n", (char *)message->payload);
    strcpy( ((struct cbMqttMessage *)obj)->topic,(char *)message->topic);
    strcpy( ((struct cbMqttMessage *)obj)->payload,(char *)message->payload);

//    Pop;
    firstTime=false;
    ((struct cbMqttMessage *)obj)->msgFlag++ ;
}

prim mqttGetPayload() {
    Sl(1);
    So(1);

    struct cbMqttMessage *msg;

    msg=(struct cbMqttMessage *)S0;
    S0=(stackitem)&(msg->payload);

}

prim mqttGetTopic() {
    Sl(1);
    So(1);

    struct cbMqttMessage *msg;

    msg=(struct cbMqttMessage *)S0;
    S0=(stackitem)&(msg->topic);

}

prim mqttInit() {
    Sl(0);
    So(1);

    int rc;
    static bool doneFlag=false;

    if (doneFlag == false) {
        rc=mosquitto_lib_init();
        memset(&mqttMessage, 0, (size_t)sizeof(struct cbMqttMessage));
        doneFlag=true;
    } else {
        rc=0;
    }
    Push=rc;
//    Push=(void *)&mqttMessage ;
}
// 
// <client name> <message buffer address> -- <id> false | true
// 
prim mqttNew() {
    Sl(2);
    So(2);

    struct mosquitto *mosq = NULL;
    void *obj=NULL;

    mosq=mosquitto_new((char *)S1, true, (void *)S0);
    Pop;

    if(!mosq) {
        S0=true;
    } else {
        S0=(stackitem)mosq;
        Push=false;
    }

}

// client topic payload -- bool
prim mqttPublish() {
    struct mosquitto *mosq = NULL;
    char *topic;
    char *payload;

    Sl(3);
    So(1);

    int rc=0;

    payload=(char *)S0;
    topic=(char *)S1;
    mosq=(struct mosquitto *)S2;

    Pop2;

    rc=mosquitto_publish(mosq,NULL,topic,strlen(payload), payload, 0, true);
    if( rc == MOSQ_ERR_SUCCESS) {
        S0=false;
    } else {
        S0=true;
    }

}

prim mqttSubscribe() {
    struct mosquitto *mosq = NULL;
    char *topic;

    int rc=0;

    topic=(char *)S0;
    mosq=(struct mosquitto *)S1;
    Pop;

    rc = mosquitto_subscribe(mosq,NULL, topic, 0);

    if( rc == MOSQ_ERR_SUCCESS) {
        S0=false;
    } else {
        S0=true;
    }
}

/*
 * id hostname port 
 */
prim mqttClient() {
    Sl(3);
    So(1);

    struct mosquitto *mosq ;
    char *hostname;
    int port;
    int rc;

    port=S0;
    hostname=(char *)S1;
    mosq=(struct mosquitto *)S2;

    rc = mosquitto_connect(mosq, hostname, port, 10);
    Pop2;

    if( rc == MOSQ_ERR_SUCCESS) {
        mosquitto_message_callback_set (mosq, messageCallback);
        S0=false;
    } else {
        S0=true;
    }
}

// id timeout
prim mqttLoop() {
    int rc;
    struct mosquitto *mosq ;
    int timeout=0;

    timeout=S0;
    mosq=(struct mosquitto *)S1;

    Pop2;

    rc = mosquitto_loop(mosq,timeout,1);

    if( rc == MOSQ_ERR_SUCCESS) {
        Push=false;
    } else {
        Push=true;
        perror("mqtt-loop");
    }

}

prim ATH_strtok() {
    char *ptr;
    // Note not re-entrent

    Sl(2);
    So(1);

    ptr=strtok( (char *)S1, (char *)S0);
    Pop;
    S0=(stackitem)ptr;
}


static struct primfcn mqtt[] = {
    {"0STRTOK", ATH_strtok},
    {"0MQTT-INIT", mqttInit},
    {"0MQTT-NEW", mqttNew},
    {"0MQTT-CLIENT", mqttClient},
    {"0MQTT-PUB", mqttPublish},
    {"0MQTT-SUB", mqttSubscribe},
    {"0MQTT-LOOP", mqttLoop},
    {"0MQTT-TOPIC@", mqttGetTopic},
    {"0MQTT-PAYLOAD@", mqttGetPayload},
    {NULL, (codeptr) 0}
};

void mqttLoad() {
    atl_primdef( mqtt );
}
