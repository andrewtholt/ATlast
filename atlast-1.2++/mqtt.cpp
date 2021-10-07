#include <stdio.h>
#include <stdlib.h>

#include <mosquitto.h>
#include <string.h>
#include <libgen.h>

#include <typeinfo>
#include <list>
#include <string>
#include <iostream>
#include <fstream>

#include <mutex>
#include <nlohmann/json.hpp>
#include "ringBuffer.h"

using namespace std;
using json = nlohmann::json;

#include "atldef.h"


#define BUFFER_SIZE 16
std::mutex listLock;

ringBuffer buffer(BUFFER_SIZE);

void messageCallback(struct mosquitto *mosq, void *obj,const struct mosquitto_message *message) {
//`   atl_eval("\"cb\" type cr");

    static int count=0;

    static bool firstTime=true;
    struct cbMqttMessage mqttMessage;

    mqttMessage.topic = message->topic ;
    mqttMessage.payload = (char *)message->payload ;

    std::cout << "Rx topic  :" <<  mqttMessage.topic << endl;
    std::cout << "Rx payload  :" <<  mqttMessage.payload << endl;
    std::cout << "Counter   :" << ++count << endl;

    bool failFlag=true;

    std::cout << "Buffer full " <<  buffer.full()  << endl;
    if ( ! buffer.full() ) {
        failFlag = buffer.put( mqttMessage );
    }
    /*
    printf ("Rx topic  :>%s<\n", (char *)message->topic);
    printf ("Rx payload:>%s<\n", (char *)message->payload);

    strcpy( ((struct cbMqttMessage *)obj)->topic,(char *)message->topic);
    strcpy( ((struct cbMqttMessage *)obj)->payload,(char *)message->payload);

    firstTime=false;
    ((struct cbMqttMessage *)obj)->msgFlag++ ;
    */

    std::cout << "Q depth : " << buffer.getSize() << endl;
    atl_eval("\"cb\" type cr");
}

//   2   1   0
// json key result-buffer ---
prim jsonSearch() {
    Sl(3);
    So(0);

    char *dest = (char *) S0;
    char *keyEntry = (char *)S1;
    char *jsonIn = (char *)S2;

    json j = json::parse( jsonIn);

    bool found = false;

    for (const auto& item : j.items()) {        
        if ( item.key() == keyEntry ) {    
            std::cout << "Found" << std::endl;    
            std::cout << "  " << item.key() << ": " << item.value() << "\n";        

            std::cout << (item.value()).is_number() << std::endl;

            string v;
            if ( item.value().is_number() ) {
                int x = item.value();

                v = std::to_string(x);
            } else {
                v = item.value();
            }
            strcpy(dest, v.c_str());
            break;    
        }    

        for (const auto& val : item.value().items()) {    

            if (val.key() == keyEntry) {    
                std::cout << "  " << val.key() << ": " << val.value() << "\n";    
                string v = item.value();

                if( v.at(0) != "\"" ) {
                    v = "\"" + v + "\"";
                }
                strcpy(dest, v.c_str());
                break;
            }    
        }     
    }
    Npop(3);;

}

prim mkStringMap() {
    Sl(0);
    So(1);

    map<string, string> *fred = new map<string,string>;

    Push = fred;
}

prim rmStringMap() {
    Sl(1);
    So(0);

    map<string, string> *fred = (map<string, string> *)S0;

    fred->clear();

    delete fred;
}
// 
// key value --
//
prim mapAdd() {
    Sl(3);
    So(0);

    string value = (char *) S0;
    string key = (char *) S1;
    map<string, string> *fred = (map<string, string> *)S2;

    fred->insert({key, value});

    Npop(3);;
}

prim mapRm() {
    Sl(2);
    So(0);

    string key = (char *)S0;
    map<string, string> *fred = (map<string, string> *)S1;

    fred->erase( key );

    Pop2;
}



prim mapDump() {
    Sl(1);
    So(0);

    map<string, string> *myMap = (map<string, string> *)S0;

    for (const auto &kv : *myMap) {
        std::cout << kv.first << " = " << kv.second << std::endl;
    }
    Pop;
}


prim mkStringList() {
    Sl(0);
    So(1);

    list<string> *fred = new list<string>;
    Push=fred;
}

