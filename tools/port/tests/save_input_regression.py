"""Actual saved-input code, SetClientViewAngle and math; canonical engine types."""
from pathlib import Path
import argparse, hashlib, json
root = Path(__file__).resolve().parents[3]
p = argparse.ArgumentParser()
p.add_argument('--out', type=Path, required=True)
a = p.parse_args()

def body(path, signature):
    s = (root/path).read_bytes().decode('cp1252')
    start = s.index(signature)
    b = s.index('{', start)
    e, depth = b+1, 1
    while depth:
        depth += (s[e] == '{') - (s[e] == '}')
        e += 1
    return s[start:e]

files = ['src/game_sp/g_sp_save_input.inl', 'src/client_mp/g_client_mp.cpp',
         'src/universal/com_math.cpp', 'src/game_mp/g_main_mp.cpp']
actual = '\n'.join(body(path, sig) for path, sig in [
    (files[3], 'gclient_s::gclient_s()'),
    (files[2], 'float AngleNormalize180(float angle)'),
    (files[2], 'double __cdecl AngleNormalize360(float angle)'),
    (files[2], 'double __cdecl AngleDelta(float angle1, float angle2)'),
    (files[1], 'void __cdecl SetClientViewAngle(')])
fixture = r'''
#include <game_mp/g_main_mp.h>
#include <server_mp/sv_main_mp.h>
#include <server/server.h>
#include <qcommon/common.h>
#include <bgame/bg_misc.h>
#include <universal/com_math.h>
#include <stdexcept>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
serverStatic_t svs; level_locals_t level;
const dvar_t *bg_prone_yawcap;
static unsigned checks;
static void check(bool ok){if(!ok){std::fprintf(stderr,"failed check %u\n",checks);std::abort();}++checks;}
void Com_Error(errorParm_t,const char*,...){throw std::runtime_error("expected owner rejection");}
void Com_Printf(int,const char*,...){}
bool Assert_MyHandler(const char*,int,int,const char*,...){std::abort();}
ACTUAL_FUNCTIONS
#include <game_sp/g_sp_save_input.inl>
int main(){
 static gclient_s client; static gentity_s ent,wrongEnt; static client_t transport;
 static dvar_t cap;cap.current.value=45.0f;bg_prone_yawcap=&cap;
 level.clients=&client;level.gentities=&ent;level.maxclients=1;level.time=45000;
 svs.clients=&transport;ent.client=&client;ent.r.inuse=true;transport.gentity=&ent;
 transport.header.state=CS_ACTIVE;
 for(unsigned trial=0;trial<512;++trial){
  client.ps={};client.ps.clientNum=0;client.ps.commandTime=45000+trial*50;
  client.ps.weapon=4;client.ps.weaponTime=713;client.ps.weaponstate=(weaponstate_t)3;
  client.ps.stats[0]=37;client.ps.weapFlags=0x20;client.ps.pm_flags=0;
  client.ps.fWeaponPosFrac=0.75f;
  client.sess.connected=CON_CONNECTED;
  client.sess.cmd={};client.sess.oldcmd={};
  client.sess.cmd.weapon=17;client.sess.cmd.offHandIndex=6;
  client.sess.cmd.lastWeaponAltModeSwitch=99;
  client.sess.cmd.forwardmove=127;client.sess.cmd.rightmove=-127;
  client.sess.cmd.upmove=127;client.sess.cmd.pitchmove=43;client.sess.cmd.yawmove=73;
  client.sess.cmd.rollmove=3;client.sess.cmd.meleeChargeDist=42;
  for(int axis=0;axis<3;++axis){
   client.ps.viewangles[axis]=(float)((trial*(axis+3)*71)%720)-360.0f;
   client.sess.cmd.angles[axis]=777;transport.lastUsercmd.angles[axis]=(trial*12347+axis*17000)&65535;
  }
  for(int word=0;word<2;++word){
   client.sess.cmd.button_bits.array[word]=-1;client.sess.oldcmd.button_bits.array[word]=-1;
   client.button_bits.array[word]=-1;client.oldbutton_bits.array[word]=-1;
   client.latched_button_bits.array[word]=-1;client.button_bitsSinceLastFrame.array[word]=-1;
   transport.lastUsercmd.button_bits.array[word]=-1;
  }
  playerState_s before=client.ps;usercmd_s current=transport.lastUsercmd;
  transport.gentity=(trial&1)?nullptr:&ent;
  transport.header.deltaMessage=17;transport.nextSnapshotTime=90000;transport.lastSnapshotTime=89950;
  int sequence=transport.header.netchan.outgoingSequence;
  G_RebaseInstalledSaveInput();
  check(transport.gentity==&ent && transport.header.state==CS_ACTIVE);
  check(transport.header.deltaMessage==-1 && transport.nextSnapshotTime==level.time && transport.lastSnapshotTime==-1);
  check(transport.header.netchan.outgoingSequence==sequence);
  check(client.ps.commandTime==before.commandTime);check(client.ps.weapon==4);
  check(client.ps.weaponTime==713 && client.ps.weaponstate==3);
  check(client.ps.stats[0]==37 && client.ps.fWeaponPosFrac==0.75f && client.ps.weapFlags==0x20);
  check(client.sess.cmd.weapon==17 && client.sess.cmd.offHandIndex==6);
  check(client.sess.cmd.lastWeaponAltModeSwitch==0);
  check(client.sess.cmd.forwardmove==0 && client.sess.cmd.rightmove==0 && client.sess.cmd.upmove==0);
  check(client.sess.cmd.pitchmove==0 && client.sess.cmd.yawmove==0 && client.sess.cmd.rollmove==0);
  check(client.sess.cmd.meleeChargeDist==0);
  check(client.lastCmdTime==level.time && client.sess.cmd.serverTime==before.commandTime);
  check(std::memcmp(&client.sess.cmd,&client.sess.oldcmd,sizeof(usercmd_s))==0);
  check(std::memcmp(&client.sess.cmd,&transport.lastUsercmd,sizeof(usercmd_s))==0);
  for(int word=0;word<2;++word){
   check(client.sess.cmd.button_bits.array[word]==0 && client.button_bits.array[word]==0);
   check(client.oldbutton_bits.array[word]==0 && client.latched_button_bits.array[word]==0 && client.button_bitsSinceLastFrame.array[word]==0);
  }
  for(int axis=0;axis<3;++axis){
   check(client.sess.cmd.angles[axis]==current.angles[axis]);
   check(client.ps.viewangles[axis]==before.viewangles[axis] && ent.r.currentAngles[axis]==before.viewangles[axis]);
   float reconstructed=current.angles[axis]*(360.0f/65536.0f)+client.ps.delta_angles[axis];
   check(std::fabs(std::remainder(reconstructed-before.viewangles[axis],360.0f))<0.001f);
  }
 }
 // The canonical prone clamp still owns view constraints.
 client.ps.pm_flags=1;client.ps.eFlags=0;client.ps.proneDirection=0;client.ps.proneTorsoPitch=0;
 client.ps.viewangles[0]=80;client.ps.viewangles[1]=170;client.ps.viewangles[2]=0;
 G_RebaseInstalledSaveInput();
 check(std::fabs(client.ps.viewangles[0])<=45.001f && std::fabs(client.ps.viewangles[1])<=45.001f);
 // Disconnected slots are untouched; mismatched real owners fail explicitly.
 client.sess.connected=CON_DISCONNECTED;client.sess.cmd.forwardmove=99;
 G_RebaseInstalledSaveInput();check(client.sess.cmd.forwardmove==99);
 client.sess.connected=CON_CONNECTED;
 for(int bad=0;bad<4;++bad){
  ent.r.inuse=bad!=0;ent.client=bad==1?nullptr:&client;
  transport.header.state=bad==2?CS_FREE:CS_ACTIVE;transport.gentity=bad==3?&wrongEnt:&ent;
  bool rejected=false;try{G_RebaseInstalledSaveInput();}catch(const std::runtime_error&){rejected=true;}
  check(rejected);check(client.sess.cmd.forwardmove==99);
 }
 std::printf("save input %u checks PASS\n",checks);
}
'''
a.out.mkdir(parents=True, exist_ok=True)
(a.out/'fixture.cpp').write_text(fixture.replace('ACTUAL_FUNCTIONS', actual), encoding='utf-8')
(a.out/'source-manifest.json').write_text(json.dumps({f: hashlib.sha256((root/f).read_bytes()).hexdigest() for f in files}, indent=2)+'\n', encoding='utf-8')
