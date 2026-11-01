"""Generate the actual bounded stock Ascension voice-worker repair fixture.

Tests the private source transform/guards, not a substitute GSC interpreter.
The real old-script failure and repaired natural round must be tested separately.
"""
from pathlib import Path
import json, hashlib
ROOT=Path(__file__).resolve().parents[3]
source=ROOT/'src/clientscript/cscr_parser.cpp';text=source.read_text(encoding='utf-8')
helpers=text[text.index('static bool Scr_ModIsSpace('):text.index('// The retail round loop')]
body=text[text.index('static void Scr_AscMonkeyVoxLifetimeSource('):text.index('static void Scr_ModPatchSource(')]
stock=ROOT/'build-android-audit/asc-monkey-lifetime-regression/stock-worker.gsc'
sample=stock.read_text(encoding='utf-8')
fixture=r"""
#include <clientscript/cscr_main.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
static bool sp=true,zombie=true;
static const char *mapName="zombie_cosmodrome";
static unsigned checks;
bool Com_IsSPLevel(){return sp;}
bool G_SP_IsZombieMode(){return zombie;}
const char *Dvar_GetString(const char *name){if(std::strcmp(name,"mapname"))std::abort();return mapName;}
int I_stricmp(const char *a,const char *b){
    for(;*a&&*b;++a,++b){int d=std::tolower((unsigned char)*a)-std::tolower((unsigned char)*b);if(d)return d;}
    return (unsigned char)*a-(unsigned char)*b;
}
void Com_Printf(int,const char*,...){}
ACTUAL_CODE
static void check(const std::string &input,scriptInstance_t inst,const char *file,bool expected){
    std::string data=input;
    Scr_AscMonkeyVoxLifetimeSource(inst,file,data.data(),(int)data.size());
    const std::string guard="monkey.pack.perk.script_noteworthy!=perk";
    bool patched=data.find(guard)!=std::string::npos;
    bool already=input.find(guard)!=std::string::npos;
    if((data!=input)!=expected||patched!=(expected||already)||data.size()!=input.size()
        ||std::count(data.begin(),data.end(),'\n')!=std::count(input.begin(),input.end(),'\n')){
        std::fprintf(stderr,"FAIL size=%zu expected=%d changed=%d patched=%d\n",input.size(),expected,data!=input,patched);std::abort();
    }
    if(expected){
        size_t begin=input.find("play_player_perk_theft_vox"),end=input.find("suffix");
        if(data.substr(0,begin)!=input.substr(0,begin)||data.substr(end)!=input.substr(end))std::abort();
        size_t count=0;for(size_t i=0;(i=data.find(guard,i))!=std::string::npos;i+=guard.size())++count;
        if(count!=2||data.find("level.perk_theft_vox[perk]=false;")==std::string::npos)std::abort();
    }
    ++checks;
}
int main(){
    std::string sample=SAMPLE;
    const char *file="maps/_zombiemode_ai_monkey.gsc";
    const std::string prefix="prefix // unchanged\n",suffix="\nsuffix // unchanged\n";
    std::string input=prefix+sample+suffix;
    check(input,SCRIPTINSTANCE_SERVER,file,true);
    auto crlf=input;for(size_t p=0;(p=crlf.find('\n',p))!=std::string::npos;p+=2)crlf.insert(p,"\r");
    check(crlf,SCRIPTINSTANCE_SERVER,file,true);
    auto applied=input;Scr_AscMonkeyVoxLifetimeSource(SCRIPTINSTANCE_SERVER,file,applied.data(),(int)applied.size());
    check(applied,SCRIPTINSTANCE_SERVER,file,false);
    check(input,SCRIPTINSTANCE_CLIENT,file,false);
    check(input,SCRIPTINSTANCE_SERVER,"maps/custom.gsc",false);
    check(input,SCRIPTINSTANCE_SERVER,"maps/_zombiemode_ai_monkey.gsc.custom",false);
    sp=false;check(input,SCRIPTINSTANCE_SERVER,file,false);sp=true;
    zombie=false;check(input,SCRIPTINSTANCE_SERVER,file,false);zombie=true;
    mapName="zombie_temple";check(input,SCRIPTINSTANCE_SERVER,file,false);mapName="zombie_cosmodrome";
    check(input+input,SCRIPTINSTANCE_SERVER,file,false);
    auto custom=input;custom.replace(custom.find("force_quit = 0"),14,"force_quit = 7");check(custom,SCRIPTINSTANCE_SERVER,file,false);
    auto ident=input;ident.insert(ident.find("play_player_perk_theft_vox"),"custom_");check(ident,SCRIPTINSTANCE_SERVER,file,false);
    for(size_t n=0;n<sample.size();++n)check(prefix+sample.substr(0,n)+suffix,SCRIPTINSTANCE_SERVER,file,false);
    std::printf("PASS actual Ascension voice-worker source repair: %u bounded/mode/map/file/custom/truncation cases\n",checks);
}
"""
fixture=fixture.replace('ACTUAL_CODE',helpers+body).replace('SAMPLE','R"BO1GSC('+sample+')BO1GSC"')
out=ROOT/'build-android-audit/asc-monkey-lifetime-regression'
(out/'fixture.cpp').write_text(fixture,encoding='utf-8')
(out/'generation.json').write_text(json.dumps({'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'actual_body_sha256':hashlib.sha256((helpers+body).encode()).hexdigest(),'stock_worker_sha256':hashlib.sha256(stock.read_bytes()).hexdigest(),'scope':'Actual transform, SP/Zombie/map/file/unique/custom/truncated guards, both original mutating calls retained behind actual-target guards, voice lock cleanup, unchanged byte/LF count/prefix/suffix. Not live GSC execution.'},indent=2)+'\n',encoding='utf-8')
print(out)
