// Native entry integration: actual Com_Init owns engine startup ordering.
#include "native_engine_runtime.h"
#include <platform/common_boot_api.h>
#include <platform/android/command_line_services.h>
#include <platform/android/headless_policy.h>
#include <platform/android/console_input.h>
#include <platform/android/engine_paths.h>
#include <win32/win_localize.h>
#include <universal/timing.h>
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <platform/android/controller_service.h>
#if defined(BO1_NATIVE_FULL_DATABASE)
#include <SDL.h>
#include "system_initialization.h"
#include <client/client.h>
#include <qcommon/common.h>
#include <qcommon/cmd.h>
#include <game_sp/g_sp_savegame.h>
#endif
#include <cerrno>
#include <atomic>
#include <platform/android/quit_services.h>
#include <platform/android/process_guard.h>
#include <platform/android/shutdown_services.h>
#include "launcher_session.h"
#include <platform/android/headless_diagnostics.h>
void Sys_InitializeNativeDiagnostics(const char *commandLine);
extern "C" bool Bo1_ConfigureInitializationTrace(bool rendered);
extern "C" void Bo1_InitializationTraceReturned();

void track_init();
void Sys_FindInfo();
void Com_Frame();
namespace {
std::atomic<bool> initializerReturned{};
int RejectSDLLaunch(const char *reason, int error=0) {
#if defined(__ANDROID__) && defined(BO1_NATIVE_FULL_DATABASE)
    if(error)SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,"ENGINE ENTRY %s (errno %d)",reason,error);
    else SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,"ENGINE ENTRY %s",reason);
