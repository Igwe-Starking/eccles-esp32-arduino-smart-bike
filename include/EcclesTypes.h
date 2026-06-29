/*
  Eccles ESP 32 Library holding custom typenames and common functions,this file holds every 
  Eccles abstraction library for arduino esp
*/

#ifndef ECCLES_TYPES
#define ECCLES_TYPES

//arduino core library
#include<Arduino.h>
#include<string>
#include "EcclesConfig.h"

//Eccles core definitions
#define ECCLES_SYSTEM using namespace Eccles;
#define ECCLES_API namespace Eccles
#define ECCLES_API_ENTRY
#define eccles_main setup
#define eccles_thread loop

//arduino functions wrapper
#ifdef ECCLES_DEBUG
#define ECCLES_LOG(arg) Serial.print((arg))
#define ECCLES_LOG_LINE(arg) Serial.println((arg))
#else
#define ECCLES_LOG(arg)
#define ECCLES_LOG_LINE(arg)
#endif

//architecture abstractions, this definitions is used to abstract framework architecture,this makes it easy to change the framework for different architecture

#define eccles_enable(pin) digitalWrite((pin),HIGH)
#define eccles_disable(pin) digitalWrite((pin),LOW)
#define eccles_isOn(pin) (digitalRead((pin)) == HIGH)
#define eccles_isOff(pin) (digitalRead((pin)) == LOW)
#define eccles_pinState(pin) (digitalRead((pin)))
#define eccles_read(pin) ((analogRead((pin))* 255) / 4095) //we care about 0 - 255 here we dont need micro resolution.
#define eccles_write(pin,value) (analogWrite((pin),(value)))
#define eccles_wait(time) (vTaskDelay(time / portTICK_PERIOD_MS))
#define eccles_taskInit(func,name,st,arg) (xTaskCreate(func,name,st,arg,1,NULL)) //all task should currently have a fair share of the cpu so we use 1 priority for all
#define eccles_taskDelete() (vTaskDelete(NULL)) //we currently don't need the task handle task are auto deleted by themselves
#define eccles_createMsgQueue(num,size) (xQueueCreate(num,size))
#define eccles_deleteMsgQueue(queue) (vQueueDelete(queue))
#define eccles_sendMsg(queue,msg) (xQueueSend(queue,msg,portMAX_DELAY))
#define eccles_readMsg(queue,msg) (xQueueReceive(queue,msg,portMAX_DELAY))
#define eccles_clearMsg(msg) (xQueueReset(msg))
#define eccles_writeDAC(pin,value) (dacWrite(pin,value))
#define eccles_waitMicro(value) (ets_delay_us(value))
#define eccles_hasMsg(queue) (uxQueueMessagesWaiting(queue))
#define eccles_installWDT(time) (esp_task_wdt_init((time),(true)))
#define eccles_wdtReset()(esp_task_wdt_reset())
#define eccles_wdtInclude(task) (esp_task_wdt_add((task)))
#define eccles_deleteWDT(task) (esp_task_wdt_delete((NULL)))
#define eccles_lock(mux,t) (xSemaphoreTake(mux,t))
#define eccles_unlock(mux) (xSemaphoreGive(mux))
#define eccles_createLock() (xSemaphoreCreateMutex())
#define eccles_startLog(br) (Serial.begin(br))
#define eccles_mutex SemaphoreHandle_t
#define e_stringa String //done like this because using causes problem with this


//Eccles namespace used in all Eccles projects
ECCLES_API {

//custom typenames

using e_uint = unsigned int;
using e_int = int;
using e_uint8 = uint8_t;
using e_uint16 = uint16_t;
using e_uint32 = uint32_t;
using e_float = float;
using e_boolean = bool;
using e_uint64 = uint64_t;
using e_string = const char*;
using e_char = char;
using e_strings = std::string;


//custom values

#define e_true true
#define e_false false

//state definitions
 enum class State {
    OFF,ON,UNKNOWN
  };

  constexpr State ON = State::ON;
  constexpr State OFF = State::OFF;
  constexpr State UNKNOWN = State::UNKNOWN;

  struct GLOBAL_STATE {
    State IGNITION_STATE = OFF;
    State AUDIBLE = ON;
    e_uint8 TEMP_DATA = 0;
    e_uint8 FUEL_DATA = 0;
    State ENGINE_LOCK = OFF;
    State ENGINE_RUNNING = OFF;
    e_float VOLTAGE_LEVEL = 0.0f;
    e_uint8 SHOCK_DATA = 0;
    e_boolean EXECUTION_MODE = true;
  };

  extern GLOBAL_STATE globalState; //handle to global data
  
  //fast string comparism fuction
  e_boolean eccles_compareString(e_string a,e_string b);

  //string hashing functions
  //compile time hashine
  constexpr e_uint32 eccles_hashCT(e_string s,e_uint32 h = 5381){
    return (*s == 0) ? h : eccles_hashCT(s + 1,((h << 5) + h) + (e_uint8)(*s));
  }

  const e_uint32 eccles_hashRT(e_string s); //runtime hashing


};

#endif