//base implementation of audio system

#include "Audio.h"
#include "RuntimeMemory.h"
#include "Pins.h"
#include "esp_task_wdt.h"
#include <driver/i2s.h>

ECCLES_API {

    //the i2s player used to play audio in i2s mode
    namespace I2S
    {

        e_boolean port0Installed = false,port1Installed = false; //we keep track of ports that are installed
        i2sConfig iConfig = {};
        audioConfig config;

        void install(audioConfig cf,AUDIO_MODE md){
            //setting up the i2s configuration

            config = cf;
            iConfig.rate = config.sampleRate;
            iConfig.depth = config.sampleSize;
            iConfig.stereo = config.channel == AUDIO_CHANNEL::STEREO;

            e_boolean* currentPort = &port0Installed; //used to track the port we are likely installing
            //currently only ECCLES_INPUT uses port1

            I2S_PLAY_MODE m = ECCLES_INTERNAL;
            switch (md)
            {
                case AUDIO_MODE::I2S : m = ECCLES_EXTERNAL; currentPort = &port1Installed; break;
                case AUDIO_MODE::I2S_DAC : m = ECCLES_INTERNAL; break;
                case AUDIO_MODE::I2S_ADC : m = ECCLES_INPUT_INTERNAL; m = (I2S_PLAY_MODE) (m | ECCLES_INTERNAL); break; //audio should be playing while record is going on
                case AUDIO_MODE::I2S_IN : m = ECCLES_INPUT; m = (I2S_PLAY_MODE) (m | ECCLES_EXTERNAL); currentPort = &port1Installed; break;
                default: break;
            };
            iConfig.mode = m;

            *currentPort = Pins::initializeAudioPins(iConfig);
            if(!(*currentPort)){
                ECCLES_LOG_LINE("i2s driver operation failed");
            } else {
                ECCLES_LOG_LINE("i2s driver operation successful");
            }
            if(iConfig.exit){
                 *currentPort = !(*currentPort); //invert the result in exit mode to turn port init false
                iConfig.exit = false;
            }

        }

        void uninstall(AUDIO_MODE m){
            if(!(port0Installed || port1Installed)) return; //we were never installed in the first place
            iConfig.exit = true;
            install(config,m);
        }


        void playBuffer(e_uint8* buffer,e_uint32 size,AUDIO_MODE md){
            //here we write to i2s system

            //check if the mode is really i2s
            if((md == AUDIO_MODE::NONE)) return; //bad mode
            //check if system is actually installed
            if(!port0Installed && !port1Installed) return; //no port must call install first
            i2s_port_t port = I2S_NUM_0;
            if((md == AUDIO_MODE::I2S) || (md == AUDIO_MODE::I2S_IN)) port = I2S_NUM_1;

            e_uint32 wr = 0;
            ECCLES_LOG_LINE("writing to the i2s play driver");
            i2s_write(port,buffer,size,&wr,pdMS_TO_TICKS(100)); //we wait here 100 ms untill the requested audio is actually written since its freed after this function
            ECCLES_LOG("audio player writes len: ");
            ECCLES_LOG_LINE(wr);
            if(wr != size){
                ECCLES_LOG_LINE("i2s written bytes not equal to requested");
            }
        }

        void play(FileSystem* f,audioClip c,AUDIO_MODE md){
            //play i2s from file

            //check if we have a valid clip and file
            if(f == nullptr){
                ECCLES_LOG_LINE("attempting to play an audio clip on null file or clip exiting...");
                return;
            }
            //create a static buffer for reading file content
             e_uint8 buffer[FILE_MAX_CHUNK_BUFFER]; //buffer size defined in FileSystem.h
             e_uint32 rdi = 0; //used to keep track of how many bytes we have written
             
             //we needed to get the underlying file to be sure of how many bytes are actually read
             File& fl = f->getFile();
             fl.seek(c.offset);

             while(c.size >= (rdi + FILE_MAX_CHUNK_BUFFER)){
                e_uint16 rd = fl.read(buffer,FILE_MAX_CHUNK_BUFFER);
                playBuffer(buffer,rd,md);
                rdi+= rd;

               // eccles_wait(1); //not sure if this idea is good 1 millis
             }
             //if we got here the remaining clip size is bigger than the buffer size
             //we find how many bytes is remaining on the clip and read them
             //we do this because the clip may not be perfectly align with 512 bytes and
             //to avoid losing some clip data we find whats left

             //we first check if the remaining clip is actually bigger than our file position
             if(c.size > rdi){
                //we subtract the clip position from the size to get the remaining bytes
                e_uint16 rm = c.size - rdi;
                //we must make sure we don't read past the clip bounds,but the subtraction already
                //give us confidence we are not doing that
                rm = fl.read(buffer,rm);
                playBuffer(buffer,rm,md);
             }
        }

        //reads raw pcm and call the audio handler to process it,we intentionally want to 
        //let the user define the buffer size but this will lead to dynamic allocation which we
        //have been avoiding so we default to FILE_MAX_CHUNK_BUFFER we still leave the buffSize
        //in case we change mind later,NOTE: the buffer is not valid after the audioHandler returns
        //and this function returns whatever the audio handler returns
        e_boolean recordAndCall(AudioInputHandler* h,e_uint16 buffSize/*researved*/,AUDIO_MODE m){
            if(m == AUDIO_MODE::I2S && !port1Installed) {
                ECCLES_LOG_LINE("play attempted but port not yet installed aborting");
                return true; //we do in case the audio was initialized late
            } else if(m == AUDIO_MODE::I2S_ADC && !port0Installed) {
                ECCLES_LOG_LINE("play attempted but port not yet installed aborting");
                return true;
            }
            //creating the audio buffer
            e_uint8 data[AUDIO_RECORD_MAX_BUFFER];//data[FILE_MAX_CHUNK_BUFFER]; we found out this glitches its too small for android processing
            e_uint32 rb; //holds the actual bytes read,we can't read more than 65,536 bytes on poor esp

            //detecting the port used
            i2s_port_t port = m == AUDIO_MODE::I2S_ADC ? I2S_NUM_0 : I2S_NUM_1;

            //reading from the i2s adm buffer
            ECCLES_LOG_LINE("reading from i2s buffer");
            i2s_read(port,data,AUDIO_RECORD_MAX_BUFFER,&rb,pdMS_TO_TICKS(100)); //wait until we read the full bytes

            //call the handler to process the buffer
            ECCLES_LOG_LINE("calling the event handler");
            return h->onData(data,rb); //we supply the actual bytes read incase of partial reads though we wait to read all
        }

        
    };

    /*  ADDS TOO MUCH LATENCY TO A SYSTEM ALREADY DOING MUCH
    WE FOUND OUT THAT THERE IS NO WAY TO PLAY EVEN 8KHZ SAMPLE WITHOUT
    HUMMING THE CPU EVEN WITH TIMERS SO WE USE I2S FOR EVERYTHING AUDIO

    //used for playing audio in DAC mode
    namespace DAC {
        void play(FileSystem* f,audioClip c,audioConfig config){
            //we get the underlying file
            File& fl = f->getFile();

            fl.seek(c->offset); //the starting point for the clip

            //to save RAM we read the audio clip byte by byte
            while(fl.available() && fl.position() < (c->offset + c->size)){
                //esp DAC is only 8bit depth and we only support mono here
                e_uint16 sample = 0;
                if(config->sampleSize == 16){
                    //16 bit samples read 2 discard the lower bite and add little noise
                    sample = (fl.read() << 8 | (fl.read() & 0xff));
                } else if(config->sampleSize == 8){
                    sample = fl.read()<<8; //we shift this to 8 because if the sample is 16 we still need to shift it and we do this here so that whether the sample is 8 or 16 we shift right to 8 during write
                } else {
                    //we only support 8 and 16 bit samples
                    ECCLES_LOG_LINE("unsupported sample size we currently support only 8 and 16 bit");
                }
                //we currently use one pin for I2S data and DAC output because we ran out of pins
                //we must call I2S uninstall before calling this function or we encounter a mess here
                //we are still looking for better ways to handle this

                eccles_writeDAC(Pins::I2S_DATA_PIN,((sample >> 8) + 128/*a little noise)); //discard lower bits

                eccles_waitMicro(1000000 / config->sampleRate); //this timing will not be perfect but we are avoiding using hardware timer for this for a reason
            }
        }

        void playBuffer(e_uint8* buffer,e_uint32 size,audioConfig config){
            //DAC buffer player driver
            for(e_uint32 i = 0;i<size;i++){
                e_uint16 sample = 0;
                if(config->sampleSize == 16){
                    sample = (buffer[i] << 8) | buffer[++i] & 0xff;
                } else if(config->sampleSize == 8){
                    sample = buffer[i];
                } else {
                    ECCLES_LOG_LINE("unsupported sample size supplied we currently support 8 and 16");
                    return; //nothing else to do
                }
                eccles_writeDAC(Pins::I2S_DATA_PIN,(sample >> 8)+ 128);
            }
        }
    };

    //adc engine used for recording 
    namespace ADC {
        e_uint16 record(e_uint8* buffer,e_uint16 len,audioConfig cf){
            //we are coming back to this still in development
            return 0; //still in development
        }
    }; */

    //the main function that runs the audio playing task,this calls our drivers
    //we won't add watchdog here because this task is idle until audio arrives
    void playThread(void* arg){
        //we retrieve our message queue handle 
        QueueHandle_t handle = (QueueHandle_t) arg;

        if(handle == nullptr){
            //how can this be
            ECCLES_LOG_LINE("audio task created with null message handle");
            eccles_taskDelete();
            return; //we can't do anything without the message handle
        }
        audioTask t; //the task we are looking for
        AUDIO_MODE prevMode = AUDIO_MODE::NONE;

        while(1){
            //this task remains idle 0 cpu usage until audio task is received and
            //thats actually the reason we use xQueue instead of pools

            eccles_readMsg(handle,&t); //this sleeps until a message is present

            //we use the same pin for I2S and DAC and to avoid conflict we unistall I2S
            //whenever we are requested to use DAC and install it otherwise
            if(t.mode != prevMode){
                //we only do this whenever we detected a mode change and not in every clip
                //NOTE: mode change costs cpu and can introduce audio lag,ensure to use a consistent
                //mode and avoid rapid and frequent change of mode
                
                //if(t.mode == AUDIO_MODE::DAC){
                    //we must uninstall I2s for DAC operations
                  //  if(prevMode != AUDIO_MODE::NONE) I2S::uninstall(prevMode);
                 if(t.mode != AUDIO_MODE::NONE) {
                    //every other mode deals with i2s
                    if(prevMode != AUDIO_MODE::NONE) I2S::uninstall(prevMode); //uninstall the previous i2s mode
                    I2S::install(t.config,t.mode);
                }
            }
            prevMode = t.mode;

            //parsing the task
            if(t.type == AUDIO_TYPE::CLIP){
                    //case AUDIO_MODE::DAC : DAC::play(t.file,t.clip,t.config); break;
                I2S::play(t.file,t.clip,t.mode);
                
            } else if(t.type == AUDIO_TYPE::BUFFER){
                    //case AUDIO_MODE::DAC : DAC::playBuffer(t.buffer,t.size,t.config); break;
                I2S::playBuffer(t.buffer,t.size,t.mode);

                //our whole intention is to avoid heap,but we have no other way to handle this apart from heap
                //so if the buffer is supplied it must be created from heap so we free it here after playing
                e_free(t.buffer);
                
            } else if(t.type == AUDIO_TYPE::SHUTDOWN){
                break;
            }
            eccles_wait(AudioOutput::delay + t.clip.delay); //delay playbacks on half a second
        }
        //if we get here we are actually ask to quit so we do that safely
        eccles_taskDelete();

        //we delete the message queue here
        eccles_deleteMsgQueue(handle);
    }

    //audio record engine
    //must not block,watchdog resets if hangs for 2 sec
    void recordThread(void* arg){
        //initializing the watchdog,record threads should not hang
        eccles_wdtInclude(NULL);
        //get the underlying audio input task
        AudioInputTask* t = (AudioInputTask*) arg;
        if(!t){
            ECCLES_LOG_LINE("attempting to record audio on a null audio input task exiting...");
            eccles_taskDelete();
            return;
        }
        //converting the audio input mode to i2s audio mode
        AUDIO_MODE m = t->inputMode == AUDIO_INPUT::I2S ? AUDIO_MODE::I2S : AUDIO_MODE::I2S_ADC;

        //installing the i2s driver
        //I2S::install(t->config,m); causes i2s driver conflit since audio is already installed we use its driver here

        //starting the record loop
        while(AudioInput::run){
            if(!I2S::recordAndCall(t->handler,t->buffSize,m)) break;
            eccles_wait(1); //a little delay to yield the cpu to other tasks

            //inform watch dog we are still alive
            eccles_wdtReset();
        }
        //if we got here we are done recording

        //uninstall watchdog
        eccles_deleteWDT(NULL);
        //remove ourselve from the task queue
        eccles_taskDelete();
    }

    //the delay for clip playback
    e_uint16 AudioOutput::delay = AUDIO_OUTPUT_DEFAULT_DELAY;

    //initializing audio clip buffer
   // audioClip AudioOutput::clips[MAX_CLIP_SIZE] = {};

    //audio ouput constructor must be provided with audioconfig data
    AudioOutput::AudioOutput() {}

    //starting the Audio Engine must be called before any play
    void AudioOutput::start(audioConfig config){
        //starting our audio queue
        this->config = config;
        msgQueue = eccles_createMsgQueue(10 /* our queue holds max 10 audioTask*/,sizeof(audioTask) /*this is the actual object we are sending*/);

        //here we start the audio play task
        eccles_taskInit(playThread,"audioThread",6000,msgQueue);

        //initializing the i2s driver for i2s plays
        //I2S::install(config); not here
    }

    AudioOutput::~AudioOutput(){
        audioTask t;
        t.type = AUDIO_TYPE::SHUTDOWN;
        play(t);

        //now we delete our message queue
        //eccles_deleteMsgQueue(msgQueue); too early
    }

    //loads the audio file data this does't load the entire data to RAM 
    //instead open the file containing the audio we are planning to play
    //NOTE: all audio clip values must point to valid address in this file
    e_boolean AudioOutput::load(e_string path){
        //ask the fileSystem to open this file
        return (audioFile.load(path,"r") == FileStatus::SUCCESSFUL);
    }

    //returns the underlying audio buffer for filling,NOTE: not to be confused
    //by the name buffer audioBuffer is not the actual audio binaries but just the
    //offset and size in the loaded file where the actual audio is stored
    /*audioClip* AudioOutput::getBuffer(){
        //checks if the buffer is currently in use in that case we can't return a buffer 
        //we are still using
        if(inUse) return nullptr;

        //the size of this buffer is the MAX_BUFFER_SIZE so you query that to get the buffer size
        //we mark this buffer as inUse so we don't accidentally hand it over to another client
        inUse = true; //only release buffer can change this
        return clips;
    } currently using queue 

    //releases the audio clip buffer,this must be called before any other client can access the buffer
    void AudioOutput::releaseBuffer(){
        //here we reset the entire buffer
        clips = {};
        inUse = false;
    } currently using queue */

    //checks if the audio system is currently handling audio
    e_boolean AudioOutput::isPlaying(){
        //if there is any message in the queue we are probably playing music
        return (eccles_hasMsg(msgQueue));
    }

    //this plays the actual audio binary using the mode provided,NOTE: audio is played in a different task
    //this function hands over the audio buffer to a player to play them in a different tast and returns immediately
    void AudioOutput::play(audioClip& clip,AUDIO_MODE mode){
        if(!audioFile){
            ECCLES_LOG_LINE("attempting to play an audio clip without loading any audio file,please call AudioOutput::load before calling this");
            return;
        }
        audioTask t; 
        t.clip = clip;
        t.type = AUDIO_TYPE::CLIP;
        t.mode = mode;
        play(t);
        this->mode = mode;
    }

    //return the underlying audioConfig
    audioConfig AudioOutput::getConfig(){
        return config;
    }

    //reconfigure the audioSystem with the given config and the given mode
    e_boolean AudioOutput::reConfigure(audioConfig cfg,AUDIO_MODE m){
        //we needed to make sure no other audio is queued during reset
        //so we delete all pending messages here if any
        if(msgQueue == nullptr) return false;
        //eccles_clearMsg(msgQueue);

        ECCLES_LOG("reconfiguring audio pipeline with bitrate:");
        ECCLES_LOG(cfg.sampleRate);
        ECCLES_LOG(" bitDepth:");
        ECCLES_LOG(cfg.sampleSize);
        ECCLES_LOG(" channels:");
        ECCLES_LOG_LINE(cfg.channel == AUDIO_CHANNEL::MONO ? "mono" : "stereo");
        
        //we ask the i2s system to uninstall and reinstall our desired config
        I2S::uninstall(mode); //reconfigure faile
        I2S::install(cfg,m);

        //if we successfully installs update the mode incase of further reconfiguring
        mode = m;
        return true;

    }

    //plays the audio clip from audioTask
    void AudioOutput::play(audioTask& task){
        //check if audio is disabled
        if(globalState.AUDIBLE == OFF) return;
        //add the config to the task incase we forgot it
        task.config = config;

        //add the file handle
        if(task.type == AUDIO_TYPE::CLIP) task.file = &audioFile; //the audioFile must remain valid as long as the task is running
        //we hand over this task to the task queue
        eccles_sendMsg(msgQueue,&task);

    }

    //plays a binary buffer already loaded to RAM,currently we have no way of
    //storing the buffer globaly without wasting RAM space so we default to using malloc to
    //create this buffer, NOTE: the buffer is automatically freed after it is played
    void AudioOutput::playBuffer(e_uint8* buffer,e_uint32 size,AUDIO_MODE mode){
        audioTask t;
        t.type = AUDIO_TYPE::BUFFER;
        t.buffer = buffer;
        t.size = size;
        t.mode = mode;
        play(t);
        this->mode = mode; //needed so that reConfigure can unistall this mode
    }


    /* AUDIO RECORDING APIs*/

    volatile e_boolean AudioInput::run = false; //used by the record thread this should only be changed by AudioInput

    AudioInput::AudioInput(){}
    AudioInput::~AudioInput(){
        stop();
    }

    //force stop the audio record engine NOTE: audio record is automatically stopped when handler returns false
    void AudioInput::stop(){
        AudioInput::run = false;
    }

    //starts the audio record engine, returns true on success false otherwise
    //NOTE: audio engine runs async in a different task so this function should return immediately
    e_boolean AudioInput::start(AudioInputHandler* h,audioConfig config,e_uint16 bSize,AUDIO_INPUT m){
        //check if the handler is present we can't start recording if there is no handler
        //because there is nothing to handle the stream,waste of cpu and resources
        if(!h){
            ECCLES_LOG_LINE("attempt to start recording on a null handler exiting...");
            return false;
        }
        
        //creating the audio task we use static so the object is still valid in the record thread
        static AudioInputTask t = {
            .handler = h,
            .config = config,
            .inputMode = m,
            .buffSize = bSize
        };

        //setting the run flag this caused me serious runtime headache
        run = true;
        //starting the record thread engine
        eccles_taskInit(recordThread,"recordThread",6000,&t);
        return true;
    }
};