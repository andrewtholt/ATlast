/***********************************************************************
 * AUTHOR: andrewh <andrewh>
 *   FILE: .//ringBuffer.cpp
 *   DATE: Tue Jul 20 15:22:33 2021
 *  DESCR: 
 ***********************************************************************/
#include "ringBuffer.h"
#include <iostream>

/***********************************************************************
 *  Method: ringBuffer::ringBuffer
 *  Params: int s
 * Effects: 
 ***********************************************************************/
ringBuffer::ringBuffer(int s) {
    bufferLen = s;
}


/***********************************************************************
 *  Method: ringBuffer::put
 *  Params: cbMqttMessage in
 * Returns: void
 * Effects: 
 ***********************************************************************/
bool ringBuffer::put(cbMqttMessage in) {

    bool fail = true;
    const std::lock_guard<std::mutex> lock(listLock);

    if( buffer.size() < bufferLen ) {
        buffer.push_back( in );
        fail = false;
    } else {
        fail = true;
    }
    return fail;
}


/***********************************************************************
 *  Method: ringBuffer::get
 *  Params: 
 * Returns: cbMqttMessage
 * Effects: 
 ***********************************************************************/
cbMqttMessage ringBuffer::get() {

    cbMqttMessage tmp;
    const std::lock_guard<std::mutex> lock(listLock);

    tmp = buffer.front();
    buffer.pop_front();

    return tmp;
}


/***********************************************************************
 *  Method: ringBuffer::empty
 *  Params: 
 * Returns: bool
 * Effects: 
 ***********************************************************************/
bool ringBuffer::empty() {

    const std::lock_guard<std::mutex> lock(listLock);
    return( buffer.size() == 0);
}


/***********************************************************************
 *  Method: ringBuffer::full
 *  Params: 
 * Returns: bool
 * Effects: 
 ***********************************************************************/
bool ringBuffer::full() {

    const std::lock_guard<std::mutex> lock(listLock);
    return( buffer.size() >= bufferLen );
}


/***********************************************************************
 *  Method: ringBuffer::getSize
 *  Params: 
 * Returns: int
 * Effects: 
 ***********************************************************************/
int ringBuffer::getSize() {

    const std::lock_guard<std::mutex> lock(listLock);
    return buffer.size();
}


/***********************************************************************
 *  Method: ringBuffer::clear
 *  Params: 
 * Returns: void
 * Effects: 
 ***********************************************************************/
void ringBuffer::clear() { 
    const std::lock_guard<std::mutex> lock(listLock);
    buffer.clear();
}


/***********************************************************************
 *  Method: ringBuffer::dump(n)
 *  Params: void
 * Returns: int
 * Effects: Denug display
 ***********************************************************************/
void ringBuffer::dump(cbMqttMessage n) {
    const std::lock_guard<std::mutex> lock(listLock);
    std::cout << "Topic  :" << n.topic << std::endl;
    std::cout << "Payload:" << n.payload << std::endl;;
}

/***********************************************************************
 *  Method: ringBuffer::getMaxSize
 *  Params: 
 * Returns: int
 * Effects: 
 ***********************************************************************/
int ringBuffer::getMaxSize() {
    return bufferLen;
}


/***********************************************************************
 *  Method: ringBuffer::setMaxSize
 *  Params: int n
 * Returns: void
 * Effects: 
 ***********************************************************************/
void ringBuffer::setMaxSize(int n) {
    bufferLen = n;
}


/***********************************************************************
 *  Method: cbMqttMessage::getTopic
 *  Params: 
 * Returns: std::string
 * Effects: 
 ***********************************************************************/
std::string cbMqttMessage::getTopic() {

    std::cout << "Topic  :" << topic << std::endl;
    return topic;
}


/***********************************************************************
 *  Method: cbMqttMessage::getPayload
 *  Params: 
 * Returns: std::string
 * Effects: 
 ***********************************************************************/
std::string cbMqttMessage::getPayload() {
    std::cout << "Payload:" << payload << std::endl;;
    return payload ;
}


