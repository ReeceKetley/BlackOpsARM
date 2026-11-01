"""Actual CCR1 codec, canonical declarations and field coverage. Synthetic IO.

Actual service callbacks, animation/material binding and sidecars are APK tests.
"""
from pathlib import Path
import argparse,subprocess,sys,re,json,hashlib
root=Path(__file__).resolve().parents[3]
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);a=p.parse_args()
subprocess.run([sys.executable,str(root/'tools/port/tests/native_client_prediction_owners_regression.py'),'--out',str(a.out)],check=True)
source=root/'src/game_sp/g_sp_client_character_checkpoint.inl';text=source.read_text()
helpers=text[:text.index('static clientInfo_t *SP_CRCClientSlot')]
# A new field must be deliberately classified; no silent record-size mirroring.
header=(root/'src/bgame/bg_local.h').read_text()
decl=header[header.index('struct __declspec(align(4)) clientInfo_t'):header.index('struct loadAnim_t')]
decl=re.sub(r'//[^\n]*','',decl)
fields=re.findall(r'^\s*[\w:]+(?:\s+\w+)?\s*\*?\s+(\w+)\s*(?:\[[^;]*\])?;',decl,re.M)
for field in fields:
 assert re.search(r'\b'+field+r'\b',text),'Unclassified clientInfo field '+field