prim rmStringList() {
    Sl(1);
    So(0);

    list<string> *fred = (list<string> *)S0;

    delete fred;
    Pop;
}

prim stringListEmpty() {
    Sl(1);
    So(1);

    list<string> *fred = (list<string> *)S0;

    S0 = (int) (fred->size() == 0);
}


prim stringPushFront() {
    Sl(2);
    So(0);

    list<string> *fred;

    fred = (list<string> *)S1;
    const std::lock_guard<std::mutex> lock(listLock);
    fred->push_front((char *)S0);

    Pop2;
}
// 
// string ptr --
//
prim stringPopFront() {
    Sl(2);
    So(0);

    char *ptr=(char *)S0;
    char *from;

    bool fail = true;

    const std::lock_guard<std::mutex> lock(listLock);

    list<string> *fred = S1;
    if(fred->size() > 0) {
        from = (fred->front()).c_str();
        int len = strlen(from)+1;
        strncpy(ptr, from, len);

        fred->pop_front();

        fail = false;
    } else {
        ptr[0] = 0;
        fail = true;
    }
    Pop;
    S0 = fail;
}

prim stringPushBack() {
    Sl(2);
    So(0);

    list<string> *fred;

    fred = (list<string> *)S1;

    const std::lock_guard<std::mutex> lock(listLock);
    fred->push_back((char *)S0);

    Pop2;
}

prim stringPopBack() {
    Sl(2);
    So(0);

    char *ptr=(char *)S0;
    char *from;

    bool fail = true;

    const std::lock_guard<std::mutex> lock(listLock);

    list<string> *fred = S1;
    if(fred->size() > 0) {
        from = (fred->front()).c_str();
        int len = strlen(from)+1;
        strncpy(ptr, from, len);

        fred->pop_back();
        fail = false;
    } else {
        ptr[0] = 0;
        fail = true;
    }
    Pop;
    S0 = fail;
}

prim stringListDump() {
    Sl(1);
    So(0);

    int counter=0;
    list<string> *fred = (list<string> *)S0;
    Pop;

    const std::lock_guard<std::mutex> lock(listLock);
    for (list<string>::iterator it=fred->begin(); it!= fred->end(); ++it) {
        cout << counter++ << ' ' << *it << endl;
    }

}

prim stringListSize() {
    Sl(1);
    So(0);

    const std::lock_guard<std::mutex> lock(listLock);
    list<string> *fred = (list<string> *)S0;

    S0=fred->size();
}
// 
// topic list -- 
prim splitTopic() {
    Sl(2);
    So(0);

    char *token;    

    list<string> *fred = (list<string> *)S0;
    char *topicChars = (char *)S1;
    char *rest = topicChars;    

    while ((token = strtok_r(rest, "/", &rest))) {    
//        printf("%s\n", token);    
        fred->push_back( token );    
    }  
    Pop2;
}

void mqttQgetSize() {
    Sl(0);
    So(1);

    int s = buffer.getSize();

    Push=s;
}

void mqttQempty() {
    Sl(0);
    So(1);

    bool e = buffer.empty();

    Push = e;
}

// 
// string:topic string:payload--
void mqttQget() {
    Sl(2);
    So(0);

    std::string *t = (std::string *)S1;
    std::string *m = (std::string *)S0;

    struct cbMqttMessage msg = buffer.get();

    m->assign(  msg.payload);
    t->assign(  msg.topic);

    Pop2;
}

