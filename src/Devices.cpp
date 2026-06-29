#include "Devices.h"
#include "EcclesTTS.h"

//device hardware implementation

ECCLES_API {

  //digital io devices
  DigitalIODevice::DigitalIODevice(const e_uint8 p,const e_uint8 fb_p,e_boolean activeL,e_string audioK) : Device(p),fb_pin(fb_p),activeLow(activeL),audioKey(audioK){
    type = DIGITAL_IO;
  }
  
  //turn on device and check feedback to confirm its actualy on
  void DigitalIODevice::on(){
    if(enabled){
      e_strings audiop = "ok"; // a response to turn on device so ok first
      if(audible){
        respond(audiop.c_str());
      }
      audiop = audioKey;

      eccles_enable(pin);
      desiredState = ON;
      update();
      if(audible){
        if(actualState == ON){
          audiop += " turned on successfully";
          respond(audiop.c_str());
        } else {
          audiop += " activation failed";
          respond(audiop.c_str());
        }
      }
    }
  } 

  //turn off device and check feedback to confirm its actualy off;
  void DigitalIODevice::off(){
    e_strings audiop = "ok";
    if(audible){
      respond(audiop.c_str());
    }
    eccles_disable(pin); //turn off pin wether enabled or not
    desiredState = OFF;
    update();
    audiop = audioKey;
    if(audible){
      if(actualState == OFF){
        audiop += " turned off successfully";
        respond(audiop.c_str());
      } else {
        audiop += " failed to turn off";
        respond(audiop.c_str());
      }
    }
  }

  //enable device
  void DigitalIODevice::enable(){
    Device::enable();
    e_strings audiop= audioKey;
    audiop += " enabled successfully";
    respond(audiop.c_str());
  }

  //disable device
  void DigitalIODevice::disable(){
    Device::disable();
    e_strings audiop = audioKey;
    audiop += " disabled successfully";
    respond(audiop.c_str());
  }

  //make the device silent
  void DigitalIODevice::silent(e_boolean s){
    audible = !s;
  }

  //respond the current state of device
  void DigitalIODevice::respondState(){
    if(audible){
      State s = getState();
      e_strings audiop = audioKey;
      if(s == ON){
        audiop += " is on";
        respond(audiop.c_str());
      } else {
        audiop += "is off";
        respond(audiop.c_str());
      }
    }
  }

  //get the State of the hardware device,NOTE: the state is the actual state.
  State DigitalIODevice::getState(){
    update();
    return actualState;
  }

  //read data from the sensor input, this is only useful for analog device we dont need it here
  e_uint8 DigitalIODevice::read(){ return 0;}

  //update the state of the device
  void DigitalIODevice::update(){
    ECCLES_LOG_LINE("updating the state of device");
    actualState = eccles_pinState(fb_pin) == (activeLow ? LOW : HIGH) ? ON : OFF;
  }

  //audio response
  void DigitalIODevice::respond(e_string s){
    EcclesTTS::speak(s);
  }

  //DigitalOutput devices,this device controls hardware that can only be toggled by us and in that case
  //we dont need to read feedback from external events.

  DigitalOutputDevice::DigitalOutputDevice(const e_uint8 p,const e_uint8 fb_p,e_boolean activeL,e_string audioK) : DigitalIODevice(p,fb_p,activeL,audioK) {
    type = DIGITAL_OUTPUT;
  }

  void DigitalOutputDevice::update(){
    actualState = desiredState; //we control the toggling alone so the feedback depends on what we asked it to do.
  }

  //DigitalOutputAnalogFeedbackDevice, the device that process analog feedback but controlls
  //digital devices.

  DigitalOutputAnalogFeedbackDevice::DigitalOutputAnalogFeedbackDevice(const e_uint8 p,const e_uint8 fb_p,e_boolean activeL,e_string audioK) : DigitalIODevice(p,fb_p,activeL,audioK) {
    type = DIGITAL_OUTPUT_ANALOG_INPUT;
  }
  void DigitalOutputAnalogFeedbackDevice::update(){
    ECCLES_LOG_LINE("updating state of a DigitalOutputAnalogFeedbackDevice");
    actualState = value > 0 ? ON : OFF;
  }
  e_uint8 DigitalOutputAnalogFeedbackDevice::read(){
    value = eccles_read(fb_pin);
    return value;
  }


  //analog devices, this device reads data and update global variables
  AnalogInputDevice::AnalogInputDevice(e_uint8 p) : Device(p){
    type = ANALOG_INPUT;
  }

  //on and off not needed here needed only for digital devices
  void AnalogInputDevice::on(){}
  void AnalogInputDevice::off(){}

  //State not needed here we dont care about feedback
  State AnalogInputDevice::getState(){return ON;}

  //read the current analog input from the device
  e_uint8 AnalogInputDevice::read(){
    data = eccles_read(pin);
    return data;
  }

  //read and write data continuosly to an input stream,reserved for future implementation
  void AnalogInputDevice::stream(){}

  //SerialInput devices this type of device reads digital pulses per time
  SerialInputDevice::SerialInputDevice(e_uint8 p) : AnalogInputDevice(p) {
    type = SERIAL_INPUT;
    start();
  }

  SerialInputDevice::~SerialInputDevice(){
    stop();
  }

  void IRAM_ATTR SerialInputDevice::isrRouter(void* a){

    SerialInputDevice* self = static_cast<SerialInputDevice*>(a);
    e_uint now = micros();
    self->deltaTime = now - self->lastTime;
    self->lastTime = now;
    self->pulseCount++;
  }

  //reads pulses and frequency from serial device
  e_uint8 SerialInputDevice::read(){
    static e_uint8 smoothed = 0;

    e_uint dt = deltaTime;
    e_uint pulses =pulseCount;

    portENTER_CRITICAL(&mux);
    if(millis() - (lastTime / 1000) > 200){
      smoothed = 0;
      return 0;
    }

    e_uint freq = (dt > 0) ? (1000000UL / dt) : 0;
    e_uint raw = (freq >> 4) + (pulses << 2);

    if(raw > 255) raw = 255;
    e_uint8 value = (e_uint8) raw;

    smoothed = (smoothed * 3 + value) >> 2;
    
    portEXIT_CRITICAL(&mux);
    return smoothed;
  }

  void SerialInputDevice::start(){
    attachInterruptArg(pin,isrRouter,this,RISING);
  }

  void SerialInputDevice::stop(){
    detachInterrupt(digitalPinToInterrupt(pin));
  }

  void SerialInputDevice::stream(){
    
  }

};