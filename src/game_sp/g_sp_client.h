#pragma once

void G_SP_ResetClientConnects();
void G_SP_SetClientConnectPending(unsigned int clientNum, bool pending);
void SV_RunClientConnectCallbacks();
void G_SP_HeadlessTestClientFrame(bool afterFrame);
// Bounded save diagnostic: retain the decoded bot/player instead of admitting
// another test client after G_InitGame reset the harness bookkeeping.
void G_SP_HeadlessAdoptSavedClient();
void G_SP_HeadlessClientBegin(unsigned int clientNum);
bool G_SP_DeferHeadlessTestClientBegin();
struct client_t;
struct usercmd_s;
struct playerState_s;
bool G_SP_HeadlessFightCommand(client_t *client, usercmd_s *cmd);
bool G_SP_HeadlessLocalClientCommand(usercmd_s *cmd, const playerState_s *predictedPs);
bool G_SP_TestClientRealWindow();
bool G_SP_TestLocalClient();
