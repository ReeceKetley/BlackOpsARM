// Actual canonical vehicle callbacks, weapon graph parser, tokenizer and yEnc.
// Files are real temporary files. Only isolated logging/thread/file providers
// are installed; unrelated callback paths fail explicitly if reached.
#include <bgame/bg_misctables.h>
#include <game/g_weapon_load_obj.h>
#include <vehicle/nitrous_vehicle.h>
#include <universal/com_encode.h>
#include <universal/q_parse.h>
#include <qcommon/common.h>
#include <clientscript/cscr_vm.h>
#include <universal/com_files.h>
#include <algorithm>
#include <array>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <pthread.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

static unsigned checks, opens, closes;
static FILE *file;
static bool truncateAfterOpen;
static const pthread_t owner = pthread_self();
static void check(bool ok, const char *name) {
    if (!ok) { fprintf(stderr, "FAIL %s\n", name); exit(1); }
    ++checks; printf("PASS %s\n", name);
}
static void logMessage(const char *format, va_list args) { vfprintf(stderr, format, args); }
void Com_Printf(int, const char *format, ...) { va_list args; va_start(args,format); logMessage(format,args); va_end(args); }
void Com_PrintError(int, const char *format, ...) { va_list args; va_start(args,format); logMessage(format,args); va_end(args); }
void Com_PrintWarning(int, const char *format, ...) { va_list args; va_start(args,format); logMessage(format,args); va_end(args); }
void Com_Error(errorParm_t, const char *format, ...) { va_list args; va_start(args,format); logMessage(format,args); va_end(args); abort(); }
bool Assert_MyHandler(const char *, int, int, const char *format, ...) { va_list args; va_start(args,format); logMessage(format,args); va_end(args); abort(); }
bool Sys_IsMainThread() { return pthread_equal(owner,pthread_self()); }
bool Sys_IsRenderThread() { return false; }
bool Sys_IsServerThread() { return false; }
bool Sys_IsDatabaseThread() { return false; }
int I_stricmp(const char *a,const char *b) { return strcasecmp(a,b); }
char *va(const char *format,...) { static thread_local char text[4096]; va_list args; va_start(args,format); vsnprintf(text,sizeof(text),format,args); va_end(args); return text; }
int Key_StringToKeynum(const char *) { abort(); }
unsigned SL_FindLowercaseString(const char *,scriptInstance_t) { abort(); }
void Scr_Error(const char *,bool) { abort(); }
GraphFloat *GraphFloat_Load(char *) { abort(); }

