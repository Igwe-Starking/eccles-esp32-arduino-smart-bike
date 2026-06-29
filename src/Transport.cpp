//base implemenation of a transport

#include "Transport.h"
#include "Executors.h"
#include <ESPmDNS.h>
#include <WiFiudp.h>

ECCLES_API {


  //defining the main globals,we do this here to prevent exposing it globally to others and to avoid linker errors
  //which we experienced earlier.

  static AsyncWebServer server(80); //we used static here because we want only this file to own it and avoid all extern linkage
  static AsyncWebSocket socket("/ws");


  //handlers,this process and send result to client
  //web handler
  struct WebHandler : public ResultHandler {
    void sendResult(e_uint8* res,e_uint16 size,e_uint32 senderID) const override {
      //sending result to a web client
      AsyncWebSocketClient* cl = socket.client(senderID);
      if(cl){
        cl->binary(res,size);
      }
    }
  };

  //Serial Handler
  struct SerialHandler : public ResultHandler {
    void sendResult(e_uint8* res,e_uint16 size,e_uint32 senderID) const override {
      //for now we don't send result data to serial since the result data is binary and must be converted to text to make
      //sense on serial log,we only send device state and sensor data

      e_uint16 v = (res[1] << 8) | (res[2] & 0xFF); //we get the Result.size which is where state and sensor data are stored
      e_string msg = nullptr;
      if(v == 0) msg = "OFF"; else if(v == 1) msg= "ON";
      if(msg != nullptr){
        //print state to serial
        Serial.print("Device is ");
        Serial.println(msg);
        return;
      }
      //if we got here we may be dealing with sensor values
      Serial.print("Device value is ");
      Serial.println(v);
    }
  };

  WebHandler wh;
  SerialHandler sh; //web and serial handlers

  //Transport Handler,this is the main transport handle given to executor it detects which transport layer sends the 
  //command and route the result back to the layer, senderID 1 indicates Serial the rest is web

  struct TransportHandler : public ResultHandler {
    void sendResult(e_uint8* res,e_uint16 size,e_uint32 senderID) const override {
      if(senderID == SRL_RT_MGC){
        //hand the result to serial,this is ambiguos because we need to make sure that WebTransport does't give sender id == SRL_RT_MGC but it is unlikely
        SerialTransport::getHandler()->sendResult(res,size,senderID);
      } else {
        //for every other sender id hand the result to web since we currently implement this two
        WebTransport::getHandler()->sendResult(res,size,senderID);
      }
    }
  };

  constexpr e_uint16 PORT = 4210; //our udp port
  constexpr e_string IP = "255.255.255.255"; //not to be  confused with the name this is just our subnet ip

  WiFiUDP udp; //used to broadcast our IP to clients
  e_boolean connected = false;

  e_uint32 delay = 1000; //time interval used to broadcast our IP address,default 1sec

  TransportHandler rh; //processes result and sent to clients

  #define SERIAL_BAUD 115200 //our serial baud rate
  #define SERIAL_BUFFER_MAX 64 //reserved bytes for our serial text
  #define SERIAL_MAX_READ 16 //max serial byte we can read per loop to avoid huging other task and to improve responsiveness


  //this functions runs web socket events
  static void onEvent(AsyncWebSocket *s,AsyncWebSocketClient *client,AwsEventType type,void* arg,e_uint8* data,size_t len){
    if(type == WS_EVT_DATA){
      if(len < 8) return; //corrupted or partial data
      Command* com = BinaryCommand::parse(data,len);
      if(com){
        com->sender = client->id();
        Executor::send(*com);
      }
    } else if(type == WS_EVT_CONNECT){
      connected = true;
      
      //now stop our udp broadcast
      udp.stop();
    } else if(type == WS_EVT_DISCONNECT){
      connected = false;
    }
  }

  //broacasts the IP address to the client incase the Domain name system failed
  void broadcastIP(){
    
    //we add ECCLES_IP because we use it in the client code to detect that we are the one talking on the udp network

    //String txt = "ECCLES_IP:"+WiFi.localIP().toString(); we use snprintf instead of this to minimize heap usage
    char txt[64];
    snprintf(txt,sizeof(txt),"ECCLES_IP:%s",WiFi.localIP().toString().c_str());

    udp.beginPacket(IP,PORT);

    udp.write((e_uint8*)txt,strlen(txt));
    udp.endPacket();
  }

  void WebTransport::prepare(){
    //configuring the ip address, we do this to avoid dynamically assigned IPs not recommended here
    /*

    we tried this on most android and it's not consistent so we default to normal DHCP server
    but the issue is that DHCP is dynamic we don't control which ip is assigned but we are working on a way
    to detect and supply the ip at runtime to our client

    
    IPAddress local(192,168,43,50); //this is our constant IP address on the network
    IPAddress gate(192,168,43,1); //this is our router address,NOTE: our android is the router which is why we use 43 here
    IPAddress sub(255,255,255,0);

    WiFi.config(local,gate,sub);

    */

    //starting wifi driver
    e_string ssid = Configuration::readString("wifi_ssid").c_str();
    e_string psw = Configuration::readString("wifi_pswd").c_str();

    if(eccles_compareString(ssid,"")) ssid = DEFAULT_SSID;
    if(eccles_compareString(psw,"")) psw = DEFAULT_PSWD;

    WiFi.begin(ssid,psw);

  }

  //runs the web transport layer,NOTE: this function runs repeatedly in the loop to avoid blocking the thread waiting for wifi to connect
  void WebTransport::run(){
    static e_boolean initialized = false;
    if(WiFi.status() != WL_CONNECTED){
      if(initialized){ //we where once connected so we cleanup the connection
        cleanup();
      }
       initialized = false;
       return; //we are already connected exit the function
    } 

    if(!connected){ //we support max one connection for now
    
      //we broadcast our IP here,NOTE: if domanin name fail only app can reliable connect to us,web browsing will need to guess our IP
      e_uint32 t = millis();
      if(t - delay >= 1000){
        //we broadcast our IP every seconds so client has chance of reading it and connecting to us
        broadcastIP();
        delay = t + 1000; //non blocking delay
      }
     
    }

   
    if(initialized) return; //init guard to prevent our setup from repeating once connected
      //we are connected to AP but not to a client setup mDNS here
    
    //displaying our ip once in log
    ECCLES_LOG_LINE(WiFi.localIP());

    socket.onEvent(onEvent);
    server.addHandler(&socket);

    //loading our html page here
    HTML::load(server);
    server.begin();
    initialized = true;
    ECCLES_LOG_LINE("connected succussfully to AP");

    //begin our udp broadcast
    udp.begin(PORT);
  }

  void WebTransport::cleanup(){
    //closing all sockets
    socket.closeAll();
    socket.cleanupClients();
  }

  //returning the result handler

  ResultHandler* WebTransport::getHandler(){
    return &wh;
  }

  //Serial transport,controls communication over the serial bus

  void SerialTransport::prepare(){
    //prepairing the serial bus
    Serial.begin(SERIAL_BAUD);
  }

  //runs the serial communication bus
  void SerialTransport::run(){
    static e_char buffer[SERIAL_BUFFER_MAX]; //holds our serial data
    static e_uint8 index = 0;

    e_uint8 rb = SERIAL_MAX_READ;

    while(Serial.available() && rb--){
      e_char c = Serial.read();
      if(c == '\n'){ //we got full instruction
        buffer[index] = '\0'; //we terminate string whenever we got a newline

        //processing of the instruction
        
        e_uint16 len = 0;
        Command* com = StringCommand::parse(buffer,&len);
        if(com != nullptr){
          com->sender = SRL_RT_MGC; //serial sender id
          Executor::send(*com);
        }
        //reseting buffer
        index = 0;
        return;
      }
      if(c == '\r') continue; //we don't need carriage return here
      if(index < SERIAL_BUFFER_MAX - 1){
        //we convert to lower case here
        if(c > 64 && c < 91) c+=32; //lower case conversion
        buffer[index++] = c;
      } else { //overflow we reset buffer index here
        index = 0;
      }
    }
  }


  //cleaning and exiting the serial bus
  void SerialTransport::cleanup(){
    Serial.end();
  }

  //returning the result handler
  ResultHandler* SerialTransport::getHandler(){
    return &sh;
  }

  //prepairing all transports
  void Transport::prepare(){
    //we prepares serial first since its also used for debuging that might debug webtransport
    SerialTransport::prepare();
    WebTransport::prepare();
  }

  //running the transport system
  void Transport::run(){
    //here Web transport first and most important
    WebTransport::run();
    SerialTransport::run();
  }

  //cleaning the transport layers
  void Transport::cleanup(){
    WebTransport::cleanup();
    SerialTransport::cleanup();
  }

  //returning the result processing object
  ResultHandler* Transport::getHandler(){
    return &rh;
  }


  //printing html page for web browser clients,NOTE: only web browsers uses html,android apps uses views to compose their display
  void HTML::load(AsyncWebServer& s){
    //the html source code,NOTE: we currently uses hardcoded source here but we are looking forward to having things like index.html,style.css and scrypt.js 
    //loaded at runtime from littleFS file and sent to our web clients

    constexpr e_string HTML_SOURCE = R"(
      <!DOCTYPE html>
      <html>
      <head>
      <meta charset="utf-8">
      <title>Eccles Smart Control</title>

      <style>
        body {
          font-family: Arial;
          text-align: center;
          background: #111;
          color: white;
        }

        button {
          width: 180px;
          height: 60px;
          margin:10px;
          font-size: 18px;
          border: none;
          border-radius: 10px;
        }

        .on {background: #2ecc71;}
        .off {background: #e74c3c}
      </style>
      </head>

      <body>

        <button data-cmd="3" class="off">Headlamp</button>
        <button data-cmd="2" class="off">Horn</button>
        <button data-cmd="4" class="off">Left</button>
        <button data-cmd="5" class="off">Right</button>

        <script>
          let ws = new WebSocket("ws://"+location.host+"/ws");
          
          document.querySelectorAll("button").forEach(btn => {
            let state = false;

            btn.onclick = () => {
              state = btn.classList.toggle("on");
              let dv = parseInt(btn.dataset.cmd);
              
              let buf = new Uint8Array(8);
              buf[1] = state ? 1 : 2;
              buf[2] = dv;

              ws.send(buf);

              btn.textContent = btn.textContent.split(" ")[0] + (state ? " ON" : " OFF");
              btn.classList.toggle("off",!state);
            };
          });
        </script>
        
      </body>
      </html>
    )";

    //serving our page to web clients,NOTE: we currently support only turning on and off in browsers, enhanced features are only available on apps
    s.on("/",HTTP_GET,[](AsyncWebServerRequest* r){
      r->send(200,"text/html",HTML_SOURCE);
    });
  }
};