/*
  Eccles bike control software used to power and run smart bike system,created by nwobodo Ecclesiastes
  on 19/03/2026,all right reserved to starking cooperation
*/



//#define ESP_ARDUINO_VERSION_VAL(major,minor,patch) ((major << 16) | (minor << 8) | (patch))
#include "Eccles.h" //Eccles header library

ECCLES_SYSTEM

ExecutorManager manager;

//use this to signal the system that we are using bluetooth
//so they don't release its memory
bool btInUse(){return true;}

ECCLES_API_ENTRY void eccles_main() {

  //installing watchdog timer this task must not hold past 2 sec
  eccles_installWDT(10); //5 seconds timer

  //include the main task here
  eccles_wdtInclude(NULL);

  //initialize the RuntimeMemory
  initRuntimeMemory();

  //initialize the tts engine
  EcclesTTS::initEngine(LANG::EN_UK);
  
  //prepare the executor manager
  manager.prepare();

  //prepare the transport layer
  Transport::prepare();
  #ifdef CONFIG_BT_CLASSIC_ENABLED
  ECCLES_LOG_LINE("bt classic defined");
  if(btInUse()) ECCLES_LOG_LINE("bt in use true"); else ECCLES_LOG_LINE("bt in use false");
  #else
  ECCLES_LOG_LINE("bt classic not defined");
  #endif
  //set the handler for results
  manager.setResultHandler(Transport::getHandler());
}
  
void eccles_thread() {
  //run the transport layer
  Transport::run();

  //run the command layer
  manager.run();
  
  //tell watchdog we are still alive
  eccles_wdtReset();
}
