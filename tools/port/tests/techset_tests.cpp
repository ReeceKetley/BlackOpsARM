#include <database/db_techset_load.h>
#define BO1_CODE_ZONE_MAIN CodeZonePrerequisiteMain
#include "code_zone_tests.cpp"
#undef BO1_CODE_ZONE_MAIN
// Fixture-only dependency refusal. The certified asset has 130 null tags.
void __cdecl Load_MaterialTechnique(bool){throw std::runtime_error("TechniqueSet fixture does not support nonnull technique dependency");}
static const StringTablePayloadPlan*techsetPriorTable;
void ReportTechniqueSetDefaultUnavailable(){
 auto*s=varMaterialTechniqueSet;require(reader&&reader->Position()+52==66226&&s&&s->name&&s->name[0]==','&&std::all_of(std::begin(s->techniques),std::end(s->techniques),[](auto*p){return !p;}),"actual stub dependency reached after complete genuine header/name130loop");require(adler32(1,reinterpret_cast<const Bytef*>(s->name),20)==1243743876,"genuine observed name checksum before dependency refusal");
 require(g_streamPosIndex==0&&g_streamPosStackIndex==2&&DB_GetStreamPos()==g_streamBlocks[0].data+sizeof(MaterialTechniqueSet)&&XAssetEntryHighCount==1,"default refusal before actual Add publication and root Pop");
 require(techsetPriorTable,"prior genuine table plan available");executeLookups(*techsetPriorTable,owner_fixture::persistent,false);require(lastLookup.queries==1594&&!lastLookup.rowMisses&&!lastLookup.rowDisagreements&&!lastLookup.columnDisagreements,"all1594 genuine queries before explicit dependency abort");
 std::printf("{\"schema\":\"BlackOpsARM.TASK38.blocked.v1\",\"pointer_bytes\":%zu,\"source_end\":66226,\"header_bytes\":%zu,\"null_techniques\":130,\"stub_name\":true,\"missing_dependency\":\"DB_CreateDefaultEntry/preloaded genuine TechniqueSet\",\"registered_assets\":1,\"techset_registered\":false,\"root_pop_executed\":false,\"actual_deserialization_complete\":true,\"prior_queries\":1594}\n",sizeof(void*),sizeof(MaterialTechniqueSet));std::fflush(stdout);
}
namespace techset {
static void decodePrefix(const char*path,const char*out){Bo1InflatedFastFile f;require(Bo1_InflateFastFilePath(path,{},&f)==Bo1InflateResult::Ok&&f.decoded.size()>66226,"actual complete retail zombie decode");auto count=f.decoded.size();auto digest=adler32(1,f.decoded.data(),static_cast<unsigned>(count));std::ofstream file(out,std::ios::binary);file.write(reinterpret_cast<const char*>(f.decoded.data()),66226);require(bool(file),"native bounded techset prefix output");std::vector<std::uint8_t>().swap(f.decoded);std::printf("{\"decoded_bytes\":%zu,\"decoded_adler32\":%lu,\"prefix_bytes\":66226,\"buffer_released\":true}\n",count,(unsigned long)digest);}
static void run(const char*path){
 std::ifstream file(path,std::ios::binary|std::ios::ate);require(bool(file)&&file.tellg()==66226,"bounded exact TechniqueSet prefix before allocation");file.close();auto bytes=readPrefix(path);require(bytes.size()==66226,"exact bounded zombie prefix");auto f=preflight(bytes.data(),bytes.size(),108009041);auto dir=directoryPlan(f,bytes.data(),bytes.size());auto header=headerPlan(f,dir,bytes.data(),bytes.size(),108009041);auto table=payloadPlan(f,dir,header,bytes.data(),bytes.size());
 require(table.end==65678&&u32(bytes.data()+f.end+8)==7&&u32(bytes.data()+f.end+12)==0xffffffffu&&u32(bytes.data()+f.end+16)==5,"actual sequential type7 before XModel5");
 require(u32(bytes.data()+65678)==0xffffffffu&&!u32(bytes.data()+65682),"genuine name tag and scalar bytes");for(unsigned i=0;i<130;++i)require(!u32(bytes.data()+65686+4*i),"preflight all130 null techniques before allocations");require(bytes[66225]==0&&!std::memchr(bytes.data()+66206,0,19),"exact inline name20");
 require(bytes[66206]==','&&std::all_of(bytes.begin()+66206,bytes.end(),[](unsigned char c){return c<128;}),"genuine ASCII stub name before registry hashing");
 frontend_fixture::startup();DB_InitStreams(detached);DB_InitPool<decltype(g_StringTablePool)>(&g_StringTablePool,576);DB_InitPool<decltype(g_MaterialTechniqueSetPool)>(&g_MaterialTechniqueSetPool,2048);owner_fixture::active=576;entry_owner_fixture::begin();
 auto zone=f.envelope.zone;zone.limits.allocType=0;frontend_fixture::Owner owner;owner.type=0;XBlock blocks[7]{};unsigned failed;require(AllocateZonePlan(zone,frontend_fixture::page(),frontend_fixture::backend(owner),blocks,failed)==ZoneResult::Ok,"actual zombie owner");DB_InitStreams(blocks);XAssetList envelope{};seedEnvelope(bytes.data(),envelope);DecodedSource r(bytes.data()+52,bytes.size()-52);execute(f,r,blocks,false);executeDirectory(f,dir,r,blocks,false);DB_PushStreamPos(4);executeHeader(header,r,blocks,&envelope,false,&table,nullptr);
 auto*assets=reinterpret_cast<XAsset*>(blocks[4].data+dir.nativeStart);auto*slot=reinterpret_cast<MaterialTechniqueSet**>(&assets[1].header);require(reinterpret_cast<uintptr_t>(*slot)==UINTPTR_MAX&&r.Position()+52==65678,"actual widened root directory slot");varMaterialTechniqueSetPtr=slot;reader=&r;tableReadBytes=528;techsetPriorTable=&table;Load_MaterialTechniqueSetPtr(false);
 require(false,"genuine stub unexpectedly returned without a preloaded default");
}
}
int main(int argc,char**argv){if(argc==3&&!std::strcmp(argv[1],"--techset")){techset::run(argv[2]);return 0;}if(argc==4&&!std::strcmp(argv[1],"--decode-techset")){techset::decodePrefix(argv[2],argv[3]);return 0;}return CodeZonePrerequisiteMain(argc,argv);}
