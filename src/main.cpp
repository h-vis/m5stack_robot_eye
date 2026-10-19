#include <Arduino.h>
#include <M5Unified.h>
#include <M5Chain.h>
#include <atomic>
#include "Config.h"
#include "Eyes.h"

Chain chain;
std::atomic<uint32_t> audioRms{0}, audioUpdated{0};
bool micReady=false, chainReady=false;
uint8_t encoderId=0, eyeId[2]={0,0};
int colorIndex=0, expression=0;
bool enabled=true, encoderPrimed=false;
int16_t previousEncoder=0;
int encoderRemainder=0;
eyes::Button button;
eyes::Voice voice;
uint32_t lastScan=0,lastPoll=0,lastFrame=0,lastLog=0,nextBlink=0;
uint32_t blinkStart=0;
bool blinking=false;
uint32_t nextLook=0, lookStart=0;
int gaze=0;
uint8_t consecutiveFailures=0;

// Audio capture runs independently of UART waits and display refreshes.
void audioTask(void*) {
 static int16_t samples[config::SAMPLES];
 for(;;) {
   if(M5.Mic.record(samples,config::SAMPLES,config::SAMPLE_RATE)) {
     while(M5.Mic.isRecording()) vTaskDelay(1);
     double mean=0, power=0;
     for(auto v:samples) mean+=v;
     mean/=config::SAMPLES;
     for(auto v:samples) {double ac=v-mean;power+=ac*ac;}
     audioRms.store(uint32_t(sqrt(power/config::SAMPLES)));
     audioUpdated.store(millis());
   } else vTaskDelay(pdMS_TO_TICKS(10));
 }
}

bool scanChain() {
 uint16_t count=0;
 if(chain.getDeviceNum(&count,120)!=CHAIN_OK || count!=3) {
   Serial.printf("Chain: expected 3 devices, detected %u. Check IN/OUT and cable.\n",count);
   return false;
 }
 device_info_t items[3]{};
 device_list_t list{}; list.count=3; list.devices=items;
 if(!chain.getDeviceList(&list,100)) return false;
 encoderId=0; uint8_t n=0;
 for(auto& d:items) {
   Serial.printf("Chain ID %u type 0x%02x\n",d.id,unsigned(d.device_type));
   if(d.device_type==CHAIN_ENCODER_TYPE_CODE) encoderId=d.id;
   if(d.device_type==CHAIN_RGB_TYPE_CODE && n<2) eyeId[n++]=d.id;
 }
 if(!encoderId || n!=2) {Serial.println("Expected U207 + U218 + U218.");return false;}
 if(config::SWAP_EYES) std::swap(eyeId[0],eyeId[1]);
 uint8_t ok=0;
 // Poll raw button state; do not enable asynchronous button event reports.
 if(chain.setEncoderButtonMode(encoderId,CHAIN_BUTTON_NONE_REPORT_MODE,&ok)!=CHAIN_OK || !ok) return false;
 for(int i=0;i<2;++i) {
   if(chain.setRGBMode(eyeId[i],RGB_PIXEL_MODE,&ok)!=CHAIN_OK || !ok) return false;
   auto rotation=rgb_rotation_t(i==0?config::LEFT_ROTATION:config::RIGHT_ROTATION);
   if(chain.setRGBRotation(eyeId[i],rotation,&ok)!=CHAIN_OK || !ok) return false;
   if(chain.setRGBBrightness(eyeId[i],config::BRIGHTNESS_LIMIT,&ok)!=CHAIN_OK || !ok) return false;
 }
 encoderPrimed=false;encoderRemainder=0;button=eyes::Button{};
 consecutiveFailures=0;
 Serial.printf("Ready: encoder=%u left=%u right=%u\n",encoderId,eyeId[0],eyeId[1]);
 return true;
}