unsigned FS_FOpenFileByMode(char *path,int *handle,fsMode_t mode) {
    check(mode==FS_READ && !file,"file provider owns one read handle");
    file=fopen(path,"rb+"); ++opens;
    if (!file) { *handle=0; return -1; }
    fseek(file,0,SEEK_END); const long length=ftell(file); rewind(file);
    if (truncateAfterOpen) { check(ftruncate(fileno(file),length-1)==0,"actual file shortened after measured length"); truncateAfterOpen=false; }
    *handle=1; return static_cast<int>(length);
}
unsigned FS_Read(unsigned char *out,unsigned length,int handle) {
    check(file && handle==1,"read uses owned real file");
    return static_cast<unsigned>(fread(out,1,length,file));
}
void FS_FCloseFile(int handle) { check(file && handle==1,"close retires real file"); fclose(file); file=nullptr; ++closes; }
static void writeGraph(const char *data) {
    FILE *out=fopen("accuracy/aivsai/fixture","wb"); check(out!=nullptr,"create real graph file");
    const size_t length=strlen(data); check(fwrite(data,1,length,out)==length,"write complete graph file"); fclose(out);
}
static int offset(const char *name) {
    for (const auto &field:s_vehicleFields) if (!strcmp(field.szName,name)) return field.iOffset;
    abort();
}
static void vehicles() {
    check(offset("gasButton")==offsetof(vehicle_info_t,gasButton) &&
          offset("dropDeployableButton")==offsetof(vehicle_info_t,dropDeployableButton),"all button tail endpoints use native layout");
    check(offset("steerGraph")==offsetof(vehicle_info_t,steerGraph) &&
          offset("accelGraph")==offsetof(vehicle_info_t,accelGraph),"graph pointer fields use native aligned offsets");
    check(offset("isNitrous")==offsetof(vehicle_info_t,isNitrous) &&
          offset("nitrous_traction_type")==offsetof(vehicle_info_t,nitrousVehParams)+offsetof(VehicleParameter,m_traction_type),"post-pointer scalar and parameter tail use native layout");
    struct Guarded { unsigned char before[32]; alignas(vehicle_info_t) unsigned char value[sizeof(vehicle_info_t)]; unsigned char after[32]; } guarded;
    memset(&guarded,0xa5,sizeof(guarded));
    auto *bytes=guarded.value;
    std::array<unsigned char,sizeof(vehicle_info_t)> expected; memcpy(expected.data(),bytes,expected.size());
    const int traction=offset("nitrous_traction_type"); const unsigned type=TRACTION_TYPE_BACK;
    memcpy(expected.data()+traction,&type,sizeof(type));
    check(VEH_ParseSpecificField(bytes,"TRACTION_TYPE_BACK",18,traction)==1,"actual traction callback succeeds");
    check(!memcmp(expected.data(),bytes,expected.size()),"traction callback changes exactly native member bytes");
    const int mass=offset("nitrous_body_mass"); const float massValue=1.0f;
    memcpy(expected.data()+mass,&massValue,sizeof(massValue));
    check(VEH_ParseSpecificField(bytes,"1000",20,mass)==1 && !memcmp(expected.data(),bytes,expected.size()),"actual scaled scalar callback writes native tail only");
    check(std::all_of(std::begin(guarded.before),std::end(guarded.before),[](unsigned char v){return v==0xa5;}) &&
          std::all_of(std::begin(guarded.after),std::end(guarded.after),[](unsigned char v){return v==0xa5;}),"vehicle surrounding canaries intact");
}
static void graphs() {
    struct Knots { unsigned before=0xa5a5a5a5; float pairs[16][2]{}; unsigned after=0xa5a5a5a5; } knots;
    Com_InitParse(); int count=0;
    check(G_ParseAIWeaponAccurayGraphFile("3 0 0.25 0.5 0.75 1 1", "valid",knots.pairs,&count)==1 && count==3 && knots.pairs[1][0]==0.5f && knots.pairs[2][1]==1.0f,"actual tokenizer and graph parser publish three typed pairs");
    check(G_ParseAIWeaponAccurayGraphFile("0", "zero",knots.pairs,&count)==0,"zero knot declaration rejected");
    check(G_ParseAIWeaponAccurayGraphFile("2 0 0.2 1", "missing-value",knots.pairs,&count)==0,"incomplete knot pair rejected");
    check(G_ParseAIWeaponAccurayGraphFile("16", "capacity",knots.pairs,&count)==0,"original rejected sixteenth-pair limit preserved");
    check(knots.before==0xa5a5a5a5 && knots.after==0xa5a5a5a5,"graph pair canaries intact");
    check(Com_GetParseThreadInfo()->parseInfoNum==0,"all parser paths retire actual parse sessions");
    WeaponDef weapon{}; weapon.weapType=WEAPTYPE_BULLET;
    writeGraph("WEAPONACCUFILE3 0 0.25 0.5 0.75 1 1");
    check(G_ParseWeaponAccurayGraphInternal(&weapon,"aivsai","fixture",knots.pairs,&count)==1,"real file wrapper parses canonical graph");
    writeGraph("WEAPONACCUFILE3 0 0.25 0.5 0.75 1 1"); truncateAfterOpen=true;
    check(G_ParseWeaponAccurayGraphInternal(&weapon,"aivsai","fixture",knots.pairs,&count)==0,"real short body read rejected");
    writeGraph("WEAPONACCUFILE"); truncateAfterOpen=true;
    check(G_ParseWeaponAccurayGraphInternal(&weapon,"aivsai","fixture",knots.pairs,&count)==0,"real short header read rejected");
    writeGraph("short");
    check(G_ParseWeaponAccurayGraphInternal(&weapon,"aivsai","fixture",knots.pairs,&count)==0,"advertised file shorter than header rejected");
    check(opens==closes && !file,"every graph failure closes real file");
    char tooLong[80]; memset(tooLong,'x',sizeof(tooLong)-1); tooLong[sizeof(tooLong)-1]=0;
    const unsigned before=opens;
    check(G_ParseWeaponAccurayGraphInternal(&weapon,"aivsai",tooLong,knots.pairs,&count)==0 && opens==before,"oversize formatted path rejected before open");
}
static void encoding() {
    unsigned char alphabet[256],decoded[256]; for(unsigned i=0;i<256;++i) alphabet[i]=static_cast<unsigned char>(i);
    char encoded[600]; const int size=Com_EncodeYEnc(alphabet,256,encoded,sizeof(encoded));
    check(size>256 && Com_DecodeYEnc(encoded,decoded,sizeof(decoded))==256 && !memcmp(alphabet,decoded,256),"actual yEnc round-trips complete byte alphabet");
    char guarded[8]; memset(guarded,0xa5,sizeof(guarded));
    check(Com_EncodeYEnc(alphabet,1,guarded+2,0)==0 && static_cast<unsigned char>(guarded[2])==0xa5,"zero-capacity encode writes nothing");
    check(Com_EncodeYEnc(alphabet,1,guarded+2,1)==0 && guarded[2]==0 && static_cast<unsigned char>(guarded[3])==0xa5,"one-byte output fails with bounded terminator");
    unsigned char critical=static_cast<unsigned char>(61-42); memset(guarded,0xa5,sizeof(guarded));
    check(Com_EncodeYEnc(&critical,1,guarded+2,2)==0 && guarded[2]==0 && static_cast<unsigned char>(guarded[4])==0xa5,"escaped byte cannot overrun two-byte destination");
    check(Com_EncodeYEnc(&critical,1,guarded+2,3)==2 && guarded[4]==0 && static_cast<unsigned char>(guarded[5])==0xa5,"escaped byte exactly fills payload plus terminator");
    check(Com_DecodeYEnc(encoded,decoded,255)==0 && Com_DecodeYEnc(encoded,decoded,-1)==0,"decode rejects undersized and negative capacity");
}
int main() {
    check(mkdir("accuracy",0700)==0 && mkdir("accuracy/aivsai",0700)==0,"create isolated real file directories");
    vehicles(); graphs(); encoding(); printf("TABLES_ASSET_HELPERS PASS checks=%u\n",checks);
}
