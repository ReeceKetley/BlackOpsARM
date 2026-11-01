"""Actual FDO1 codec guards with native types and a synthetic asset registry.

This fixture does not replace the real device DObj allocator/restart probe.
"""
from pathlib import Path
import argparse, subprocess, sys, json, hashlib

root = Path(__file__).resolve().parents[3]
p = argparse.ArgumentParser()
p.add_argument('--out', type=Path, required=True)
a = p.parse_args()
subprocess.run([sys.executable, str(root/'tools/port/tests/native_audio_mixer_regression.py'), '--out', str(a.out)], check=True)
source = root/'src/game_sp/g_sp_client_models_checkpoint.inl'
s = source.read_text()

def extract(key):
    begin = s.index(key)
    end = s.index('{', begin)+1
    depth = 1
    while depth:
        depth += (s[end] == '{')-(s[end] == '}')
        end += 1
    return s[begin:end]

helpers = r'''
#include <bgame/bg_local.h>
#include <xanim/dobj.h>
#include <database/db_registry.h>
#include <string>
int g_entNumCscEnd = 1280;
static XModel fixtureModel{};
static unsigned short fixtureBones[2] = {1,2};
char *SL_ConvertToString(unsigned id, scriptInstance_t) {
 return const_cast<char *>(id==0?"":id==1?"tag_origin":id==2?"tag_attach":"unknown");
}
XAssetHeader DB_FindXAssetHeader(XAssetType, char *name, bool, int) {
 XAssetHeader h{}; if (!strcmp(name,"fixture_model")) h.model=&fixtureModel; return h;
}
bool DB_IsXAssetDefault(XAssetType,const char *) { return false; }
void Com_Printf(int,const char*,...){}
'''
helpers += '\n'.join(extract(k)+';' for k in ['struct SPClientModelPart', 'struct SPClientModelOwner', 'struct SPClientEntityRegistry'])
helpers += '\n'.join(extract(k) for k in ['static void SP_ClientOwnerName(', 'static void SP_ClientModelRecord(', 'static void SP_ClientEntityRegistryCodec('])
tests = r'''
 fixtureModel.name=const_cast<char *>("fixture_model");fixtureModel.numBones=2;fixtureModel.localBoneNames=fixtureBones;
 static SPClientEntityRegistry registry{},staged{};
 registry.end=1280;registry.count=2;registry.fromMap=1;registry.fromScript=2;
 registry.slots[0].used=registry.slots[255].used=true;
 registry.slots[255].linked=true;registry.slots[255].parent=1024;registry.slots[255].tag="tag_attach";registry.slots[255].tagMode=1;
 registry.slots[255].axis[0][0]=registry.slots[255].axis[1][1]=registry.slots[255].axis[2][2]=1;
 SPClientModelOwner model{};model.handle=1024;
 model.parts={{"fixture_model","",false,255},{"fixture_model","tag_attach",true,1}};
 model.bones={"tag_origin","tag_attach","tag_origin","tag_attach"};model.hide[0]=0x80000000;
 registry.models.push_back(model);model.handle=1536;model.animated=true;registry.models.push_back(model);
 auto modelEncode=[&]{encoded={};SPNativeCheckpointCodec c{nullptr,&encoded};SP_ClientEntityRegistryCodec(c,registry);};
 auto modelDecode=[&]{input={};decoded={};input.buffer=encoded.cacheBuffer;input.bufferSize=encoded.cacheBufferUsed;SPNativeCheckpointCodec c{&input,&decoded};staged={};SP_ClientEntityRegistryCodec(c,staged);};
 modelEncode();modelDecode();check(encoded.cacheBufferUsed==decoded.cacheBufferUsed&&!memcmp(encoded.cacheBuffer,decoded.cacheBuffer,encoded.cacheBufferUsed));
 check(staged.slots[255].used&&staged.slots[255].tag=="tag_attach"&&staged.models[0].parts[0].model=="fixture_model"&&staged.models[1].animated);
 for(int bad:{-1,0,1024,1279,1537}){registry.end=bad;rejects(modelEncode);}registry.end=1280;
 registry.count=1;rejects(modelEncode);registry.count=2;
 registry.slots[256].used=true;registry.count=3;rejects(modelEncode);registry.slots[256].used=false;registry.count=2;
 for(int bad:{-1,MAX_CL_ENTNUM,1279}){registry.slots[255].parent=bad;rejects(modelEncode);}registry.slots[255].parent=1024;
 registry.slots[0].linked=true;registry.slots[0].parent=1279;rejects(modelEncode);registry.slots[0].linked=false;
 registry.slots[255].parent=1025;rejects(modelEncode);registry.slots[255].parent=1024;
 for(float *f:{&registry.slots[0].origin[0],&registry.slots[255].angles[2],&registry.slots[255].axis[3][1],&registry.slots[255].relative[2]}){
  float old=*f;for(float bad:{NAN,INFINITY,-INFINITY}){*f=bad;rejects(modelEncode);}*f=old;
 }
 for(int bad:{-1,MAX_CL_ENTNUM,1025}){registry.models[0].handle=bad;rejects(modelEncode);}registry.models[0].handle=1024;
 registry.models[1].handle=1024;rejects(modelEncode);registry.models[1].handle=1536;
 registry.models[0].parts[0].model="missing";rejects(modelEncode);registry.models[0].parts[0].model="fixture_model";
 registry.models[0].parts[0].tag="tag_origin";rejects(modelEncode);registry.models[0].parts[0].tag.clear();
 registry.models[0].parts[1].tag="missing";rejects(modelEncode);registry.models[0].parts[1].tag="tag_attach";
 for(int bad:{0,2,4,254,255}){registry.models[0].parts[1].parentBone=bad;rejects(modelEncode);}registry.models[0].parts[1].parentBone=1;
 for(int bad:{-2,-1,0,2}){registry.slots[255].tagMode=bad;rejects(modelEncode);}registry.slots[255].tagMode=1;
 registry.models[0].bones[0]="tag_attach";rejects(modelEncode);registry.models[0].bones[0]="tag_origin";
 auto parts=registry.models[0].parts;registry.models[0].parts.clear();rejects(modelEncode);registry.models[0].parts=parts;
 registry.models[0].bones.pop_back();rejects(modelEncode);registry.models[0].bones.push_back("tag_attach");
 modelEncode();int length=encoded.cacheBufferUsed;
 for(int cut=0;cut<length;++cut){input={};decoded={};input.buffer=encoded.cacheBuffer;input.bufferSize=cut;SPNativeCheckpointCodec c{&input,&decoded};staged={};rejects([&]{SP_ClientEntityRegistryCodec(c,staged);});}
 // The slot booleans use the codec's strict one-byte representation.
 for(int bad=2;bad<256;++bad){auto old=encoded.cacheBuffer[20];encoded.cacheBuffer[20]=bad;rejects(modelDecode);encoded.cacheBuffer[20]=old;}
 modelDecode();check(staged.models[0].hide[0]==0x80000000&&staged.models[0].parts[1].ignoreCollision);
 fixtureBones[0]=0;for(auto &o:registry.models){o.bones[0].clear();o.bones[2].clear();}
 modelEncode();modelDecode();check(staged.models[0].bones[0].empty()&&staged.models[1].bones[2].empty());
 registry.models[0].bones[0]="tag_origin";rejects(modelEncode);
 printf("actual FDO1 registry/model guards PASS; ");
'''
f = a.out/'fixture.cpp'
text = f.read_text().replace('static void check(bool b)', helpers+'\nstatic void check(bool b)')
text = text.replace(' printf("actual SDP2 mixer', tests+'\n printf("actual SDP2 mixer')
f.write_text(text, encoding='utf-8')
manifest = json.loads((a.out/'source-manifest.json').read_text())
manifest.append({'file':str(source),'sha256':hashlib.sha256(source.read_bytes()).hexdigest()})
(a.out/'source-manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