tests=r'''
 crcFixtureExtended=true;
 static SPClientCharacters characters{},charactersDecoded{};
 characters.definitions.resize(2);strcpy(characters.definitions[1].name,"walk");characters.definitions[1].moveSpeed=110;
 characters.actors.resize(MAX_ACTORS+MAX_ACTOR_CORPSES_SP);
 auto &character=characters.clients[0];character.info.infoValid=1;character.info.clientNum=0;
 strcpy(character.info.name,"saved player");strcpy(character.info.model,"char_model");
 character.legs=1;character.torso=0;character.status="hud_status";character.rank="hud_rank";
 character.info.legs.animation=reinterpret_cast<animation_s *>(0xdeadbeef);
 character.info.pXAnimTree=reinterpret_cast<XAnimTree_s *>(0xdeadbeef);
 character.info.hStatusIcon=reinterpret_cast<Material *>(0xdeadbeef);
 character.info.control.tag_origin_offset[0]=12;character.info.clientConditions[22]=0xfedc1234;
 character.info.weaponSpin=15;characters.clients[35].info.health=19;
 auto &actor=characters.actors.back();actor.info.infoValid=1;actor.info.actorNum=0;actor.info.entityNum=30;
 actor.animation=1;strcpy(actor.models.model,"actor_model");actor.models.attachIgnoreCollision=63;
 actor.info.animInfo.legs.oldFramePos[2]=31;characters.time=14100;characters.prone[31]=700;
 auto characterEncode=[&]{encoded={};SPNativeCheckpointCodec c{nullptr,&encoded};SP_ClientCharactersCodec(c,characters);};
 auto characterDecode=[&]{input={};decoded={};input.buffer=crcFixtureBuffers[&encoded].data();input.bufferSize=encoded.cacheBufferUsed;SPNativeCheckpointCodec c{&input,&decoded};charactersDecoded={};SP_ClientCharactersCodec(c,charactersDecoded);};
 characterEncode();auto characterBytes=crcFixtureBuffers[&encoded];characterDecode();check(characterBytes==crcFixtureBuffers[&decoded]);
 check(charactersDecoded.clients[0].legs==1&&charactersDecoded.clients[0].torso==0);
 check(!charactersDecoded.clients[0].info.legs.animation&&!charactersDecoded.clients[0].info.pXAnimTree&&!charactersDecoded.clients[0].info.hStatusIcon);
 check(charactersDecoded.clients[0].status=="hud_status"&&charactersDecoded.clients[35].info.health==19);
 check(charactersDecoded.actors.back().animation==1&&charactersDecoded.actors.back().models.attachIgnoreCollision==63);
 check(charactersDecoded.clients[0].info.clientConditions[22]==0xfedc1234&&charactersDecoded.prone[31]==700);
 character.info.legs.animation=nullptr;character.info.pXAnimTree=nullptr;character.info.hStatusIcon=nullptr;
 characterEncode();check(characterBytes==crcFixtureBuffers[&encoded]); // addresses cannot affect disk bytes
 auto refused=[&]{bool bad=false;try{characterEncode();}catch(...){bad=true;}check(bad);};
 character.legs=2;refused();character.legs=-2;refused();character.legs=1;
 character.info.weaponSpin=std::numeric_limits<float>::infinity();refused();character.info.weaponSpin=15;
 memset(character.info.model,'x',sizeof(character.info.model));refused();strcpy(character.info.model,"char_model");
 character.info.clientNum=32;refused();character.info.clientNum=0;
 actor.models.attachIgnoreCollision=64;refused();actor.models.attachIgnoreCollision=63;
 actor.info.entityNum=MAX_CL_ENTNUM;refused();actor.info.entityNum=30;
 actor.animation=-2;refused();actor.animation=1;
 characters.definitions[1].forceAnimRate=std::numeric_limits<float>::quiet_NaN();refused();characters.definitions[1].forceAnimRate=0;
 characterEncode();auto allCharacterBytes=crcFixtureBuffers[&encoded];
 for(size_t cut:{size_t(0),size_t(4),size_t(8),allCharacterBytes.size()/2,allCharacterBytes.size()-1}){
  crcFixtureBuffers[&encoded].assign(allCharacterBytes.begin(),allCharacterBytes.begin()+cut);encoded.cacheBufferUsed=cut;
  bool bad=false;try{characterDecode();}catch(...){bad=true;}check(bad);
 }
 auto identityBytes=[&](const animation_s &v){encoded={};SPNativeCheckpointCodec c{nullptr,&encoded};SP_CRCIdentityDefinition(c,v);return crcFixtureBuffers[&encoded];};
 animation_s rateA{},rateB{};strcpy(rateA.name,"pt_pistol_stand_putaway");rateB=rateA;rateA.forceAnimRate=-1;rateB.forceAnimRate=1.11111116f;
 check(identityBytes(rateA)==identityBytes(rateB));check(rateA.forceAnimRate==-1&&rateB.forceAnimRate>1);
 rateB.flags=1;check(identityBytes(rateA)!=identityBytes(rateB));rateB=rateA;rateB.duration=500;check(identityBytes(rateA)!=identityBytes(rateB));
 printf("actual CCR1 character/animation/material-name codec PASS; ");
'''
f=a.out/'fixture.cpp';s=f.read_text().replace('static void check(bool b)',helpers+'\nstatic void check(bool b)')
s=s.replace(' printf("actual SDP2 mixer',tests+'\n printf("actual SDP2 mixer')
# Prior fixtures deliberately fit the engine's inline write cache. CCR1 is
# larger; use a bounded synthetic uncompressed byte stream for this phase.
s=s.replace('MemFile_WriteData(', 'CRCFixtureWriteData(')
s=s.replace('MemFile_WriteCString(', 'CRCFixtureWriteCString(')
io=r'''
#include <unordered_map>
static bool crcFixtureExtended;
static std::unordered_map<MemoryFile *,std::vector<unsigned char>> crcFixtureBuffers;
static void CRCFixtureWriteData(MemoryFile *f,unsigned n,unsigned char *p){
 if(!crcFixtureExtended){MemFile_WriteData(f,n,p);return;}
 if(n>0x100000||f->cacheBufferUsed<0||f->cacheBufferUsed>0x100000-int(n))throw std::runtime_error("bounded IO");
 auto &b=crcFixtureBuffers[f];b.resize(f->cacheBufferUsed+n);memcpy(b.data()+f->cacheBufferUsed,p,n);f->cacheBufferUsed+=n;
}
static void CRCFixtureWriteCString(MemoryFile *f,char *p){CRCFixtureWriteData(f,strlen(p)+1,reinterpret_cast<unsigned char *>(p));}
'''
s=s.replace('static unsigned checks;',io+'\nstatic unsigned checks;')
f.write_text(s,encoding='utf-8')
manifest=json.loads((a.out/'source-manifest.json').read_text());manifest.extend({'file':str(f),'sha256':hashlib.sha256(f.read_bytes()).hexdigest()} for f in [source,root/'src/bgame/bg_local.h'])
(a.out/'source-manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
