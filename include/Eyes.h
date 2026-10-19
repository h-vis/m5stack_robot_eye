#pragma once
#include <stdint.h>
#include <math.h>
#include <algorithm>
namespace eyes {
struct Color { uint8_t r,g,b; };
constexpr Color COLORS[8] = {{0,220,255},{25,65,255},{0,255,65},{255,220,0},
                            {255,90,0},{255,0,0},{255,45,145},{170,25,255}};
// One byte per row, bit7 is the leftmost pixel. Right angry/sad eye is mirrored.
constexpr uint8_t MASKS[6][8] = {
 {0x00,0x3c,0x7e,0x66,0x66,0x7e,0x3c,0x00},
 {0x00,0x00,0x3c,0x7e,0xc3,0xc3,0x00,0x00},
 {0x00,0x40,0x60,0x78,0x7e,0x7e,0x3c,0x00},
 {0x00,0x02,0x06,0x1e,0x7e,0x66,0x3c,0x00},
 {0x3c,0x7e,0xc3,0xc3,0xc3,0xc3,0x7e,0x3c},
 {0x00,0x66,0xff,0xff,0x7e,0x3c,0x18,0x00}
};
constexpr uint8_t BIG_NORMAL[8] = {0x3c,0x7e,0xe7,0xc3,0xc3,0xe7,0x7e,0x3c};
constexpr uint8_t BIG_SURPRISE[8] = {0x7e,0xc3,0x81,0x81,0x81,0x81,0xc3,0x7e};
inline int wrap(int value, int count) { return (value % count + count) % count; }
inline uint16_t rgb565(Color c, float intensity) {
 auto r=uint8_t(c.r*intensity), g=uint8_t(c.g*intensity), b=uint8_t(c.b*intensity);
 return ((r&0xf8)<<8)|((g&0xfc)<<3)|(b>>3);
}
inline void render(uint16_t (&out)[64], int expression, int color, bool right,
                   bool large, float intensity, float openness, bool enabled) {
 std::fill(out,out+64,0);
 if (!enabled) return;
 expression=wrap(expression,6); color=wrap(color,8);
 const uint8_t* mask=MASKS[expression];
 if(large && expression==0) mask=BIG_NORMAL;
 if(large && expression==4) mask=BIG_SURPRISE;
 const uint16_t ink=rgb565(COLORS[color],std::max(0.f,std::min(1.f,intensity)));
 for(int y=0;y<8;++y) for(int x=0;x<8;++x) {
   if(!(mask[y] & (0x80>>x))) continue;
   int dx=right && (expression==2||expression==3) ? 7-x:x;
   int dy=int(lroundf(3.5f+(y-3.5f)*openness));
   dy=std::max(0,std::min(7,dy)); out[dy*8+dx]=ink;
 }
}
struct Button {
 bool raw=false, stable=false, longSent=false;
 uint32_t changed=0, pressed=0;
 // 1=short release, 2=long hold. Long hold never causes a short release.
 int update(bool down,uint32_t now,uint32_t hold=1500) {
   if(down!=raw) { raw=down; changed=now; }
   int event=0;
   if(raw!=stable && uint32_t(now-changed)>=30) {
     stable=raw;
     if(stable) { pressed=now; longSent=false; }
     else if(!longSent) event=1;
   }
   if(stable && !longSent && uint32_t(now-pressed)>=hold) {longSent=true;event=2;}
   return event;
 }
};
struct Voice {
 float noise=80, envelope=0;
 uint32_t start=0,last=0,held=0;
 bool initialized=false, detected=false, large=false;
 void update(float rms,uint32_t now,float ratio=2.8f,float minimum=250.f) {
   if(!initialized) {initialized=true;start=last=held=now;noise=std::max(20.f,rms);}
   float dt=std::min(0.2f,uint32_t(now-last)*.001f); last=now;
   bool calibrating=uint32_t(now-start)<1200;
   float threshold=std::max(minimum,noise*ratio);
   bool active=!calibrating && rms>threshold;
   if(calibrating || !active) {
     float tau=calibrating?.3f:(rms<noise?1.f:8.f);
     noise+=(rms-noise)*(1.f-expf(-dt/tau)); noise=std::max(20.f,noise);
   }
   if(active) {detected=true;held=now;}
   else if(uint32_t(now-held)>300) detected=false;
   float target=active?std::min(1.f,.15f+(rms-threshold)/std::max(1000.f,threshold*3.f)):0.f;
   envelope+=(target-envelope)*(1.f-expf(-dt/(target>envelope?.06f:.30f)));
   if(envelope>.60f) large=true;
   if(envelope<.40f) large=false;
 }
};
}
