"""Actual entry dispatch and saved handoff; transport and respawn providers isolated."""
from pathlib import Path
import argparse, hashlib, json, subprocess, sys
root = Path(__file__).resolve().parents[3]
p = argparse.ArgumentParser()
p.add_argument('--out', type=Path, required=True)
a = p.parse_args()
subprocess.run([sys.executable, str(root/'tools/port/tests/save_input_regression.py'),
                '--out', str(a.out)], check=True)
s = (root/'src/client_mp/sv_client_mp.cpp').read_text(encoding='utf-8')
start = s.index('void __cdecl SV_ClientEnterWorld(')
b = s.index('{', start)
end, depth = b+1, 1
while depth:
    depth += (s[end] == '{') - (s[end] == '}')
    end += 1
entry = s[start:end]
def body(path, signature):
    source = (root/path).read_bytes().decode('cp1252')
    start = source.index(signature)
    b = source.index('{', start)
    end, depth = b+1, 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]
local_input = body('src/client_mp/cl_input_mp.cpp', 'void __cdecl CL_ClearKeys(')+'\n'+body(
    'src/client_mp/cl_parse_mp.cpp', 'static void CL_ClearSavedGameInput(')
providers = r'''
#include <initializer_list>
#include <clientscript/cscr_variable.h>
#include <client_mp/cl_input_mp.h>
kbutton_t playersKb[1][47];
server_t sv;
const dvar_t *com_maxclients;
int sv_serverId_value;
static unsigned begins,sends;
static bool refuseSend;
static bool freezeGate, pairedGate=true, mainThread=true;
static int clientFrames=1200;
scrVarPub_t gScrVarPub[2];
bool Sys_IsMainThread(){return mainThread;}
bool Dvar_GetBool(const char *name){
 if(!strcmp(name,"bo1_restore_freeze_world"))return freezeGate;
 if(!strcmp(name,"bo1_restore_saved_client"))return pairedGate;
 return false;
}
int Dvar_GetInt(const char*){return clientFrames;}
static bool s_continuousZombieResume, s_publicZombieCapture;
bool G_SP_IsContinuousZombieResume(){return s_continuousZombieResume;}
ACTUAL_CHECKPOINT_POLICY
void Com_DPrintf(int,const char*,...){}
void ClientBegin(unsigned int i){++begins;level.clients[i].ps.stats[0]=-999;}
void SV_SendClientGameState(client_t*c){++sends;if(!refuseSend)c->header.state=CS_CLIENTLOADING;}
#include <game_sp/g_sp_save_handshake.inl>
ACTUAL_ENTRY
ACTUAL_LOCAL_INPUT
'''
tests = r'''
 static gclient_s players[2];static gentity_s entities[2];static client_t connections[2];
 static dvar_t maxClients;maxClients.current.integer=2;com_maxclients=&maxClients;
 auto initialize=[&]{
  G_SP_ClearSavedClientHandshake();level.clients=players;level.gentities=entities;
  level.maxclients=2;level.time=45000;svs.clients=connections;svs.time=level.time;
  sv.gentities=entities;sv.gentitySize=sizeof(gentity_s);sv_serverId_value=19;
  for(int i=0;i<2;++i){
   players[i].ps={};players[i].sess.connected=CON_DISCONNECTED;
   connections[i].header.state=CS_FREE;connections[i].gentity=nullptr;
   entities[i].r.inuse=false;entities[i].client=&players[i];
  }
  auto &p=players[0];auto &e=entities[0];auto &c=connections[0];
  p.sess.connected=CON_CONNECTED;p.ps.clientNum=0;p.ps.commandTime=45000;
  p.ps.stats[0]=37;p.ps.weapon=4;p.ps.weaponTime=713;p.ps.fWeaponPosFrac=0.75f;
  p.sess.cmd={};p.sess.cmd.weapon=17;p.sess.cmd.offHandIndex=6;
  e.r.inuse=true;c.gentity=&e;c.header.state=CS_ACTIVE;
  c.header.netchan.remoteAddress.type=NA_LOOPBACK;c.challenge=1234;c.scriptId=567;c.serverId=sv_serverId_value;
  c.lastUsercmd={};c.lastUsercmd.angles[1]=14000;
  begins=0;sends=0;refuseSend=false;
  freezeGate=false;pairedGate=true;mainThread=true;clientFrames=1200;
  s_continuousZombieResume=false;s_publicZombieCapture=false;
  level.framenum=123;gScrVarPub[SCRIPTINSTANCE_SERVER].time=900;
 };
 // Real entry dispatch with no pending restore retains the ordinary spawn path.
 initialize();usercmd_s first={};first.serverTime=125;first.angles[1]=9000;
 connections[0].header.state=CS_CLIENTLOADING;
 SV_ClientEnterWorld(&connections[0],&first);
 check(begins==1 && players[0].ps.stats[0]==-999 && connections[0].header.state==CS_ACTIVE);
 check(connections[0].lastUsercmd.serverTime==125 && connections[0].lastUsercmd.angles[1]==9000);
 // Across the gamestate reset, first command angles replace the old reference.
 for(unsigned trial=0;trial<256;++trial){
  initialize();auto &p=players[0];auto &c=connections[0];
  c.header.netchan.remoteAddress.type=(trial&1)?NA_LOOPBACK:NA_BOT;
  for(int axis=0;axis<3;++axis)p.ps.viewangles[axis]=(float)((trial*(axis+3)*71)%720)-360;
  G_RebaseInstalledSaveInput();playerState_s before=p.ps;
  G_BeginSavedClientHandshake();check(sends==1 && c.header.state==CS_CLIENTLOADING && begins==0);
  first={};first.serverTime=0;first.weapon=0;first.offHandIndex=0;
  first.button_bits.array[0]=-1;first.forwardmove=127;
  for(int axis=0;axis<3;++axis)first.angles[axis]=(trial*12013+axis*19000)&65535;
  SV_ClientEnterWorld(&c,&first);
  check(begins==0 && c.header.state==CS_ACTIVE && c.gentity==&entities[0]);
  check(c.header.deltaMessage==-1 && c.nextSnapshotTime==45000 && c.lastSnapshotTime==-1);
  check(c.lastUsercmd.serverTime==45000 && p.ps.commandTime==45000);
  check(p.ps.stats[0]==37 && p.ps.weapon==4 && p.ps.weaponTime==713 && p.ps.fWeaponPosFrac==0.75f);
  check(p.sess.cmd.weapon==17 && p.sess.cmd.offHandIndex==6 && c.lastUsercmd.weapon==17);
  check(c.lastUsercmd.button_bits.array[0]==0 && c.lastUsercmd.forwardmove==0);
  for(int axis=0;axis<3;++axis){
   check(p.ps.viewangles[axis]==before.viewangles[axis] && p.sess.cmd.angles[axis]==first.angles[axis]);
   float reconstructed=first.angles[axis]*(360.0f/65536.0f)+p.ps.delta_angles[axis];
   check(std::fabs(std::remainder(reconstructed-before.viewangles[axis],360.0f))<0.001f);
  }
  check(!G_SP_TryEnterSavedClient(&c,&first)); // handoff consumed exactly once
 }
 // Cancellation on world replacement and connection loss leaves normal entry.
 for(int cancel=0;cancel<2;++cancel){
  initialize();G_BeginSavedClientHandshake();
  if(cancel)G_SP_CancelSavedClientHandshake(&connections[0]);else G_SP_ClearSavedClientHandshake();
  SV_ClientEnterWorld(&connections[0],&first);check(begins==1);
 }
 initialize();G_BeginSavedClientHandshake();
 G_SP_CancelSavedClientHandshake(&connections[1]);
 check(G_SP_TryEnterSavedClient(&connections[0],&first) && begins==0);
 // Identity/state/ownership failures do not fall through to ClientBegin.
 for(int bad=0;bad<11;++bad){
  initialize();G_BeginSavedClientHandshake();
  switch(bad){
   case 0:++sv_serverId_value;break;
   case 1:++connections[0].challenge;break;
   case 2:++connections[0].scriptId;break;
   case 3:connections[0].header.state=CS_ACTIVE;break;
   case 4:connections[0].gentity=&entities[1];break;
   case 5:entities[0].r.inuse=false;break;
   case 6:entities[0].client=&players[1];break;
   case 7:players[0].sess.connected=CON_DISCONNECTED;break;
   case 8:players[0].ps.clientNum=1;break;
   case 9:level.gentities=&entities[1];break;
   case 10:++connections[0].serverId;break;
  }
  bool rejected=false;try{SV_ClientEnterWorld(&connections[0],&first);}catch(const std::runtime_error&){rejected=true;}
  check(rejected && begins==0 && players[0].ps.stats[0]==37);
 }
 // Reject unsupported sessions before transmitting any gamestate.
 for(int bad=0;bad<5;++bad){
  initialize();switch(bad){
   case 0:connections[0].header.netchan.remoteAddress.type=NA_IP;break;
   case 1:players[1].sess.connected=CON_CONNECTED;connections[1].header.state=CS_ACTIVE;break;
   case 2:players[0].sess.connected=CON_CONNECTING;break;
   case 3:connections[0].gentity=nullptr;break;
   case 4:players[0].sess.connected=CON_DISCONNECTED;connections[0].header.state=CS_FREE;break;
  }
  bool rejected=false;try{G_BeginSavedClientHandshake();}catch(const std::runtime_error&){rejected=true;}
  check(rejected && sends==0 && begins==0);
 }
 initialize();G_BeginSavedClientHandshake();
 bool duplicate=false;try{G_BeginSavedClientHandshake();}catch(const std::runtime_error&){duplicate=true;}
 check(duplicate && sends==1);
 initialize();refuseSend=true;bool rejected=false;
 try{G_BeginSavedClientHandshake();}catch(const std::runtime_error&){rejected=true;}
 check(rejected && begins==0);G_SP_ClearSavedClientHandshake();
 // The real local key-owner clear is restricted to the saved loopback join.
 initialize();
 auto fillKeys=[](){for(auto &key:playersKb[0]){key.down[0]=11;key.down[1]=12;key.active=true;key.wasPressed=true;key.msec=99;key.val=0.75f;}};
 fillKeys();CL_ClearSavedGameInput(0,NA_LOOPBACK);check(playersKb[0][0].active);
 G_BeginSavedClientHandshake();check(G_SP_IsSavedClientHandshakePending());
 for(auto address:{NA_BOT,NA_IP,NA_BAD}){CL_ClearSavedGameInput(0,address);check(playersKb[0][0].active);}
 CL_ClearSavedGameInput(1,NA_LOOPBACK);check(playersKb[0][0].active);
 CL_ClearSavedGameInput(0,NA_LOOPBACK);
 for(auto &key:playersKb[0])check(key.down[0]==0 && key.down[1]==0 && !key.active && !key.wasPressed && key.msec==0 && key.val==0);
 G_SP_ClearSavedClientHandshake();check(!G_SP_IsSavedClientHandshakePending());
 // Frozen reconnect is distinct from the one-use entry claim. Real entry
 // completes while time/frame/script stay owned until CLIENT publication.
 initialize();freezeGate=true;
 G_BeginSavedClientHandshake();check(G_SP_IsSavedWorldFrozen());
 rejected=false;try{G_SP_ReleaseSavedWorldFreeze();}catch(...){rejected=true;}
 check(rejected && G_SP_IsSavedWorldFrozen());
 SV_ClientEnterWorld(&connections[0],&first);
 check(!G_SP_IsSavedClientHandshakePending() && G_SP_IsSavedWorldFrozen() && begins==0);
 for(int fault=0;fault<5;++fault){
  level.time=45000;level.framenum=123;gScrVarPub[SCRIPTINSTANCE_SERVER].time=900;
  mainThread=true;connections[0].header.state=CS_ACTIVE;
  if(fault==0)++level.time;
  if(fault==1)++level.framenum;
  if(fault==2)++gScrVarPub[SCRIPTINSTANCE_SERVER].time;
  if(fault==3)mainThread=false;
  if(fault==4)connections[0].header.state=CS_CLIENTLOADING;
  rejected=false;try{G_SP_ReleaseSavedWorldFreeze();}catch(...){rejected=true;}
  check(rejected && G_SP_IsSavedWorldFrozen());
 }
 level.time=45000;level.framenum=123;gScrVarPub[SCRIPTINSTANCE_SERVER].time=900;
 mainThread=true;connections[0].header.state=CS_ACTIVE;
 G_SP_ReleaseSavedWorldFreeze();G_SP_ReleaseSavedWorldFreeze();
 check(!G_SP_IsSavedWorldFrozen() && !G_SP_IsSavedClientHandshakePending());
 initialize();freezeGate=true;G_BeginSavedClientHandshake();
 SV_ClientEnterWorld(&connections[0],&first);
 G_SP_CancelSavedClientHandshake(&connections[1]);check(G_SP_IsSavedWorldFrozen());
 G_SP_CancelSavedClientHandshake(&connections[0]);check(!G_SP_IsSavedWorldFrozen());
 initialize();freezeGate=true;pairedGate=false;rejected=false;
 try{G_BeginSavedClientHandshake();}catch(...){rejected=true;}
 check(rejected && !G_SP_IsSavedWorldFrozen() && !G_SP_IsSavedClientHandshakePending());
 initialize();freezeGate=true;clientFrames=0;rejected=false;
 try{G_BeginSavedClientHandshake();}catch(...){rejected=true;}
 check(rejected && !G_SP_IsSavedWorldFrozen());
 initialize();freezeGate=true;connections[0].header.netchan.remoteAddress.type=NA_BOT;rejected=false;
 try{G_BeginSavedClientHandshake();}catch(...){rejected=true;}
 check(rejected && !G_SP_IsSavedWorldFrozen());
 // Continuous operation selects the full logical path without changing the
 // developer switches, acquiring their quit behavior or relaxing ownership.
 initialize();s_continuousZombieResume=true;freezeGate=false;pairedGate=false;clientFrames=0;
 check(G_SP_CheckpointOption("bo1_restore_freeze_world") && G_SP_CheckpointOption("bo1_restore_saved_client"));
 check(!Dvar_GetBool("bo1_restore_freeze_world") && !Dvar_GetBool("bo1_restore_saved_client"));
 check(!G_SP_CheckpointOption("bo1_restore_verify_quit") && !G_SP_CheckpointOption("unknown_option"));
 G_BeginSavedClientHandshake();check(G_SP_IsSavedWorldFrozen() && G_SP_IsSavedClientHandshakePending());
 SV_ClientEnterWorld(&connections[0],&first);G_SP_ReleaseSavedWorldFreeze();
 check(!G_SP_IsSavedWorldFrozen() && begins==0);
 initialize();s_publicZombieCapture=true;
 check(G_SP_CheckpointOption("bo1_client_checkpoint") && !Dvar_GetBool("bo1_client_checkpoint"));
 check(!G_SP_CheckpointOption("bo1_restore_animation") && !G_SP_CheckpointOption("bo1_restore_verify_quit"));
 // A late save must not expire the real new-process channel. Keep genuine
 // timestamp ages (including already expired clients) and timeout history.
 for(int savedTime:{50,249000,3600000}){
  initialize();auto &c=connections[0];c.lastPacketTime=19950;c.lastConnectTime=19000;c.timeoutCount=4;
  connections[1].lastPacketTime=-99;connections[1].lastConnectTime=-98;
  G_RebaseSavedTransportClock(connections,2,20000,savedTime);
  check(savedTime-c.lastPacketTime==50 && savedTime-c.lastConnectTime==1000 && c.timeoutCount==4);
  check(connections[1].lastPacketTime==-99 && connections[1].lastConnectTime==-98);
  c.lastPacketTime=20000-200000;G_RebaseSavedTransportClock(connections,2,20000,savedTime);
  check(savedTime-c.lastPacketTime==200000 && c.timeoutCount==4);
 }
 initialize();connections[0].lastPacketTime=2147483647;connections[0].lastConnectTime=1;
 rejected=false;try{G_RebaseSavedTransportClock(connections,2,0,249000);}catch(...){rejected=true;}
 check(rejected && connections[0].lastPacketTime==2147483647 && connections[0].lastConnectTime==1);
 // A paired restore returns the original command-angle epoch after reconnect.
 // The fresh reference must not rotate the first saved-history live command.
 for(unsigned trial=0;trial<256;++trial){
  initialize();freezeGate=true;auto &p=players[0];auto &c=connections[0];
  for(int axis=0;axis<3;++axis){
   p.sess.cmd.angles[axis]=(trial*(axis+1)*127+9000)&65535;
   p.ps.viewangles[axis]=(float)((trial*(axis+3)*71)%360)-180;
  }
  SetClientViewAngle(&entities[0],p.ps.viewangles);auto original=p.ps;
  int angles[3];memcpy(angles,p.sess.cmd.angles,sizeof(angles));
  G_RebaseInstalledSaveInput();G_BeginSavedClientHandshake();
  bool rejected=false;try{G_RestoreSavedClientInputEpoch();}catch(...){rejected=true;}
  check(rejected&&s_savedPairedInputEpoch);
  usercmd_s fresh{};fresh.serverTime=1;fresh.angles[0]=500;fresh.angles[1]=30000;fresh.angles[2]=100;
  SV_ClientEnterWorld(&c,&fresh);G_RestoreSavedClientInputEpoch();
  check(!s_savedPairedInputEpoch&&G_SP_IsSavedWorldFrozen());
  check(!memcmp(p.ps.delta_angles,original.delta_angles,sizeof(original.delta_angles)));
  check(!memcmp(p.ps.viewangles,original.viewangles,sizeof(original.viewangles)));
  check(p.ps.commandTime==original.commandTime&&p.ps.weapon==original.weapon&&p.ps.weaponTime==original.weaponTime);
  check(p.sess.cmd.forwardmove==0&&p.sess.cmd.rightmove==0&&p.sess.cmd.button_bits.array[0]==0&&p.sess.cmd.button_bits.array[1]==0);
  check(!memcmp(&p.sess.cmd,&p.sess.oldcmd,sizeof(p.sess.cmd))&&!memcmp(&c.lastUsercmd,&p.sess.cmd,sizeof(p.sess.cmd)));
  for(int axis=0;axis<3;++axis){
   check(p.sess.cmd.angles[axis]==angles[axis]);
   const auto view=angles[axis]*(360.0f/65536.0f)+p.ps.delta_angles[axis];
   check(std::fabs(std::remainder(view-original.viewangles[axis],360.0f))<0.001f);
  }
  rejected=false;try{G_RestoreSavedClientInputEpoch();}catch(...){rejected=true;}check(rejected);
  G_SP_ReleaseSavedWorldFreeze();
 }
 std::printf("save handshake %u combined checks PASS\n",checks);
'''
f = a.out/'fixture.cpp'
policy = body('src/game_sp/g_sp_savegame.cpp', 'bool G_SP_CheckpointOption(')
fixture = f.read_text().replace('int main(){', providers.replace('ACTUAL_ENTRY', entry).replace('ACTUAL_LOCAL_INPUT', local_input).replace('ACTUAL_CHECKPOINT_POLICY',policy)+'\nint main(){')
fixture = fixture.replace(' std::printf("save input %u checks PASS\\n",checks);', tests)
f.write_text(fixture, encoding='utf-8')
manifest = json.loads((a.out/'source-manifest.json').read_text())
for name in ['src/game_sp/g_sp_save_handshake.inl','src/client_mp/sv_client_mp.cpp',
             'src/client_mp/cl_parse_mp.cpp','src/client_mp/cl_input_mp.cpp',
             'src/game_sp/g_sp_savegame.h','tools/port/tests/save_input_regression.py']:
    manifest[name] = hashlib.sha256((root/name).read_bytes()).hexdigest()
(a.out/'source-manifest.json').write_text(json.dumps(manifest, indent=2)+'\n', encoding='utf-8')
