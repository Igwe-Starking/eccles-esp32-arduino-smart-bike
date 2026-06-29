//Base implementation of Conversation executor

#include "Executors.h"
#include "EcclesTTS.h"
#include <WiFiudp.h>
#include <driver/i2s.h>
#include <cstring>

ECCLES_API {

    Conversation* Conversation::instance = nullptr;
    constexpr e_uint16 PORT = 4210; //our remote port
    constexpr e_string IP = "255.255.255.255"; //our remote ip
    constexpr e_uint8 RAW_REPLY = 36; //used to track conversation AI mode that should reply exactly what is sent


    WiFiUDP cnUdp;

    //controls conversation in AI mode, lives only here, no header needed
    //word by word hashing,same pattern BinaryCommand::parse uses in Executor.cpp
    class ConversationAI {
        public:

        //caller owned buffer,must be at least REPLY_BUF_SIZE bytes
        static constexpr e_uint8 REPLY_BUF_SIZE = 64;

        static void reply(e_string heard,e_char* out){
            out[0] = '\0';
            if(heard == nullptr || heard[0] == '\0'){
                copyReply(out,pick(GREET_FALLBACK,GREET_FALLBACK_N,heard));
                return;
            }

            e_char words[96];
            e_uint8 i = 0;
            for(; i < sizeof(words) - 1 && heard[i] != '\0'; ++i){
                words[i] = (e_char) tolower((e_uint8) heard[i]);
            }
            words[i] = '\0';

            //action hashes
            constexpr e_uint32 H_HELLO = eccles_hashCT("hello");
            constexpr e_uint32 H_HI = eccles_hashCT("hi");
            constexpr e_uint32 H_HEY = eccles_hashCT("hey");
            constexpr e_uint32 H_MORNING = eccles_hashCT("morning");
            constexpr e_uint32 H_EVENING = eccles_hashCT("evening");
            constexpr e_uint32 H_NIGHT = eccles_hashCT("night");
            constexpr e_uint32 H_BYE = eccles_hashCT("bye");
            constexpr e_uint32 H_GOODBYE = eccles_hashCT("goodbye");
            constexpr e_uint32 H_LATER = eccles_hashCT("later");
            constexpr e_uint32 H_THANKS = eccles_hashCT("thanks");
            constexpr e_uint32 H_THANK = eccles_hashCT("thank");
            constexpr e_uint32 H_APPRECIATE = eccles_hashCT("appreciate");
            constexpr e_uint32 H_HOW = eccles_hashCT("how");
            constexpr e_uint32 H_ARE = eccles_hashCT("are");
            constexpr e_uint32 H_YOU = eccles_hashCT("you");
            constexpr e_uint32 H_DOING = eccles_hashCT("doing");
            constexpr e_uint32 H_FEELING = eccles_hashCT("feeling");
            constexpr e_uint32 H_FUEL = eccles_hashCT("fuel");
            constexpr e_uint32 H_GAS = eccles_hashCT("gas");
            constexpr e_uint32 H_TEMP = eccles_hashCT("temp");
            constexpr e_uint32 H_TEMPERATURE = eccles_hashCT("temperature");
            constexpr e_uint32 H_HOT = eccles_hashCT("hot");
            constexpr e_uint32 H_ENGINE = eccles_hashCT("engine");
            constexpr e_uint32 H_MOTOR = eccles_hashCT("motor");
            constexpr e_uint32 H_BLUETOOTH = eccles_hashCT("bluetooth");
            constexpr e_uint32 H_CONNECTED = eccles_hashCT("connected");
            constexpr e_uint32 H_BATTERY = eccles_hashCT("battery");
            constexpr e_uint32 H_VOLTAGE = eccles_hashCT("voltage");
            constexpr e_uint32 H_POWER = eccles_hashCT("power");
            constexpr e_uint32 H_NAME = eccles_hashCT("name");
            constexpr e_uint32 H_WHO = eccles_hashCT("who");
            constexpr e_uint32 H_IGNITION = eccles_hashCT("ignition");
            constexpr e_uint32 H_LOCK = eccles_hashCT("lock");
            constexpr e_uint32 H_LOCKED = eccles_hashCT("locked");
            constexpr e_uint32 H_HORN = eccles_hashCT("horn");
            constexpr e_uint32 H_HEADLAMP = eccles_hashCT("headlamp");
            constexpr e_uint32 H_LIGHT = eccles_hashCT("light");
            constexpr e_uint32 H_LIGHTS = eccles_hashCT("lights");
            constexpr e_uint32 H_TURN = eccles_hashCT("turn");
            constexpr e_uint32 H_SIGNAL = eccles_hashCT("signal");
            constexpr e_uint32 H_SHOCK = eccles_hashCT("shock");
            constexpr e_uint32 H_BUMP = eccles_hashCT("bump");
            constexpr e_uint32 H_HELP = eccles_hashCT("help");
            constexpr e_uint32 H_JOKE = eccles_hashCT("joke");
            constexpr e_uint32 H_FUNNY = eccles_hashCT("funny");
            constexpr e_uint32 H_GOOD = eccles_hashCT("good");
            constexpr e_uint32 H_GREAT = eccles_hashCT("great");
            constexpr e_uint32 H_FINE = eccles_hashCT("fine");
            constexpr e_uint32 H_AWESOME = eccles_hashCT("awesome");
            constexpr e_uint32 H_BAD = eccles_hashCT("bad");
            constexpr e_uint32 H_SAD = eccles_hashCT("sad");
            constexpr e_uint32 H_TIRED = eccles_hashCT("tired");
            constexpr e_uint32 H_WORRIED = eccles_hashCT("worried");
            constexpr e_uint32 H_SCARED = eccles_hashCT("scared");
            constexpr e_uint32 H_LOVE = eccles_hashCT("love");
            constexpr e_uint32 H_HATE = eccles_hashCT("hate");
            constexpr e_uint32 H_READY = eccles_hashCT("ready");
            constexpr e_uint32 H_RIDE = eccles_hashCT("ride");
            constexpr e_uint32 H_GO = eccles_hashCT("go");
            constexpr e_uint32 H_SPEED = eccles_hashCT("speed");
            constexpr e_uint32 H_FAST = eccles_hashCT("fast");
            constexpr e_uint32 H_WEATHER = eccles_hashCT("weather");
            constexpr e_uint32 H_TIME = eccles_hashCT("time");
            constexpr e_uint32 H_DATE = eccles_hashCT("date");
            constexpr e_uint32 H_OK = eccles_hashCT("ok");
            constexpr e_uint32 H_OKAY = eccles_hashCT("okay");
            constexpr e_uint32 H_YES = eccles_hashCT("yes");
            constexpr e_uint32 H_NO = eccles_hashCT("no");
            constexpr e_uint32 H_PLEASE = eccles_hashCT("please");
            constexpr e_uint32 H_SORRY = eccles_hashCT("sorry");
            constexpr e_uint32 H_WHATSUP = eccles_hashCT("sup");
            constexpr e_uint32 H_STATUS = eccles_hashCT("status");
            constexpr e_uint32 H_CHECK = eccles_hashCT("check");
            constexpr e_uint32 H_REPORT = eccles_hashCT("report");

            e_boolean sawHow = false, sawAre = false, sawYou = false, sawDoing = false, sawFeeling = false, sawStatus = false, sawSignal = false;

            e_char* word = strtok(words," ");
            while(word != nullptr){
                const e_uint32 h = eccles_hashRT(word);

                switch(h){
                    case H_HELLO: case H_HI: case H_HEY: case H_WHATSUP:
                        copyReply(out,pick(GREETINGS,GREETINGS_N,heard)); return;

                    case H_MORNING:
                        copyReply(out,pick(MORNING,MORNING_N,heard)); return;

                    case H_EVENING: case H_NIGHT:
                        copyReply(out,pick(EVENING,EVENING_N,heard)); return;

                    case H_BYE: case H_GOODBYE: case H_LATER:
                        copyReply(out,pick(FAREWELLS,FAREWELLS_N,heard)); return;

                    case H_THANK: case H_THANKS: case H_APPRECIATE:
                        copyReply(out,pick(THANKS,THANKS_N,heard)); return;

                    case H_NAME: case H_WHO:
                        copyReply(out,pick(IDENTITY,IDENTITY_N,heard)); return;

                    case H_HELP:
                        copyReply(out,pick(HELP_REPLIES,HELP_REPLIES_N,heard)); return;

                    case H_JOKE: case H_FUNNY:
                        copyReply(out,pick(JOKES,JOKES_N,heard)); return;

                    case H_SORRY:
                        copyReply(out,pick(APOLOGY_ACK,APOLOGY_ACK_N,heard)); return;

                    case H_GOOD: case H_GREAT: case H_FINE: case H_AWESOME: case H_OK: case H_OKAY: case H_YES:
                        copyReply(out,pick(POSITIVE_ACK,POSITIVE_ACK_N,heard)); return;

                    case H_BAD: case H_SAD: case H_TIRED: case H_WORRIED: case H_SCARED: case H_NO:
                        copyReply(out,pick(NEGATIVE_ACK,NEGATIVE_ACK_N,heard)); return;

                    case H_LOVE:
                        copyReply(out,pick(LOVE_ACK,LOVE_ACK_N,heard)); return;

                    case H_HATE:
                        copyReply(out,pick(HATE_ACK,HATE_ACK_N,heard)); return;

                    case H_READY: case H_RIDE: case H_GO:
                        copyReply(out,pick(RIDE_HYPE,RIDE_HYPE_N,heard)); return;

                    case H_SPEED: case H_FAST:
                        copyReply(out,pick(SPEED_TALK,SPEED_TALK_N,heard)); return;

                    case H_WEATHER:
                        copyReply(out,pick(WEATHER_REPLIES,WEATHER_REPLIES_N,heard)); return;

                    case H_TIME: case H_DATE:
                        copyReply(out,pick(TIME_REPLIES,TIME_REPLIES_N,heard)); return;

                    case H_PLEASE:
                        copyReply(out,pick(POLITE_ACK,POLITE_ACK_N,heard)); return;

                    case H_BLUETOOTH: case H_CONNECTED: {
                        Bluetooth* bt = Bluetooth::instance;
                        e_boolean on = (bt != nullptr) && bt->on;
                        copyReply(out,on ? pick(BT_ON,BT_ON_N,heard) : pick(BT_OFF,BT_OFF_N,heard));
                        return;
                    }

                    case H_ENGINE: case H_MOTOR: {
                        e_boolean running = (globalState.ENGINE_RUNNING == ON);
                        copyReply(out,running ? pick(ENGINE_ON,ENGINE_ON_N,heard) : pick(ENGINE_OFF,ENGINE_OFF_N,heard));
                        return;
                    }

                    case H_IGNITION: {
                        e_boolean on = (globalState.IGNITION_STATE == ON);
                        copyReply(out,on ? pick(IGN_ON,IGN_ON_N,heard) : pick(IGN_OFF,IGN_OFF_N,heard));
                        return;
                    }

                    case H_LOCK: case H_LOCKED: {
                        e_boolean locked = (globalState.ENGINE_LOCK == ON);
                        copyReply(out,locked ? pick(LOCK_ON,LOCK_ON_N,heard) : pick(LOCK_OFF,LOCK_OFF_N,heard));
                        return;
                    }

                    case H_FUEL: case H_GAS:
                        copyReply(out,pick(FUEL_PREFIX,FUEL_PREFIX_N,heard));
                        appendPercent(out,globalState.FUEL_DATA);
                        return;

                    case H_TEMP: case H_TEMPERATURE: case H_HOT:
                        copyReply(out,pick(TEMP_PREFIX,TEMP_PREFIX_N,heard));
                        appendPercent(out,globalState.TEMP_DATA);
                        return;

                    case H_BATTERY: case H_VOLTAGE: case H_POWER:
                        copyReply(out,pick(BATTERY_PREFIX,BATTERY_PREFIX_N,heard));
                        appendPercent(out,(e_uint8) globalState.VOLTAGE_LEVEL);
                        return;

                    case H_HORN:
                        copyReply(out,pick(HORN_REPLIES,HORN_REPLIES_N,heard)); return;

                    case H_HEADLAMP: case H_LIGHT: case H_LIGHTS:
                        copyReply(out,pick(LIGHT_REPLIES,LIGHT_REPLIES_N,heard)); return;

                    case H_TURN: case H_SIGNAL: sawSignal = true; break;

                    case H_SHOCK: case H_BUMP:
                        copyReply(out,pick(SHOCK_REPLIES,SHOCK_REPLIES_N,heard)); return;

                    case H_HOW: sawHow = true; break;
                    case H_ARE: sawAre = true; break;
                    case H_YOU: sawYou = true; break;
                    case H_DOING: sawDoing = true; break;
                    case H_FEELING: sawFeeling = true; break;
                    case H_STATUS: case H_CHECK: case H_REPORT: sawStatus = true; break;
                    default: break;
                }

                word = strtok(nullptr," ");
            }

            //order independent "how are you / how you doing / how you feeling"
            if(sawHow && sawYou && (sawAre || sawDoing || sawFeeling)){
                copyReply(out,pick(WELLBEING_REPLIES,WELLBEING_REPLIES_N,heard));
                return;
            }

            //generic status/check/report words only matter if nothing specific matched
            if(sawStatus){
                copyReply(out,pick(STATUS_REPLIES,STATUS_REPLIES_N,heard));
                return;
            }

            //turn/signal only matters if no other device word already matched
            if(sawSignal){
                copyReply(out,pick(SIGNAL_REPLIES,SIGNAL_REPLIES_N,heard));
                return;
            }

            copyReply(out,pick(GENERIC_REPLIES,GENERIC_REPLIES_N,heard));
        }

        private:

        static void copyReply(e_char* out,e_string text){
            e_uint8 i = 0;
            for(; i < REPLY_BUF_SIZE - 1 && text[i] != '\0'; ++i) out[i] = text[i];
            out[i] = '\0';
        }

        //appends a 0-100 number as plain digits without overrunning REPLY_BUF_SIZE
        static void appendPercent(e_char* out,e_uint8 value){
            e_uint8 v = value > 100 ? 100 : value;
            e_uint8 len = 0;
            while(out[len] != '\0' && len < REPLY_BUF_SIZE - 1) ++len;
            if(len >= REPLY_BUF_SIZE - 4) return;

            if(v >= 100){ out[len]='1'; out[len+1]='0'; out[len+2]='0'; out[len+3]='\0'; }
            else if(v >= 10){ out[len]=(e_char)('0'+(v/10)); out[len+1]=(e_char)('0'+(v%10)); out[len+2]='\0'; }
            else { out[len]=(e_char)('0'+v); out[len+1]='\0'; }
        }

        //xorshift32,no rand(),deterministic and allocation free
        static e_uint32 nextRand(){
            static e_uint32 state = 0x9E3779B9u;
            state ^= state << 13; state ^= state >> 17; state ^= state << 5;
            return state;
        }

        //picks a varied reply,salts the seed with the heard phrase so back to back
        //identical inputs don't always land on the same index
        static e_string pick(const e_string* list,e_uint8 count,e_string heard){
            if(count == 0) return "";
            e_uint32 salt = (heard != nullptr) ? eccles_hashRT(heard) : 0;
            return list[(nextRand() + salt) % count];
        }

        //reply banks,grouped by intent,plain literal arrays cost only pointers in rodata

        static constexpr e_string GREETINGS[] = {
            "hello there how can i help you today",
            "hey good to hear your voice",
            "hi there im listening",
            "hello im here whats on your mind",
            "hey there ready when you are"
        };
        static constexpr e_uint8 GREETINGS_N = sizeof(GREETINGS)/sizeof(GREETINGS[0]);

        static constexpr e_string MORNING[] = {
            "good morning hope you slept well",
            "morning lets have a good ride today",
            "good morning the bike is ready when you are"
        };
        static constexpr e_uint8 MORNING_N = sizeof(MORNING)/sizeof(MORNING[0]);

        static constexpr e_string EVENING[] = {
            "good evening how was your day",
            "evening lets get you home safe",
            "good evening im here if you need anything"
        };
        static constexpr e_uint8 EVENING_N = sizeof(EVENING)/sizeof(EVENING[0]);

        static constexpr e_string FAREWELLS[] = {
            "goodbye ride safe out there",
            "see you later take care",
            "bye for now ill be here",
            "later have a smooth ride",
            "goodbye stay safe on the road"
        };
        static constexpr e_uint8 FAREWELLS_N = sizeof(FAREWELLS)/sizeof(FAREWELLS[0]);

        static constexpr e_string THANKS[] = {
            "youre very welcome",
            "anytime happy to help",
            "no problem at all",
            "glad i could help"
        };
        static constexpr e_uint8 THANKS_N = sizeof(THANKS)/sizeof(THANKS[0]);

        static constexpr e_string IDENTITY[] = {
            "im eccles your bikes assistant",
            "im the eccles conversation system built into your bike",
            "eccles here built to keep you company on the road"
        };
        static constexpr e_uint8 IDENTITY_N = sizeof(IDENTITY)/sizeof(IDENTITY[0]);

        static constexpr e_string HELP_REPLIES[] = {
            "i can check fuel temp battery engine and bluetooth",
            "ask about fuel temp battery engine or bluetooth",
            "i can report on bike systems or just chat with you"
        };
        static constexpr e_uint8 HELP_REPLIES_N = sizeof(HELP_REPLIES)/sizeof(HELP_REPLIES[0]);

        static constexpr e_string JOKES[] = {
            "why did the bike fall over it was two tired",
            "i would tell you a road joke but it might not go anywhere",
            "engines dont tell jokes they just rev about it"
        };
        static constexpr e_uint8 JOKES_N = sizeof(JOKES)/sizeof(JOKES[0]);

        static constexpr e_string APOLOGY_ACK[] = {
            "no worries at all",
            "its fine dont worry about it",
            "all good no need to apologize"
        };
        static constexpr e_uint8 APOLOGY_ACK_N = sizeof(APOLOGY_ACK)/sizeof(APOLOGY_ACK[0]);

        static constexpr e_string POSITIVE_ACK[] = {
            "thats great to hear",
            "glad things are going well",
            "good to know lets keep going",
            "love to hear that"
        };
        static constexpr e_uint8 POSITIVE_ACK_N = sizeof(POSITIVE_ACK)/sizeof(POSITIVE_ACK[0]);

        static constexpr e_string NEGATIVE_ACK[] = {
            "sorry to hear that hope it gets better soon",
            "thats rough take it easy out there",
            "i hear you take your time"
        };
        static constexpr e_uint8 NEGATIVE_ACK_N = sizeof(NEGATIVE_ACK)/sizeof(NEGATIVE_ACK[0]);

        static constexpr e_string LOVE_ACK[] = {
            "thats sweet of you to say",
            "love this ride too"
        };
        static constexpr e_uint8 LOVE_ACK_N = sizeof(LOVE_ACK)/sizeof(LOVE_ACK[0]);

        static constexpr e_string HATE_ACK[] = {
            "sorry to hear that lets see if i can help",
            "thats no good lets sort it out"
        };
        static constexpr e_uint8 HATE_ACK_N = sizeof(HATE_ACK)/sizeof(HATE_ACK[0]);

        static constexpr e_string RIDE_HYPE[] = {
            "lets go im ready when you are",
            "ready when you are lets ride",
            "lets hit the road"
        };
        static constexpr e_uint8 RIDE_HYPE_N = sizeof(RIDE_HYPE)/sizeof(RIDE_HYPE[0]);

        static constexpr e_string SPEED_TALK[] = {
            "lets keep it safe out there",
            "speed feels good just stay safe",
            "i trust you just watch the road"
        };
        static constexpr e_uint8 SPEED_TALK_N = sizeof(SPEED_TALK)/sizeof(SPEED_TALK[0]);

        static constexpr e_string WEATHER_REPLIES[] = {
            "i cant check the weather yet but ride safe either way",
            "no weather sensor on board yet just watch the road conditions"
        };
        static constexpr e_uint8 WEATHER_REPLIES_N = sizeof(WEATHER_REPLIES)/sizeof(WEATHER_REPLIES[0]);

        static constexpr e_string TIME_REPLIES[] = {
            "i dont have a clock yet but ill let you know once i do",
            "no clock on board yet check your phone for now"
        };
        static constexpr e_uint8 TIME_REPLIES_N = sizeof(TIME_REPLIES)/sizeof(TIME_REPLIES[0]);

        static constexpr e_string POLITE_ACK[] = {
            "of course go ahead",
            "sure tell me what you need"
        };
        static constexpr e_uint8 POLITE_ACK_N = sizeof(POLITE_ACK)/sizeof(POLITE_ACK[0]);

        static constexpr e_string BT_ON[] = {
            "bluetooth is connected and ready",
            "bluetooth is on and linked up"
        };
        static constexpr e_uint8 BT_ON_N = sizeof(BT_ON)/sizeof(BT_ON[0]);

        static constexpr e_string BT_OFF[] = {
            "bluetooth is currently off",
            "bluetooth is not connected right now"
        };
        static constexpr e_uint8 BT_OFF_N = sizeof(BT_OFF)/sizeof(BT_OFF[0]);

        static constexpr e_string ENGINE_ON[] = {
            "the engine is running smoothly",
            "engine is on and running fine"
        };
        static constexpr e_uint8 ENGINE_ON_N = sizeof(ENGINE_ON)/sizeof(ENGINE_ON[0]);

        static constexpr e_string ENGINE_OFF[] = {
            "the engine is currently off",
            "engine is off right now"
        };
        static constexpr e_uint8 ENGINE_OFF_N = sizeof(ENGINE_OFF)/sizeof(ENGINE_OFF[0]);

        static constexpr e_string IGN_ON[] = { "ignition is on" };
        static constexpr e_uint8 IGN_ON_N = sizeof(IGN_ON)/sizeof(IGN_ON[0]);

        static constexpr e_string IGN_OFF[] = { "ignition is off" };
        static constexpr e_uint8 IGN_OFF_N = sizeof(IGN_OFF)/sizeof(IGN_OFF[0]);

        static constexpr e_string LOCK_ON[] = { "the engine is locked right now" };
        static constexpr e_uint8 LOCK_ON_N = sizeof(LOCK_ON)/sizeof(LOCK_ON[0]);

        static constexpr e_string LOCK_OFF[] = { "the engine is unlocked" };
        static constexpr e_uint8 LOCK_OFF_N = sizeof(LOCK_OFF)/sizeof(LOCK_OFF[0]);

        static constexpr e_string FUEL_PREFIX[] = { "fuel level is at " };
        static constexpr e_uint8 FUEL_PREFIX_N = sizeof(FUEL_PREFIX)/sizeof(FUEL_PREFIX[0]);

        static constexpr e_string TEMP_PREFIX[] = { "engine temperature is at " };
        static constexpr e_uint8 TEMP_PREFIX_N = sizeof(TEMP_PREFIX)/sizeof(TEMP_PREFIX[0]);

        static constexpr e_string BATTERY_PREFIX[] = { "battery voltage is around " };
        static constexpr e_uint8 BATTERY_PREFIX_N = sizeof(BATTERY_PREFIX)/sizeof(BATTERY_PREFIX[0]);

        static constexpr e_string HORN_REPLIES[] = {
            "say the word and ill sound the horn",
            "just ask and ill honk for you"
        };
        static constexpr e_uint8 HORN_REPLIES_N = sizeof(HORN_REPLIES)/sizeof(HORN_REPLIES[0]);

        static constexpr e_string LIGHT_REPLIES[] = {
            "the headlamp can be switched on whenever you need it",
            "just say turn on the lights and ill handle it"
        };
        static constexpr e_uint8 LIGHT_REPLIES_N = sizeof(LIGHT_REPLIES)/sizeof(LIGHT_REPLIES[0]);

        static constexpr e_string SIGNAL_REPLIES[] = {
            "let me know left or right and ill signal for you",
            "just tell me which way youre turning"
        };
        static constexpr e_uint8 SIGNAL_REPLIES_N = sizeof(SIGNAL_REPLIES)/sizeof(SIGNAL_REPLIES[0]);

        static constexpr e_string SHOCK_REPLIES[] = {
            "the shock sensor is keeping an eye on the road for you",
            "ill let you know if i feel anything unusual"
        };
        static constexpr e_uint8 SHOCK_REPLIES_N = sizeof(SHOCK_REPLIES)/sizeof(SHOCK_REPLIES[0]);

        static constexpr e_string STATUS_REPLIES[] = {
            "ask about fuel temp battery engine or bluetooth",
            "i can check any system just ask"
        };
        static constexpr e_uint8 STATUS_REPLIES_N = sizeof(STATUS_REPLIES)/sizeof(STATUS_REPLIES[0]);

        static constexpr e_string WELLBEING_REPLIES[] = {
            "im doing great thanks for asking how about you",
            "running smoothly how are you doing",
            "all systems good how about yourself"
        };
        static constexpr e_uint8 WELLBEING_REPLIES_N = sizeof(WELLBEING_REPLIES)/sizeof(WELLBEING_REPLIES[0]);

        static constexpr e_string GENERIC_REPLIES[] = {
            "im listening go ahead",
            "tell me more im here",
            "got it whats next",
            "interesting tell me more about that",
            "im here whenever you need me",
            "okay im following along"
        };
        static constexpr e_uint8 GENERIC_REPLIES_N = sizeof(GENERIC_REPLIES)/sizeof(GENERIC_REPLIES[0]);

        static constexpr e_string GREET_FALLBACK[] = { "im here say something whenever youre ready" };
        static constexpr e_uint8 GREET_FALLBACK_N = sizeof(GREET_FALLBACK)/sizeof(GREET_FALLBACK[0]);
    };

    //conversation handler called from AudioInput thread
    class ConversationHandler : public AudioInputHandler {
        e_boolean onData(e_uint8* data,e_uint16 len) const override {
            Conversation* cnv = Conversation::instance;
            if(cnv == nullptr || !cnv->active) return false;

            //lets dump our raw recorded buffer to the udp port
            cnUdp.beginPacket(IP,PORT);
            cnUdp.write(data,len);
            cnUdp.endPacket();

            //since this is async and runs on the record thread
            //we read the incoming udp packet here also and dump it 
            //to our i2s system
            e_uint32 ps = cnUdp.parsePacket();
            if(ps > 0){ //here we got a packet
                e_uint8 pkt[2048];
                ps = cnUdp.read(pkt,sizeof(pkt));
                if(ps > 0){
                    esp_err_t err = i2s_write(I2S_NUM_0,pkt,ps,&ps,pdMS_TO_TICKS(100));
                    if(err != ESP_OK){
                        cnv->stop();
                        return false;
                    }
                }

            }
            return true;
        }
    };

    ConversationHandler cnh;

    //execute conversation related instructions
    e_boolean Conversation::execute(Command& com){
        //check if the command belong to us
        if(com.target != DeviceID::CONVERSATION) return false;
        switch (com.action){
            case CommandAction::START_REAL : start(ConversationMode::REAL,com); return true;
            case CommandAction::START_AI : start(ConversationMode::AI,com); return true;
            case CommandAction::OFF : stop(); return true;
            case CommandAction::PLAY : reply(com); return true;
            default: break;
        }
        //probably a query command we store the result in result.size here
        CommandResult r;
        r.id = com.id;
        r.sender = com.sender;

        if(com.action == CommandAction::QUERY_ON){
            r.size = active ? 1 : 0;
        } else if(com.action == CommandAction::QUERY_OFF){
            r.size = active ? 0 : 1;
        }
        //we only support on and off for conversatio for now
        sendResult(r);
        return true;
        
    }

    e_boolean Conversation::checkCondition(Condition& con){
        //check if this condition is ours
        if(con.target != DeviceID::CONVERSATION) return false;
        switch(con.action){
            case CommandAction::QUERY_ON : return active;
            case CommandAction::QUERY_OFF : return !(active);
            default: return false;
        }
    }

    //start the conversation engine
    void Conversation::start(ConversationMode m,Command& com){
        if(active || com.size < 4) return;
        //check if bluetooth is active and turn it off
        Bluetooth* bt = Bluetooth::instance;
        if(bt != nullptr && bt->on) bt->stop();

        //check the mode we are running on
        if(m == ConversationMode::REAL){//real mode
            //get the play configuration supplied
            //NOTE: command data[0 and 1] holds the requested sample rate
            //command data[2] holds the requested bit-depth
            //command data[3] holds the requested channel
            //lets fetch those 
            
            audioConfig cfg;

            cfg.sampleRate = (e_uint16) ((com.data[0] << 8) | (com.data[1] & 0xff));
            cfg.sampleSize = com.data[2];
            cfg.channel = com.data[3] == 1 ? AUDIO_CHANNEL::MONO : AUDIO_CHANNEL::STEREO;

            //get the audio system from tts and reconfigure it with the given configuration
            EcclesTTS::pause();
            EcclesTTS::getOutput()->reConfigure(cfg,AUDIO_MODE::I2S_ADC);

            /*
                we run into a critical decision point: should we record using the exact configuration
                or create a different configuration all together for recording and give the caller our 
                buffer configuration through command result, anyway for now we will keep both template
                we will create our own record config and use the provided config to fill it and still 
                signal the user the config we are recording with incase we decided to change this later
            */
            audioConfig recordCfg;
            recordCfg.sampleSize = cfg.sampleSize;
            recordCfg.sampleRate = cfg.sampleRate;
            recordCfg.channel = cfg.channel;

            //signal the caller our record config
            CommandResult r;
            r.id = com.id;
            r.sender = com.sender;
            r.size = 4;
            e_char dt[4];
            dt[0] = (e_uint8) ((recordCfg.sampleRate >> 8) & 0xff);
            dt[1] = (e_uint8) (recordCfg.sampleRate & 0xff);
            dt[2] = recordCfg.sampleSize;
            dt[3] = recordCfg.channel == AUDIO_CHANNEL::MONO ? 1 : 2;
            r.data = dt;
            sendResult(r);

            //start our record thread engine
            if(!recorder.start(&cnh,recordCfg,2048,AUDIO_INPUT::I2S_ADC)){
                ECCLES_LOG_LINE("failed to start conversation");
                EcclesTTS::reset();
                EcclesTTS::speak("conversation could not start please try again");
                return;
            }

            //if we got here we are running lets set the flag,start udp and assign ourselve
            active = true;
            Conversation::instance = this;
            cnUdp.begin(PORT); //conversation real mode runs on udp so we start it here
            return;
        }
    }

    /*
        we previously wanted to use udp for both REAL and AI conversation
        but we found out that using udp for AI is inefficient since this deals
        with raw texts,we decided to use raw command objects for AI communications
        hence this function is called whenever AI communication is needed
        NOTE: ai communication here is limited and only support short speechs
        thats why we have a mode called reply_raw which gives the AI directly what 
        to say instead of depending on it to generate the reply

        com.data is the heard phrase,not guaranteed null terminated by the sender.
        RAW_REPLY speaks it as is,otherwise ConversationAI builds a reply from it.
    */
    void Conversation::reply(Command& com){
        if(com.data == nullptr || com.size == 0) return;

        e_char heard[96];
        e_uint16 n = com.size < (sizeof(heard) - 1) ? com.size : (sizeof(heard) - 1);
        memcpy(heard,com.data,n);
        heard[n] = '\0';

        if(com.dataType == RAW_REPLY){
            EcclesTTS::speak(heard);
            return;
        }

        e_char said[ConversationAI::REPLY_BUF_SIZE];
        ConversationAI::reply(heard,said);
        EcclesTTS::speak(said);
    }

    //stop the conversation engine
    void Conversation::stop(){
        if(!active) return;
        
        //stop the record thread
        recorder.stop();

        //reconfigure the tts engine
        EcclesTTS::reset();

        //update flags
        active = false;

        //stop the udp
        cnUdp.stop();

        EcclesTTS::speak("conversation has ended take care");
    }

    //check if conversation is active
    e_boolean Conversation::isActive(){
        return active;
    }
};
