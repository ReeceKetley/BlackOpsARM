#include <ui_mp/ui_gametype_variants_mp.h>
#include <ui/ui_main.h>
#include <new>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <type_traits>
extern GVGlob gvGlob;
static unsigned checks;static int selected;
static void Check(bool ok,const char *name){if(!ok){std::fprintf(stderr,"FAIL: %s\n",name);std::abort();}++checks;}
// Isolated UI-selection adapters only: no window/menu readiness is represented.
static uiInfo_s info;
uiInfo_s *UI_GetInfo(int local){Check(local==0,"actual selector requests client0");return &info;}
int Menu_GetFeederSelection(UiContext *context,menuDef_t *menu,int feeder,const char *name){Check(context==&info.uiDC && !menu && feeder==44 && !std::strcmp(name,"gametype_variants"),"canonical feeder request");return selected;}
int main(){
    static_assert(sizeof(GVRule)==266 && alignof(GVRule)==1);
    static_assert(sizeof(GVGlob)==17028 && alignof(GVGlob)==4);
    static_assert(std::is_same_v<decltype(GetSelectedRule(0)),GVRule *>);
    static_assert(std::is_same_v<decltype(GetRuleForFeederIndex(0)),GVRule *>);
    struct alignas(GVGlob) Storage{unsigned char before[16];alignas(GVGlob) unsigned char object[sizeof(GVGlob)];unsigned char after[16];} storage;
    std::memset(&storage,0xA5,sizeof(storage));auto *owned=new(storage.object) GVGlob;
    for(unsigned i=0;i<64;++i){const auto *row=reinterpret_cast<const unsigned char *>(&owned->rules[i]);Check(row==storage.object+i*sizeof(GVRule),"actual typed row stride");for(unsigned j=0;j<sizeof(GVRule);++j){const bool conditional=j>=offsetof(GVRule,m_conditional) && j<offsetof(GVRule,m_conditional)+sizeof(GVConditional);Check(row[j]==(conditional?0:0xA5),"constructor modifies only row conditional bytes");}}
    for(auto byte:storage.before)Check(byte==0xA5,"constructor prefix canary");for(auto byte:storage.after)Check(byte==0xA5,"constructor suffix canary");
    const unsigned char *countBytes=reinterpret_cast<const unsigned char *>(&owned->ruleCount);for(unsigned i=0;i<sizeof(int);++i)Check(countBytes[i]==0xA5,"constructor preserves nonconditional count bytes");owned->~GVGlob();
    std::memset(gvGlob.rules,0,sizeof(gvGlob.rules));gvGlob.ruleCount=3;gvGlob.rules[1].isValid=true;gvGlob.rules[31].isValid=true;gvGlob.rules[63].isValid=true;
    const int indices[]={1,31,63};for(int feeder=0;feeder<3;++feeder){GVRule *rule=GetRuleForFeederIndex(feeder);Check(rule==&gvGlob.rules[indices[feeder]],"canonical sparse feeder returns owned GVRule");selected=feeder;Check(GetSelectedRule(0)==rule,"actual selected-rule consumer returns identical typed row");rule->parameter.value[127]=static_cast<char>(feeder+1);Check(gvGlob.rules[indices[feeder]].parameter.value[127]==feeder+1,"selected tail byte writes correct actual row");}
    Check(!GetRuleForFeederIndex(-1) && !GetRuleForFeederIndex(3),"feeder bounds fail without fake row");gvGlob.ruleCount=64;Check(!GetRuleForFeederIndex(63),"inconsistent valid count returns no row");gvGlob.ruleCount=65;Check(!GetRuleForFeederIndex(0),"malformed count cannot manufacture row");gvGlob.ruleCount=-1;Check(!GetRuleForFeederIndex(0),"negative count cannot manufacture row");
    std::printf("PASS: %u actual constructor/typed-row/selection/canary checks\n",checks);
}