#endif
    if(error)std::fprintf(stderr,"ENGINE ENTRY %s (errno %d)\n",reason,error);
    else std::fprintf(stderr,"ENGINE ENTRY %s\n",reason);
    return 1;
}
#if defined(__ANDROID__) && defined(BO1_NATIVE_FULL_DATABASE)
struct CaptureDescriptor {
    int value;
    explicit CaptureDescriptor(int descriptor):value(descriptor) {}
    CaptureDescriptor(const CaptureDescriptor &)=delete;
    CaptureDescriptor &operator=(const CaptureDescriptor &)=delete;
    ~CaptureDescriptor() { if(value>=0)close(value); }
};
int DuplicateCaptureDescriptor(int descriptor) {
    int result;
    do { result=fcntl(descriptor,F_DUPFD_CLOEXEC,3); } while(result<0 && errno==EINTR);
    return result;
}
int MoveCaptureDescriptorAboveStdio(int descriptor) {
    if(descriptor<0 || descriptor>=3)return descriptor;
    const int moved=DuplicateCaptureDescriptor(descriptor);
    const int error=errno;
    close(descriptor);
    errno=error;
    return moved;
}
bool ReplaceCaptureDescriptor(int from, int to) {
    int result;
    do { result=dup2(from,to); } while(result<0 && errno==EINTR);
    return result>=0;
}
#endif
bool CaptureRenderedOutput(const char *userRoot) {
#if defined(__ANDROID__) && defined(BO1_NATIVE_FULL_DATABASE)
    char *resolved=realpath(userRoot,nullptr);
    if(!resolved) { RejectSDLLaunch("cannot resolve SDK output root",errno);return false; }
    int directoryValue;
    do { directoryValue=open(resolved,O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW); } while(directoryValue<0 && errno==EINTR);
    directoryValue=MoveCaptureDescriptorAboveStdio(directoryValue);
    const int directoryError=errno;
    std::free(resolved);
    CaptureDescriptor directory{directoryValue};
    if(directory.value<0) { RejectSDLLaunch("cannot open SDK output root",directoryError);return false; }
    int opened;
    do { opened=openat(directory.value,"native-entry.log",O_WRONLY|O_CREAT|O_APPEND|O_CLOEXEC|O_NOFOLLOW,0600); } while(opened<0 && errno==EINTR);
    CaptureDescriptor log{MoveCaptureDescriptorAboveStdio(opened)};
    if(log.value<0) { RejectSDLLaunch("cannot open SDK native-entry.log",errno);return false; }
    struct stat info{};
    if(fstat(log.value,&info)) { RejectSDLLaunch("cannot inspect SDK native-entry.log",errno);return false; }
    if(!S_ISREG(info.st_mode) || info.st_uid!=geteuid() || info.st_nlink!=1 || (info.st_mode&077)) {
        RejectSDLLaunch("SDK native-entry.log must be a private regular file owned by this app");return false;
    }
    // Sources/backups always stay above fd2. Rejection leaves any initially
    // closed standard descriptor closed; it never installs a fictional stream.
    CaptureDescriptor source{DuplicateCaptureDescriptor(log.value)};
    if(source.value<0) { RejectSDLLaunch("cannot own SDK output descriptor",errno);return false; }
    const int originalOutFlags=fcntl(STDOUT_FILENO,F_GETFD);
    if(originalOutFlags<0) { RejectSDLLaunch("SDK stdout descriptor is unavailable",errno);return false; }
    CaptureDescriptor originalOut{DuplicateCaptureDescriptor(STDOUT_FILENO)};
    if(originalOut.value<0) { RejectSDLLaunch("SDK stdout descriptor is unavailable",errno);return false; }
    CaptureDescriptor originalError{DuplicateCaptureDescriptor(STDERR_FILENO)};
    if(originalError.value<0) { RejectSDLLaunch("SDK stderr descriptor is unavailable",errno);return false; }
    if(!ReplaceCaptureDescriptor(source.value,STDOUT_FILENO)) {
        RejectSDLLaunch("cannot capture SDK stdout",errno);return false;
    }
    if(!ReplaceCaptureDescriptor(source.value,STDERR_FILENO)) {
        const int error=errno;
        // A failed second dup never leaves successful-launch ownership behind.
        if(!ReplaceCaptureDescriptor(originalOut.value,STDOUT_FILENO) ||
           fcntl(STDOUT_FILENO,F_SETFD,originalOutFlags)<0) {
            const int rollbackError=errno;
            close(STDOUT_FILENO);
            RejectSDLLaunch("cannot restore SDK stdout after capture rejection",rollbackError);
        }
        RejectSDLLaunch("cannot capture SDK stderr",error);return false;
    }
    // dup2 transfers the actual append-file description to standard output;
    // all temporary directory/source/backup references close at scope exit.
    std::fprintf(stderr,"ENGINE ENTRY SDK stdout/stderr captured in %s/native-entry.log\n",userRoot);
    return true;
#else
    (void)userRoot;
    RejectSDLLaunch("SDK output capture requires the full native SDL engine");
    return false;
#endif
}
bool InstallAutomaticKinoConsole(const char *userRoot) {
#if defined(__ANDROID__) && defined(BO1_NATIVE_FULL_DATABASE)
    char *resolved=realpath(userRoot,nullptr);
    if(!resolved) { RejectSDLLaunch("cannot resolve automatic Kino console root",errno);return false; }
    int opened;
    do { opened=open(resolved,O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW); } while(opened<0 && errno==EINTR);
    std::free(resolved);
    CaptureDescriptor directory{MoveCaptureDescriptorAboveStdio(opened)};
    if(directory.value<0) { RejectSDLLaunch("cannot open automatic Kino console root",errno);return false; }
    struct stat info{};
    if(fstat(directory.value,&info) || !S_ISDIR(info.st_mode) || info.st_uid!=geteuid() || (info.st_mode&022)) {
        RejectSDLLaunch("automatic Kino console root must be app-owned and not writable by others");return false;
    }
    int created;
    do { created=mkfifoat(directory.value,"native-console.fifo",0600); } while(created<0 && errno==EINTR);
    if(created<0 && errno!=EEXIST) { RejectSDLLaunch("cannot create automatic Kino console FIFO",errno);return false; }
    // RDWR keeps the reader alive between independent test writers. NONBLOCK
    // plus the parser's 4096-byte frame budget prevents input from stalling play.
    do { opened=openat(directory.value,"native-console.fifo",O_RDWR|O_NONBLOCK|O_CLOEXEC|O_NOFOLLOW); } while(opened<0 && errno==EINTR);
    CaptureDescriptor fifo{MoveCaptureDescriptorAboveStdio(opened)};
    if(fifo.value<0) { RejectSDLLaunch("cannot open automatic Kino console FIFO",errno);return false; }
    if(fstat(fifo.value,&info) || !S_ISFIFO(info.st_mode) || info.st_uid!=geteuid() || info.st_nlink!=1 || (info.st_mode&0777)!=0600) {
        RejectSDLLaunch("automatic Kino console must be a private 0600 FIFO owned by this app");return false;
    }
    bo1::android::InstallConsoleInput(fifo.value,true);
    std::fprintf(stderr,"ENGINE ENTRY auto Kino: slash-command FIFO ready at %s/native-console.fifo\n",userRoot);
    return true;
#else
    (void)userRoot;
    RejectSDLLaunch("automatic Kino console requires the full native SDL engine");
    return false;
#endif
}
// The launcher's saved settings (console lines, <user root>/launcher.cfg). Empty when absent.
void ReadLauncherSettings(std::string& text) {
    text.clear();
    const std::string path=std::string(Sys_EngineUserRoot())+"/launcher.cfg";
    if(FILE* file=std::fopen(path.c_str(),"rb")) {
        char buffer[4096];std::size_t read;
        while((read=std::fread(buffer,1,sizeof(buffer),file))>0&&text.size()<65536)text.append(buffer,read);
        std::fclose(file);
        std::fprintf(stderr,"ENGINE ENTRY launcher settings read (%zu bytes)\n",text.size());
    } else std::fputs("ENGINE ENTRY launcher settings file absent; using saved settings\n",stderr);
}
bool ValidateSDLRoot(const char *directory, bool writable) {
    if(!directory || directory[0]!='/' || strnlen(directory,PATH_MAX+1)>PATH_MAX)return false;
    char *resolved=realpath(directory,nullptr);
    if(!resolved)return false;
    struct stat info{};
    const bool valid=stat(resolved,&info)==0 && S_ISDIR(info.st_mode) &&
        access(resolved,R_OK|X_OK|(writable?W_OK:0))==0;
    std::free(resolved);
    return valid;
}
bool ValidateSDLActivity() {
#if defined(__ANDROID__) && defined(BO1_NATIVE_FULL_DATABASE)
    // Do not call SDL's JNI TLS getters until the genuine SDK adoption owner
    // has validated and published its completed Java setup.
    if(!bo1::android::SDLJavaSetupComplete() || !bo1::android::SDLActivityAdopted())return false;
    auto *environment=static_cast<JNIEnv *>(SDL_AndroidGetJNIEnv());
    if(!environment || environment->ExceptionCheck())return false;
    if(environment->PushLocalFrame(8)<0)return false;
    auto activity=static_cast<jobject>(SDL_AndroidGetActivity());
    auto cls=activity && !environment->ExceptionCheck()?environment->GetObjectClass(activity):nullptr;
    auto finishing=cls?environment->GetMethodID(cls,"isFinishing","()Z"):nullptr;
    auto destroyed=cls && !environment->ExceptionCheck()?environment->GetMethodID(cls,"isDestroyed","()Z"):nullptr;
    bool valid=activity && finishing && destroyed && !environment->ExceptionCheck();
    if(valid)valid=!environment->CallBooleanMethod(activity,finishing) && !environment->ExceptionCheck();
    if(valid)valid=!environment->CallBooleanMethod(activity,destroyed) && !environment->ExceptionCheck();
    environment->PopLocalFrame(nullptr);
    return valid;
#else
    return false;
#endif
}
}

