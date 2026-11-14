// Usage: render_demo <out.wav> <tracklist.txt>. Build: -DPENUMBRA_BUILD_TOOLS=ON (target penumbra_render_demo).
// Renders every factory preset on a synthetic guitar or synth phrase into one WAV.
#include "Engine.h"
#include <cstdio>
#include <string>
#include <cstdint>
#include <vector>
using namespace pn;
static const double fs = 48000;
static double saw(double ph){ return 2.0*(ph-std::floor(ph))-1.0; }
static void guitar(std::vector<float>& L, double secs){ // plucked phrase with harmonics
  const double notes[]={164.8,196.0,246.9,293.7,329.6,293.7,246.9,196.0}; size_t n=(size_t)(fs*secs);
  for(size_t i=0;i<n;++i){ double t=i/fs; int k=(int)(t/0.45); double tn=t-k*0.45; double f=notes[k%8]; double s=0;
    if(t<secs-2.0) for(int h=1;h<=10;++h) s+=std::sin(2*kPi*f*h*tn)*std::exp(-tn*(2.5+h*0.9))/h;
    L.push_back((float)(0.45*s)); } }
static void synth(std::vector<float>& L, double secs){ // detuned saw chord swells + 16th sequence
  size_t n=(size_t)(fs*secs); const double ch[]={110.0,164.8,220.0,277.2}; double lp=0;
  for(size_t i=0;i<n;++i){ double t=i/fs; double s=0; if(t<secs-2.0){
    for(double f:ch) s+=saw(f*t)+saw(f*1.004*t);
    double env=std::min(1.0,t/0.6)*std::min(1.0,(secs-2.0-t)/0.3);
    int st=(int)(t*8); double tn=t-st/8.0; const double seq[]={220,261.6,329.6,392,440,392,329.6,261.6};
    s=s*0.06*env + 0.25*saw(seq[st%8]*t)*std::exp(-tn*14); }
    lp+= (s-lp)*0.25; L.push_back((float)lp);} }
int main(int argc,char**argv){
  int np=0; auto* ps=factoryPresets(np); std::vector<float> outL,outR; FILE* list=std::fopen(argv[2],"w");
  for(int p=1;p<np;++p){ std::vector<float> x; bool g=std::string(ps[p].category)=="Guitar"; if(g) guitar(x,7.0); else synth(x,7.0);
    Params pr=ps[p].p; Engine e; e.setParams(pr); e.prepare(fs,512); e.setTempo(120);
    std::vector<float> l=x,r=x; for(size_t i=0;i<l.size();i+=512){int n=(int)std::min<size_t>(512,l.size()-i); e.process(l.data()+i,r.data()+i,n);} 
    double ri=0,ro=0,pk=0; for(size_t i=0;i<(size_t)(fs*5);++i){ri+=x[i]*x[i]; ro+=0.5*(l[i]*l[i]+r[i]*r[i]); pk=std::max(pk,(double)std::max(std::abs(l[i]),std::abs(r[i])));} std::printf("%-26s gain %+5.1f dB  peak %.2f\n",ps[p].name,10*std::log10(ro/ri),pk); double norm=pk>0.89?0.89/pk:1.0;
    double t0=outL.size()/fs; std::fprintf(list,"%d:%02d  %s (%s)\n",(int)t0/60,(int)t0%60,ps[p].name,ps[p].category);
    for(size_t i=0;i<l.size();++i){ double fade = i+4800>l.size()? (l.size()-i)/4800.0:1.0; outL.push_back(l[i]*norm*fade); outR.push_back(r[i]*norm*fade);} 
    for(int i=0;i<(int)(fs*0.5);++i){outL.push_back(0);outR.push_back(0);} }
  std::fclose(list);
  FILE* f=std::fopen(argv[1],"wb"); uint32_t n=outL.size(), data=n*4, sz=36+data; uint16_t ch=2,bits=16,fmt=1,ba=4; uint32_t rate=48000, br=rate*4, sixteen=16;
  std::fwrite("RIFF",1,4,f); std::fwrite(&sz,4,1,f); std::fwrite("WAVEfmt ",1,8,f); std::fwrite(&sixteen,4,1,f); std::fwrite(&fmt,2,1,f); std::fwrite(&ch,2,1,f); std::fwrite(&rate,4,1,f); std::fwrite(&br,4,1,f); std::fwrite(&ba,2,1,f); std::fwrite(&bits,2,1,f); std::fwrite("data",1,4,f); std::fwrite(&data,4,1,f);
  for(uint32_t i=0;i<n;++i){ int16_t a=(int16_t)std::lround(std::max(-1.f,std::min(1.f,outL[i]))*32767), b=(int16_t)std::lround(std::max(-1.f,std::min(1.f,outR[i]))*32767); std::fwrite(&a,2,1,f); std::fwrite(&b,2,1,f);} std::fclose(f); }
