#pragma once
// Requires actual successful SDL Java setup and a live application Activity.
bool Platform_LaunchSteamURI(const char *uri);
// Worker-thread call waits for actual dismissal. UI-thread calls are rejected.
void Platform_ShowSteamError(const char *title, const char *message);