int bo1::android::RunNativeEngine(NativeEngineEntry entry, int argc, char **argv) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    unsigned long watchdogSeconds=300;
    if (const char *value=std::getenv("BO1_ENGINE_WATCHDOG_SECONDS")) {
        char *end=nullptr; errno=0;
        const auto parsed=std::strtoul(value,&end,10);
        if(errno || end==value || *end || parsed<1 || parsed>3600) {
            if(entry==NativeEngineEntry::SDLActivity)return RejectSDLLaunch("watchdog seconds must be 1..3600");
            std::fputs("ENGINE ENTRY watchdog seconds must be 1..3600\n",stderr);
            return 1;
        }
        watchdogSeconds=parsed;
    }
    const bool rendered=entry==NativeEngineEntry::SDLActivity;
    if(entry!=NativeEngineEntry::DedicatedConsole && !rendered) {
        std::fputs("ENGINE ENTRY invalid native entry owner\n",stderr);
        return 1;
    }
    if(rendered) {
        if(argc!=4 || !argv || !argv[0] || !*argv[0] || !argv[3] || std::strcmp(argv[3],"--rendered") ||
           !ValidateSDLRoot(argv[1],false) || !ValidateSDLRoot(argv[2],true)) {
            return RejectSDLLaunch("SDL requires SDK program, readable absolute game root, writable absolute user root and --rendered");
        }
        if(!ValidateSDLActivity()) {
            return RejectSDLLaunch("SDL requires actual adopted SDK Activity/JNI context");
        }
    }
    if(rendered && chdir(argv[1])!=0) {
        return RejectSDLLaunch("requires readable designated game root",errno);
    }
    if(rendered && !CaptureRenderedOutput(argv[2]))return 1;
    if(!Bo1_ConfigureInitializationTrace(rendered)) {
        if(rendered)return RejectSDLLaunch("BO1_TRACE_STARTUP must be 0 or 1");
        return 1;
    }
    // Reject SDK prerequisites before spawning the watchdog: SDL_main may
    // return while the Activity process remains alive after a rejected launch.
    std::thread([watchdogSeconds] {
        std::this_thread::sleep_for(std::chrono::seconds(watchdogSeconds));
        if(initializerReturned.load(std::memory_order_acquire)) return;
        std::fputs("ENGINE ENTRY initialization watchdog expired\n", stderr);
        std::_Exit(2);
    }).detach();
    if (!rendered && (argc != 2 || !argv || !argv[0] || !argv[1] || chdir(argv[1]) != 0)) {
        std::fputs("ENGINE ENTRY requires readable designated game root\n", stderr);
        return 1;
    }
    // Select the real entry policy explicitly; do not define BO1_DEDICATED.
    bo1::android::InstallEngineDataRoot(argv[1]);
    if(rendered)bo1::android::InstallEngineUserRoot(argv[2]);
    else {
        const char *userRoot=std::getenv("BO1_ENGINE_USER_ROOT");
        bo1::android::InstallEngineUserRoot(userRoot && *userRoot ? userRoot : "/data/local/tmp/blackopsarm-user-data");
    }
    char commandLine[1024] = "+set bo1_headless 1 +set bo1_zombies 1 +set dedicated 1 +set logfile 2 +set net_ip 127.0.0.1";
    if(rendered)std::strcpy(commandLine,"+set bo1_headless 0 +set bo1_zombies 1 +set dedicated 0 +set r_fullscreen 0 +set r_borderless 1 +set r_aaSamples 1 +set logfile 2 +set net_ip 127.0.0.1");
    // Automatic Kino launches are performance sessions: survive unattended captures.
    // Use the existing spawn-safe test switch instead of timing a toggling god command.
    if(rendered && autoStartKino) {
        std::strcat(commandLine," +set bo1_testclient_god 1");
        std::fputs("ENGINE ENTRY auto Kino: spawn-safe god mode requested\n",stderr);
    }
    bo1::android::InstallEngineCommandLine(argv[0], commandLine);
    // The existing policy must reject rendered admission until the actual
    // provider closure is ready. Never fall back to the verified headless mode.
    Sys_InitializeHeadlessPolicy(commandLine);
    std::fputs("ENGINE ENTRY prerequisite setup\n", stderr);
    Sys_InitializeCriticalSections();
    Sys_InitMainThread();
    Sys_InitializeNativeDiagnostics(commandLine);
    if(!bo1::android::AcquireProcessGuard(Sys_EngineUserRoot())) {
        std::fputs("ENGINE ENTRY another instance owns the actual user-root guard\n",stderr);
        std::_Exit(1);
    }
    if(!rendered)bo1::android::InstallConsoleInput(STDIN_FILENO);
    // The private slash-command FIFO is also offered to manual launches (test
    // tooling sets dvars without the on-screen console); only automatic Kino
    // launches require it.
    else if(!InstallAutomaticKinoConsole(Sys_EngineUserRoot()) && autoStartKino)std::_Exit(1);
    // For SDL, canonical IN_Init inside Com_Init owns InitializeRenderedInput,
    // its real event consumer and Android console producer, then GPad_InitAll.
    // Pre-installing that service here would violate its running-owner checks.
    PMem_Init();
    track_init();
    Win_InitLocalization();
    std::fprintf(stderr, "ENGINE ENTRY localization language: %s\n", Win_GetLanguage());
    Com_InitParse();
    Dvar_Init();
    InitTiming();
    Sys_FindInfo();
    Sys_Milliseconds();
    std::fputs("ENGINE ENTRY calling actual Com_Init\n", stderr);
    Com_Init(commandLine);
    bo1::android::EngineInitializationReturned();
    bo1::android::DiagnosticsInitializationReturned();
    initializerReturned.store(true,std::memory_order_release);
    Bo1_InitializationTraceReturned();
    std::fputs("ENGINE ENTRY actual Com_Init returned\n", stderr);
    // Same outer ownership as WinMain: poll real input, then the engine frame.
    // Real quit/recovery services own teardown; never return with live workers.
    std::fputs("ENGINE ENTRY entering actual input/frame loop\n",stderr);
    bool kinoStartupPending=rendered&&autoStartKino;
    for (;;) {
#if defined(BO1_NATIVE_FULL_DATABASE)
        // Let the real frontend initialize shared assets and the local client.
        // Consume this launch request once: returning to menus later stays there.
        if(kinoStartupPending&&CL_GetLocalClientConnectionState(0)==CA_ACTIVE) {
            kinoStartupPending=false;
            if(Com_IsSPFrontEndRunning()) {
                std::fputs("ENGINE ENTRY auto Kino: active frontend, queueing map zombie_theater once\n",stderr);
                Cbuf_AddText(0,"map zombie_theater\n");
            } else std::fputs("ENGINE ENTRY auto Kino: gameplay already active, no map reload\n",stderr);
        }
        if(rendered) {
            // In-process launcher: publish what the engine is doing so the menu overlay
            // can follow it, start requested maps from the frontend, and idle the engine
            // while the menu is the only thing being looked at.
            namespace session=bo1::android::launcher_session;
            // Save/resume actions run inside Com_Frame's live error boundary.
            const bool recovery=G_SP_IsZombieSessionRecoveryMenu();
            const bool active=CL_GetLocalClientConnectionState(0)==CA_ACTIVE;
            const bool frontend=active&&Com_IsSPFrontEndRunning();
            static bool menuSeen;
            if(frontend)menuSeen=true;
            const session::State published=recovery?session::RecoveryMenu:session::ResumeLoading()?session::Loading:frontend?session::Menu:active?session::Match:menuSeen?session::Loading:session::Booting;
            static int lastPublished=-1;
            if(lastPublished!=int(published)){lastPublished=int(published);std::fprintf(stderr,"ENGINE ENTRY launcher session state %d\n",lastPublished);}
            session::Publish(published);
            if(recovery) std::this_thread::sleep_for(std::chrono::milliseconds(100));
            else if(frontend) {
                std::string requested;
                if(session::TakeRequestedMap(requested)) {
                    std::string commands;
                    ReadLauncherSettings(commands);
                    commands+="\nmap ";commands+=requested;commands+="\n";
                    std::fprintf(stderr,"ENGINE ENTRY launcher menu start: queueing map %s\n",requested.c_str());
                    // The map command runs inside this frame and blocks for the whole load.
                    session::Publish(session::Loading);
                    Cbuf_AddText(0,commands.c_str());
                } else std::this_thread::sleep_for(std::chrono::milliseconds(100)); // menu idle: about 10 frames/s
            }
            IN_Frame_Native();
        } else
#endif
        Platform_InputFrame();
        Com_Frame();
        bo1::android::DiagnosticsFrame();
        if (Sys_IsQuitRequested()) {
            Com_Quit_f();
            std::abort(); // The real quit owner must be terminal.
        }
    }
}


// One-shot launcher start for Com_LoadFrontEnd: the launcher's settings file
// (applied after the startup configs so it overrides config.cfg) followed by
// the map command, in place of the frontend level. Returns false when no
// launcher map was requested or it was already taken (returning to the menus
// later loads the normal frontend).
bool Android_TakeLauncherStart(std::string& commands)
{
    static std::atomic<bool> taken{false};
    if(!bo1::android::autoStartMap[0]||taken.exchange(true))return false;
    ReadLauncherSettings(commands);
    commands+="\nmap ";commands+=bo1::android::autoStartMap;commands+="\n";
    std::fprintf(stderr,"ENGINE ENTRY launcher start: queueing map %s in place of the frontend\n",bo1::android::autoStartMap);
    return true;
}