void startBlink(uint32_t now) {blinkStart=now;blinking=true;}
void pollControl(uint32_t now) {
 int16_t value=0; uint8_t down=0;
 if(chain.getEncoderValue(encoderId,&value,25)==CHAIN_OK) {
   if(encoderPrimed) {
     // Unsigned subtraction keeps the int16 counter rollover a one-step change.
     uint16_t rawDelta=uint16_t(value)-uint16_t(previousEncoder);
     int delta=rawDelta<=32767?int(rawDelta):int(rawDelta)-65536;
     if(abs(delta)<128) {
       encoderRemainder+=delta*config::ENCODER_DIRECTION;
       int steps=encoderRemainder/config::ENCODER_COUNTS_PER_COLOR;
       encoderRemainder-=steps*config::ENCODER_COUNTS_PER_COLOR;
       colorIndex=eyes::wrap(colorIndex+steps,8);
     }
   }
   previousEncoder=value;encoderPrimed=true;
 }
 if(chain.getEncoderButtonStatus(encoderId,&down,25)==CHAIN_OK) {
   int event=button.update(down!=0,now,config::LONG_PRESS_MS);
   if(event==1) {expression=(expression+1)%6;startBlink(now);}
   if(event==2) enabled=!enabled;
 }
}

void drawEyes(uint32_t now) {
 // Both eyes glance in the same direction, then return to center.
 if(voice.detected || !enabled) {
   gaze=0;nextLook=now+random(4000,8000);
 } else if(gaze!=0) {
   if(uint32_t(now-lookStart)>=900) {
     gaze=0;nextLook=now+random(4000,8000);
   }
 } else if(!blinking && int32_t(now-nextLook)>=0) {
   gaze=random(2)==0?-1:1;lookStart=now;
 }
 if(!blinking && !voice.detected && int32_t(now-nextBlink)>=0) startBlink(now);
 float openness=1.f;
 if(blinking) {
   uint32_t elapsed=now-blinkStart;
   if(elapsed>=180) {blinking=false;nextBlink=now+random(3000,6500);}
   else openness=fabsf(float(elapsed)-90.f)/90.f;
 }
 float intensity=.45f+.45f*voice.envelope;
 bool success=true;
 for(int i=0;i<2;++i) {
   uint16_t frame[64];uint8_t ok=0;
   int visibleExpression=voice.detected?0:expression;
   eyes::render(frame,visibleExpression,colorIndex,i==1,voice.large,intensity,openness,enabled,gaze);
   if(chain.setRGBBufferRefresh(eyeId[i],frame,&ok,35)!=CHAIN_OK || !ok) success=false;
 }
 if(success) consecutiveFailures=0;
 else if(++consecutiveFailures>=5) {chainReady=false;lastScan=now;Serial.println("Chain lost; retrying.");}
}

void setup() {
 auto cfg=M5.config();
 cfg.fallback_board=m5::board_t::board_M5AtomVoiceS3R;
 cfg.internal_mic=true; cfg.internal_spk=true; cfg.internal_imu=false;
 M5.begin(cfg);
 Serial.begin(115200);
 M5.Speaker.end(); // No playback; microphone owns the audio interface.
 auto mic=M5.Mic.config();mic.noise_filter_level=8;M5.Mic.config(mic);
 micReady=M5.Mic.begin();
 if(micReady) {
   micReady=xTaskCreatePinnedToCore(audioTask,"audio",4096,nullptr,2,nullptr,0)==pdPASS;
 }
 Serial.printf("Robot Eyes v0.2 | board=%d | PSRAM=%u | microphone=%s\n",
               int(M5.getBoard()),ESP.getPsramSize(),micReady?"OK":"FAILED");
 chain.begin(&Serial2,115200,config::RX_PIN,config::TX_PIN);
 delay(500);chainReady=scanChain();lastScan=millis();
 nextBlink=millis()+4000;
 nextLook=millis()+random(4000,8000);
}
void loop() {
 uint32_t now=millis();M5.update();
 // A stale/failed audio stream must not leave the eyes permanently enlarged.
 float rms=micReady && uint32_t(now-audioUpdated.load())<500?float(audioRms.load()):0.f;
 voice.update(rms,now,config::NOISE_RATIO,config::MIN_RMS);
 if(!chainReady && uint32_t(now-lastScan)>=3000) {lastScan=now;chainReady=scanChain();}
 if(chainReady && uint32_t(now-lastPoll)>=25) {lastPoll=now;pollControl(now);}
 if(chainReady && uint32_t(now-lastFrame)>=config::FRAME_MS) {lastFrame=now;drawEyes(now);}
 if(uint32_t(now-lastLog)>=1000) {
   lastLog=now;Serial.printf("rms=%.0f noise=%.0f level=%.2f active=%d color=%d face=%d enabled=%d chain=%d\n",
     rms,voice.noise,voice.envelope,voice.detected,colorIndex,expression,enabled,chainReady);
 }
 delay(1);
}