prim mqttInit() {
    Sl(0);
    So(1);

    int rc;
    static bool doneFlag=false;

    if (doneFlag == false) {
        rc=mosquitto_lib_init();
//        memset(&mqttMessage, 0, (size_t)sizeof(struct cbMqttMessage));
        doneFlag=true;
    } else {
        rc=0;
    }
    Push=rc;
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

prim mqttLoopStart() {
    struct mosquitto *mosq ;
    mosq=(struct mosquitto *)S0;

    int rc = mosquitto_loop_start( mosq );

    S0 = rc;
}

prim mqttLoopStop() {
    struct mosquitto *mosq ;
    mosq=(struct mosquitto *)S0;

    int rc = mosquitto_loop_stop( mosq, true );

    S0 = rc;
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

    Push=rc ;
    if( rc == MOSQ_ERR_SUCCESS) {
        Push=false;
    } else {
        Push=true;
    }

}

/*
prim ATH_strtok() {
    char *ptr;
    // Note not re-entrent

    Sl(2);
    So(1);

    ptr=strtok( (char *)S1, (char *)S0);
    Pop;
    S0=(stackitem)ptr;
}
*/

prim ATH_basename() {
    Sl(1);
    So(1);

    char *ptr;

    ptr = basename((char *)S0);
    S0=(stackitem)ptr;
}

prim ATH_dirname() {
    Sl(1);
    So(1);

    char *ptr;

    ptr = dirname((char *)S0);
    S0=(stackitem)ptr;
}

extern prim P_con();
extern prim P_here();

prim stringObj() {
    Sl(0);

    std::cout << "stackitem : " << sizeof(stackitem) << endl;

    std::string *tmp = new std::string("EMPTY");
    Push=tmp;
    P_create();               /* Create dictionary item */
    createword->wcode = P_con;        /* Set code to constant push */
    Ho(1);
    Hstore = S0;              /* Store constant value in body */
    Pop; 
}

prim stringLength() {
    Sl(1);
    So(1);

    std::string *tmp = (std::string *)S0;

    S0 = tmp->length();
}

prim stringCout() {
    Sl(1);
    So(0);

    std::string *tmp = (std::string *)S0;

//    std::cout << tmp ;
    std::cout << *tmp ;

    Pop;
}

/*
std::string myString;
char* data = ...;
int size = ...;
myString.assign(data, size);
*/

//
// ptr string --
//
prim stringStore() {
    Sl(2) ;
    So(0);

    std::string *tmp = (std::string *)S0;
    char *s = (char *)S1;

    tmp->assign(s,strlen(s));

    Pop2;

}

// 
// string -- <ptr here>
prim stringGet() {
    Sl(1);
    So(1);

//    void *bill = (void *)S0;
    std::string *tmp = (std::string *)S0;

    P_here();
    char *dest = (char *)S0;
    Pop;

    strcpy(dest, (*tmp).c_str() );

    std::cout << *tmp << std::endl;

}


static struct primfcn mqtt[] = {
    {"0MK-STRING", stringObj},
    {"0STRING-LENGTH", stringLength},
    {"0COUT", stringCout},
    {"0S!", stringStore},
    {"0S@", stringGet},

    {"0MK-STRING-LIST", mkStringList},
    {"0RM-STRING-LIST", rmStringList},

    {"0STRING-LIST-SIZE", stringListSize},
    {"0STRING-LIST-DUMP", stringListDump},
    {"0STRING-PUSH-FRONT", stringPushFront},
    {"0STRING-POP-FRONT", stringPopFront},

    {"0STRING-PUSH-BACK", stringPushBack},
    {"0STRING-POP-BACK", stringPopBack},
    {"0STRING-LIST-EMPTY?", stringListEmpty},

    {"0MK-STRING-MAP", mkStringMap},
    {"0RM-STRING-MAP", rmStringMap},
    {"0MAP-DUMP", mapDump},
    {"0MAP-ADD", mapAdd},
    {"0MAP-RM", mapRm},

    {"0JSON-SEARCH", jsonSearch},
    {"0SPLIT-TOPIC", splitTopic},

//    {"0STRTOK", ATH_strtok},
    {"0BASENAME", ATH_basename},
    {"0DIRNAME", ATH_dirname},

    {"0MQTT-Q-EMPTY", mqttQempty},
    {"0MQTT-Q-SIZE", mqttQgetSize},
    {"0MQTT-Q-GET", mqttQget},

    {"0MQTT-INIT", mqttInit},
    {"0MQTT-NEW", mqttNew},
    {"0MQTT-CLIENT", mqttClient},
    {"0MQTT-PUB", mqttPublish},
    {"0MQTT-SUB", mqttSubscribe},
    {"0MQTT-LOOP", mqttLoop},
    {"0MQTT-LOOP-START", mqttLoopStart},
    {"0MQTT-LOOP-STOP", mqttLoopStop },

    {NULL, (codeptr) 0}
};

void mqttLoad() {
    atl_primdef( mqtt );
}
