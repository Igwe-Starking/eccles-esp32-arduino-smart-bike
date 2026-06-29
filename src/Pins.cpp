//#include "driver/i2s_types_legacy.h"
//implementation of pins.h
#include "Pins.h"
#include<driver/i2s.h>
#include<esp_intr_alloc.h>

ECCLES_API {
  namespace Pins {

    //pins initialization state
    e_boolean isInitialized = false;
    
    //returns the hardware controlled pin name, only for debugging
    e_string getPinName(e_uint8 pin){
      switch(pin){

        //output pins
        case IGNITION_CONTROL_PIN: return "Ignition_control";
        case HEADLAMP_CONTROL_PIN: return "headlamp_control";
        case HORN_CONTROL_PIN: return "horn control";
        case LEFT_SIGNAL_CONTROL_PIN: return "left signal control";
        case RIGHT_SIGNAL_CONTROL_PIN: return "right signal control";
        case STARTER_CONTROL_PIN: return "starter control";

        //audio pins

        case I2S_CLOCK_PIN: return "i2s audio clock pin";
        case I2S_WORD_PIN: return "i2s audio word pin";
        case I2S_DATA_PIN: return "i2s audio data pin";

        //feedback pins
        
        case IGNITION_FB_PIN:return "ignition feedback pin";
        case HORN_FB_PIN:return "horn feedback pin";
        case HEADLAMP_FB_PIN:return "headlamp feedback pin";
        case STARTER_FB_PIN:return "starter feedback pin";
        case LEFT_SIGNAL_FB_PIN:return "left signal feedback pin";
        case RIGHT_SIGNAL_FB_PIN:return "right signal feedback pin";

        //analog pins

        case FUEL_GAUGE_PIN:return "fuel level pin";
        case TEMP_GAUGE_PIN:return "temperature guage pin";

        default: return "unknown_pin"; 
      }
    }

    void initializeIOPins(){
      ECCLES_LOG_LINE("initializing IO pins");

      //initializing the output pins

      for(const e_uint8 pin : OUTPUT_PINS){
        pinMode(pin,OUTPUT);
        #ifdef ECCLES_DEBUG
        ECCLES_LOG(getPinName(pin));
        ECCLES_LOG_LINE(" successfully set to output");
        #endif
      }

      //initializing the feedback pins

      for(const e_uint8 pin : FEEDBACK_PINS){
        pinMode(pin,INPUT);
        #ifdef ECCLES_DEBUG
        ECCLES_LOG(getPinName(pin));
        ECCLES_LOG_LINE(" successfully set to input");
        #endif
      }
    }

    //setup the i2s audio pins
    e_boolean initializeAudioPins(i2sConfig& config){

      i2s_port_t port = I2S_NUM_0;

      if(config.mode & ECCLES_INPUT_INTERNAL){
        port = I2S_NUM_0;
      }

      if(config.mode & ECCLES_INPUT){
        port = I2S_NUM_1;
      }

      if(config.mode & ECCLES_EXTERNAL){
        port = I2S_NUM_1;
      }

      if(config.exit){
        i2s_stop(port);
        i2s_driver_uninstall(port);
        return true;
      }

      e_uint8 channels = config.stereo ? 2 : 1;

      e_uint8 bytesPerSample;

      switch(config.depth){
        case 8:
          bytesPerSample = 1;
          break;

        case 16:
          bytesPerSample = 2;
          break;

        case 24:
          bytesPerSample = 3;
          break;

        case 32:
          bytesPerSample = 4;
          break;

        default:
          return false;
      }

      e_uint32 totalSamples = config.rate / 10;

      e_uint16 dmaLen = 512;

      if(totalSamples <= 1024){
        dmaLen = 128;//256;
      }

      if(totalSamples <= 512){
        dmaLen = 64;//128;
      }

      if(totalSamples <= 256){
        dmaLen = 32;// 64;
      }

      e_uint32 bytesPerBuf = (e_uint32)dmaLen * channels * bytesPerSample;

      while(bytesPerBuf > 4092 && dmaLen > 8){
        dmaLen >>= 1;
        bytesPerBuf = (e_uint32)dmaLen * channels * bytesPerSample;
      }

      e_uint16 dmaCount = totalSamples / dmaLen;

      if((totalSamples % dmaLen) != 0){
        dmaCount++;
      }

      if(dmaCount < 4){
        dmaCount = 4;
      }

      if(dmaCount > 24){
        dmaCount = 24;
      }

      i2s_mode_t m = I2S_MODE_MASTER;

      if(config.mode & ECCLES_INTERNAL){
        //requested to use internal 8bit DAC
        m = (i2s_mode_t) (m | I2S_MODE_TX);
        m = (i2s_mode_t) (m | I2S_MODE_DAC_BUILT_IN);
        ECCLES_LOG_LINE("internal tx and dac mode set");
      }
      if(config.mode & ECCLES_INPUT_INTERNAL){
        //requested to record
        m = (i2s_mode_t) (m | I2S_MODE_RX);
        m = (i2s_mode_t) (m | I2S_MODE_ADC_BUILT_IN);
      }
      if(config.mode & ECCLES_INPUT){
        m = (i2s_mode_t) (m | I2S_MODE_RX);
      }
      if(config.mode & ECCLES_EXTERNAL){
        m = (i2s_mode_t) (m | I2S_MODE_TX);
      }

      i2s_config_t iconfig = {

        .mode = m,

        .sample_rate = config.rate,

        .bits_per_sample =
            config.depth == 8  ? I2S_BITS_PER_SAMPLE_8BIT  :
            config.depth == 16 ? I2S_BITS_PER_SAMPLE_16BIT :
            config.depth == 24 ? I2S_BITS_PER_SAMPLE_24BIT :
                                 I2S_BITS_PER_SAMPLE_32BIT,

        .channel_format =
            config.stereo ?
            I2S_CHANNEL_FMT_RIGHT_LEFT :
            I2S_CHANNEL_FMT_ONLY_LEFT,

        .communication_format = I2S_COMM_FORMAT_I2S_LSB,

        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL2 | ESP_INTR_FLAG_IRAM,

        .dma_buf_count = 4,//dmaCount,

        .dma_buf_len = 64,//dmaLen,

        .use_apll = (config.mode & ECCLES_EXTERNAL) != 0,

        .tx_desc_auto_clear = true,

        .fixed_mclk = 0
      };

      esp_err_t res = i2s_driver_install(port,&iconfig,0,NULL);

      if(res != ESP_OK){
        return false;
      }

      i2s_pin_config_t pins = {
        .bck_io_num = I2S_CLOCK_PIN,
        .ws_io_num = I2S_WORD_PIN,
        .data_out_num = I2S_DATA_PIN,
        .data_in_num = MIC_PIN //micro phone pin for recording
      };

      if((config.mode & ECCLES_EXTERNAL) ||
         (config.mode & ECCLES_INPUT)){

        res = i2s_set_pin(port,&pins);

        if(res != ESP_OK){
          i2s_driver_uninstall(port);
          return false;
        }
      }

      if(config.mode & ECCLES_INPUT_INTERNAL){

        res = i2s_set_adc_mode(
            ADC_UNIT_1,
            ADC1_CHANNEL_0
        );

        if(res != ESP_OK){
          i2s_driver_uninstall(port);
          return false;
        }

        res = i2s_adc_enable(port);

        if(res != ESP_OK){
          i2s_driver_uninstall(port);
          return false;
        }
      }

      if(config.mode & ECCLES_INTERNAL){

        res = i2s_set_dac_mode(
            I2S_DAC_CHANNEL_LEFT_EN
        );

        if(res != ESP_OK){
          i2s_driver_uninstall(port);
          return false;
        }

        res = i2s_set_pin(port,NULL);

        if(res != ESP_OK){
          i2s_driver_uninstall(port);
          return false;
        }

        res = i2s_zero_dma_buffer(port);

        if(res != ESP_OK){
          i2s_driver_uninstall(port);
          return false;
        }
      }

      return true;
    }

    void initializeAll(){
      if(isInitialized) return;
      initializeIOPins();
      //initializeAudioPins(); done by I2S namespace
      isInitialized = true;
    }

    
  };
};