#include "Eyes.h"
#include <cassert>
#include <iostream>
int main() {
 // Contact bounce, short click, long hold with no extra click on release.
 eyes::Button b;
 assert(b.update(true,10)==0); assert(b.update(false,20)==0);
 assert(b.update(true,25)==0); assert(b.update(true,60)==0);
 assert(b.update(false,100)==0); assert(b.update(false,135)==1);
 assert(b.update(true,200)==0); assert(b.update(true,235)==0);
 assert(b.update(true,1735)==2); assert(b.update(true,1800)==0);
 assert(b.update(false,1900)==0); assert(b.update(false,1935)==0);
 eyes::Button rollover;
 rollover.update(true,0xfffffff0u); rollover.update(true,0x20);
 assert(rollover.update(true,0x620)==2);
 // Baseline noise must not trigger; a voice burst must attack, hold, and decay.
 eyes::Voice v;
 for(unsigned t=0;t<2000;t+=20) v.update(100,t);
 assert(!v.detected && v.envelope==0);
 for(unsigned t=2000;t<2400;t+=20) v.update(4000,t);
 assert(v.detected && v.large && v.envelope>.8f);
 v.update(100,2500); assert(v.detected);
 for(unsigned t=2520;t<4500;t+=20) v.update(100,t);
 assert(!v.detected && !v.large && v.envelope<.01f);
 // Renders must stay inside 64 pixels; asymmetric eyes must be mirrored.
 for(int face=0;face<6;++face) for(int color=0;color<8;++color) {
   uint16_t l[64],r[64];
   eyes::render(l,face,color,false,false,.8,1,true);
   eyes::render(r,face,color,true,false,.8,1,true);
   unsigned lit=0;
   for(int y=0;y<8;++y) for(int x=0;x<8;++x) {
     if(l[y*8+x]) ++lit;
     int rx=(face==2||face==3)?7-x:x;
     assert(l[y*8+x]==r[y*8+rx]);
   }
   assert(lit>0 && lit<64);
   eyes::render(l,face,color,false,false,.8,0,true);
   for(int y=0;y<8;++y) if(y!=4) for(int x=0;x<8;++x) assert(!l[y*8+x]);
   eyes::render(l,face,color,false,false,.8,1,false);
   for(auto p:l) assert(p==0);
 }
 assert(eyes::wrap(-1,8)==7 && eyes::wrap(8,8)==0);
 std::cout<<"PASS: button, rollover, audio gate/decay, 48 face/color combinations, mirror, blink, power\n";
}
