/*
  manages Transport connections, transport is a means of communication with our client and we currently support three means
  our clients has the opportunity co connect to us using Wifi/Web/TCP or using serial NOTE: we currently support 
  serial text not serial binaries,we are also planning to include bluetooth support as a means of communication but considering
  that bluetooth is heavy and running it side by side with wifi/tcp add bandwitch to our network we are considering using
  bluetooth only for a2dp instead of for normal instructions

  @IGWE_STARKING all rights reserved 
*/

#ifndef ECCLES_WEB_HANDLER
#define ECCLES_WEB_HANDLER


//dependencies
#include<WiFi.h>
#include<ESPAsyncWebServer.h>
#include "Executor.h"

ECCLES_API {

  #define DEFAULT_SSID "Eccles Smart Bike"
  #define DEFAULT_PSWD "12345678"

  //we used namespace here instead of class to avoid too much object abstractions

  //this handles TCP and even UDP transport system
  namespace WebTransport {
    
    void prepare(); //setup the transport layer,NOTE: this function also connects to the network ssid provided and blocks until connect successfully
    void run(); //this code tries to connect to wifi system and run the driver once successful
    void cleanup(); //safely shut down the system
    ResultHandler* getHandler(); //returns handler objects for sending back results

  };

  //handles serial instructions and replies,NOTE:serial instruction processing is heavy since we are dealing with raw strings here
  namespace SerialTransport {
    void prepare(); //sets up the serial layer for instructions,reply and debugging
    void run(); //runs the serial instructions if any
    void cleanup(); //safely shuts down the serial system
    ResultHandler* getHandler(); //returns the serial handler for replies
  };

  //bluetooth is heavy on ESP so we avoid it for now and use it only for A2DP
  /*
  namespace BluetoothTransport {
    void prepare();
    void run();
    void cleanup();
    ResultHandler* getHandler();
  };*/

  /*
    Tranport Manager this manages all transport in transport namespaces and routes result back to the appropriete 
    transport for processing
  */
  namespace Transport {
    void prepare(); //prepares all available transports
    void run(); //execute all supported transports
    void cleanup(); //cleanup all supported transport
    ResultHandler* getHandler(); //this is the default main handler given to command executor
  };

  //handles HTML displaying for web controls
  namespace HTML {
    //displays html page to a browser client using the server provided in arg
    void load(AsyncWebServer& server); //note this loads a static hardcoded page we are working toward creating a dynamic html loader from file
  };


};

#endif