#include <game_mp/ui_gameinfo_mp.h>
#include <universal/com_files.h>
#include <qcommon/common.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <vector>

extern int ui_numArenas;
extern char *ui_arenaInfos[128];
static FILE *input;
static int advertised, closes, reads, parses, expectedBytes;

// Isolate the changed read/close/termination boundary from initialized engine
// filesystem and parser state. Payload reads and closes use an actual temp file.
int FS_FOpenFileRead(const char *name, int *handle) {
    if (std::strcmp(name, "mod.arena")) std::abort();
    *handle = input ? 1 : 0;
    return advertised;
}
unsigned int FS_Read(unsigned char *buffer, unsigned int length, int handle) {
    if (handle != 1 || !input) std::abort();
    ++reads;
    return std::fread(buffer, 1, length, input);
}
void FS_FCloseFile(int handle) {
    if (handle != 1 || !input) std::abort();
    ++closes;
    if (std::fclose(input)) std::abort();
    input = nullptr;
}
void Com_PrintWarning(int, const char *, ...) {}
extern "C" int __wrap__Z13UI_ParseInfosPKciPPc(const char *buffer, int count, char **infos) {
    if (input || closes != 1 || count != 125 || infos != &ui_arenaInfos[3]) std::abort();
    for (int i = 0; i < expectedBytes; ++i) if (buffer[i] != 'a') std::abort();
    if (buffer[expectedBytes] != 0) std::abort();
    ++parses;
    return 7;
}
static void Prepare(int reportedSize, int actualSize) {
    advertised = reportedSize;
    expectedBytes = actualSize;
    closes = reads = parses = 0;
    ui_numArenas = 3;
    input = std::tmpfile();
    if (!input) std::abort();
    std::vector<char> bytes(actualSize, 'a');
    if (std::fwrite(bytes.data(), 1, bytes.size(), input) != bytes.size()) std::abort();
    std::rewind(input);
}
int main() {
    input = nullptr;
    if (UI_LoadModArenas() || closes || reads || parses) return 1;
    for (int bad : {-1, 0, 8193}) {
        Prepare(bad, 0);
        if (UI_LoadModArenas() || closes != 1 || reads || parses || input || ui_numArenas != 3) return 2;
    }
    Prepare(8192, 17);
    if (UI_LoadModArenas() || closes != 1 || reads != 1 || parses || input || ui_numArenas != 3) return 3;
    for (int length : {5, 8192}) {
        Prepare(length, length);
        if (UI_LoadModArenas() != 1 || closes != 1 || reads != 1 || parses != 1 || input || ui_numArenas != 7) return 4;
    }
    std::puts("PASS canonical custom-arena read/close/termination: missing, invalid sizes, short read, exact 8192-byte payload");
}
