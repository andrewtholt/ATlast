#include <cstdint>
#include <string>
#include <list>
#include <mutex>
#include <memory>

class cbMqttMessage {    
    public:
        uint8_t msgFlag;    
        std::string topic ;    
        std::string payload;

        std::string getTopic() ;
        std::string getPayload() ;
} ;

class ringBuffer {
    private:
        int inIdx=0;
        int outIdx=0;

        int bufferLen=4;

        std::list<cbMqttMessage> buffer;
        std::mutex listLock;


    public:
        ringBuffer(int s );

        bool put(cbMqttMessage in);
        cbMqttMessage get();

        bool empty();
        bool full();
        int getSize();
        int getMaxSize();
        void setMaxSize(int n);

        void dump(cbMqttMessage n);

        void clear();
};

