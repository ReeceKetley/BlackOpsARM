#include "rendered_input.h"
#include "window_lifecycle.h"
#include "renderer_window.h"
#include "controller_service.h"
#include "ui_platform.h"
#include <client/cl_keys.h>
#include <ui/ui_shared.h>
#include <client/client.h>
#include <client_mp/cl_input_mp.h>
#include <gfx_d3d/r_dvars.h>
#include <win32/win_input.h>
#include <win32/win_local.h>
#include <win32/win_shared.h>
void Sys_QueEvent(unsigned,sysEventType_t,int,int,int,void *);
#include <SDL.h>
#include <array>
#include <cmath>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <thread>
#include <mutex>
#include <jni.h>
extern const dvar_t *in_mouse;
// Resource-presence query supplied by the SDL lifecycle owner.
namespace bo1::android { bool HasAttachedSDLWindow(); }
namespace bo1::android {
namespace {
bool installed,running,active;
std::thread::id owner;
std::array<int,SDL_NUM_SCANCODES> scanKeys{};
std::array<unsigned,256> held{};
std::array<bool,5> buttons{};
SDL_Window *motionWindow{};
int mouseX{},mouseY{},touchX{},touchY{};
std::int64_t deltaX{},deltaY{};
bool motion,primary,touchCursorPositioned;
SDL_TouchID primaryTouch{};
SDL_FingerID primaryFinger{};
// JNI producers only touch this bounded mailbox. SDL/engine access and all
// key ownership stay on the installed frame owner; no Java object is retained.
// Keep each admitted batch well below the canonical 256-event system queue.
constexpr unsigned touchActionCount=16,touchQueueCapacity=64;
struct TouchEvent { int action{};bool down{};double x{},y{}; };
struct TouchMailbox {
    std::mutex mutex;
    std::array<TouchEvent,touchQueueCapacity> events{};
    unsigned count{};
    int epoch{1},width{},height{};
    Uint32 windowId{};
    bool enabled{},ready{},gameplay{},cancel{};
    double dx{},dy{};
} touch;
std::array<int,touchActionCount> touchHeld{};
double touchRemainderX{},touchRemainderY{};
void invalidateTouchLocked() {
    touch.epoch=touch.epoch==INT_MAX?1:touch.epoch+1;
    touch.ready=false;touch.cancel=true;touch.count=0;touch.dx=touch.dy=0;
}
void invalidateTouch() {
    std::lock_guard<std::mutex> lock(touch.mutex);invalidateTouchLocked();
}

[[noreturn]] void fail(const char *why) { Com_Error(ERR_FATAL,"Native rendered input: %s (%s)",why,SDL_GetError());std::abort(); }
void requireOwner() { if(!installed || owner!=std::this_thread::get_id() || !IsSDLEventOwner())fail("operation requires actual SDL input owner"); }
void queueKey(int key,bool down) { Sys_QueEvent(Sys_Milliseconds(),SE_KEY,key,down?1:0,0,nullptr); }
void retain(int key) { if(key<=0 || key>=256)fail("invalid canonical key code");if(!held[key]++)queueKey(key,true); }
void release(int key) { if(key<=0 || key>=256 || !held[key])fail("unbalanced canonical key release");if(!--held[key])queueKey(key,false); }
void character(int value) { Sys_QueEvent(Sys_Milliseconds(),SE_CHAR,value,0,0,nullptr); }
bool mouseEnabled() { return in_mouse && in_mouse->current.enabled; }
int named(const char *name) { const int key=Key_StringToKeynum(name);if(key<=0 || key>=256)fail("canonical named key unavailable");return key; }
int mapKey(const SDL_Keysym &key) {
    if(key.scancode==SDL_SCANCODE_GRAVE)return '~';
    if(key.sym>=32 && key.sym<127)return key.sym>='A' && key.sym<='Z'?key.sym+32:key.sym;
    struct Mapping { SDL_Keycode code;const char *name; };
    static constexpr Mapping maps[] = {
        {SDLK_TAB,"TAB"},{SDLK_RETURN,"ENTER"},{SDLK_ESCAPE,"ESCAPE"},{SDLK_BACKSPACE,"BACKSPACE"},
        {SDLK_UP,"UPARROW"},{SDLK_DOWN,"DOWNARROW"},{SDLK_LEFT,"LEFTARROW"},{SDLK_RIGHT,"RIGHTARROW"},
        {SDLK_LALT,"ALT"},{SDLK_RALT,"ALT"},{SDLK_LCTRL,"CTRL"},{SDLK_RCTRL,"CTRL"},
        {SDLK_LSHIFT,"SHIFT"},{SDLK_RSHIFT,"SHIFT"},{SDLK_LGUI,"COMMAND"},{SDLK_RGUI,"COMMAND"},
        {SDLK_CAPSLOCK,"CAPSLOCK"},{SDLK_PAUSE,"PAUSE"},{SDLK_INSERT,"INS"},{SDLK_DELETE,"DEL"},
        {SDLK_PAGEDOWN,"PGDN"},{SDLK_PAGEUP,"PGUP"},{SDLK_HOME,"HOME"},{SDLK_END,"END"},
        {SDLK_F1,"F1"},{SDLK_F2,"F2"},{SDLK_F3,"F3"},{SDLK_F4,"F4"},{SDLK_F5,"F5"},{SDLK_F6,"F6"},
        {SDLK_F7,"F7"},{SDLK_F8,"F8"},{SDLK_F9,"F9"},{SDLK_F10,"F10"},{SDLK_F11,"F11"},{SDLK_F12,"F12"},
        {SDLK_KP_7,"KP_HOME"},{SDLK_KP_8,"KP_UPARROW"},{SDLK_KP_9,"KP_PGUP"},{SDLK_KP_4,"KP_LEFTARROW"},
        {SDLK_KP_5,"KP_5"},{SDLK_KP_6,"KP_RIGHTARROW"},{SDLK_KP_1,"KP_END"},{SDLK_KP_2,"KP_DOWNARROW"},
        {SDLK_KP_3,"KP_PGDN"},{SDLK_KP_ENTER,"KP_ENTER"},{SDLK_KP_0,"KP_INS"},{SDLK_KP_PERIOD,"KP_DEL"},
        {SDLK_KP_DIVIDE,"KP_SLASH"},{SDLK_KP_MINUS,"KP_MINUS"},{SDLK_KP_PLUS,"KP_PLUS"},
        {SDLK_NUMLOCKCLEAR,"KP_NUMLOCK"},{SDLK_KP_MULTIPLY,"KP_STAR"},{SDLK_KP_EQUALS,"KP_EQUALS"}
    };
    for(const auto &map:maps)if(map.code==key.sym)return named(map.name);
    return 0;
}
void relative(bool enabled) {
    if(bool(SDL_GetRelativeMouseMode())==enabled)return;
    if(SDL_SetRelativeMouseMode(enabled?SDL_TRUE:SDL_FALSE) || bool(SDL_GetRelativeMouseMode())!=enabled)fail("actual SDL relative mouse mode change failed");
}
void releaseAll() {
    for(unsigned key=1;key<held.size();++key)if(held[key]) { held[key]=0;queueKey(key,false); }
    touchHeld.fill(0);touchRemainderX=touchRemainderY=0;touchCursorPositioned=false;invalidateTouch();
    scanKeys.fill(0);buttons.fill(false);primary=false;motion=false;motionWindow=nullptr;deltaX=deltaY=0;
    if(SDL_WasInit(SDL_INIT_VIDEO)) { relative(false);SDL_StopTextInput(); }
}
void releaseMouse() {
    for(unsigned i=0;i<buttons.size();++i)if(buttons[i]) { buttons[i]=false;release(200+i); }
    if(primary) { primary=false;release(200); }
    motion=false;motionWindow=nullptr;deltaX=deltaY=0;
    if(SDL_WasInit(SDL_INIT_VIDEO))relative(false);
}
bool liveWindow(Uint32 id,SDL_Window **out,RendererWindowInfo *geometry=nullptr) {
    if(!id || !active)return false;
    auto *window=SDL_GetWindowFromID(id);
    WindowLifecycleState state{};
    if(!window || !ReadSDLWindow(window,&state) || state.closing || !state.foreground || !state.focused)return false;
    if(geometry && (!InspectRendererWindow(window,geometry) || geometry->width<=0 || geometry->height<=0 || geometry->drawableWidth<=0 || geometry->drawableHeight<=0))fail("actual window dimensions unavailable");
    *out=window;return true;
}
void accumulate(SDL_Window *window,int x,int y,std::int64_t dx,std::int64_t dy) {
    if(motionWindow && motionWindow!=window)fail("mouse stream changed live window before frame consumption");
    // Each frame passes the canonical signed-int delta contract. Reject a
    // producer overflow instead of narrowing or silently losing movement.
    if(dx>INT_MAX || dx<INT_MIN || dy>INT_MAX || dy<INT_MIN || deltaX+dx>INT_MAX || deltaX+dx<INT_MIN || deltaY+dy>INT_MAX || deltaY+dy<INT_MIN)fail("mouse delta exceeds canonical frame capacity");
    motionWindow=window;mouseX=x;mouseY=y;deltaX+=dx;deltaY+=dy;motion=true;
}
int scale(int value,int from,int to) {
    const auto result=std::int64_t(value)*to/from;
    if(result<INT_MIN || result>INT_MAX)fail("mouse coordinate exceeds native client contract");return static_cast<int>(result);
}
void releaseTouchHeld() {
    for(int &key:touchHeld)if(key) { release(key);key=0; }
    touchRemainderX=touchRemainderY=0;touchCursorPositioned=false;
}
int touchKey(unsigned action,bool gameplay) {
    // Same default binding keys as Com_Init: user's canonical bindings still
    // decide the command. Menu directions/confirm use canonical UI key codes.
    if(action<4) {
        static constexpr int movement[]={'w','s','a','d'};
        static constexpr const char *navigation[]={"UPARROW","DOWNARROW","LEFTARROW","RIGHTARROW"};
        return gameplay?movement[action]:named(navigation[action]);
    }
    if(action==14)return named("ESCAPE");
    if(action==15)return gameplay?0:named("MOUSE1");
    if(!gameplay)return action==6?named("ENTER"):0;
    switch(action) {
    case 4:return named("MOUSE1");case 5:return named("MOUSE2");
    case 6:return named("SPACE");case 7:return 'r';case 8:return 'c';
    case 9:return 'f';case 10:return named("SHIFT");case 11:return '1';
    case 12:return 'v';case 13:return 'g';default:return 0;
    }
}
void frameTouch() {
    // Query actual window/input state on the owner before publishing admission.
    SDL_Window *window=SDL_GetKeyboardFocus();WindowLifecycleState state{};
    RendererWindowInfo geometry{};
    const bool usable=running && active && mouseEnabled() && window
        && ReadSDLWindow(window,&state) && state.focused && state.foreground && !state.closing
        && InspectRendererWindow(window,&geometry) && geometry.width>0 && geometry.height>0
        && geometry.drawableWidth>0 && geometry.drawableHeight>0;
    const bool gameplay=usable && CL_GetLocalClientConnectionState(0)==CA_ACTIVE
        && clientUIActives[0].keyCatchers==0;
    std::lock_guard<std::mutex> lock(touch.mutex);
    const bool ready=usable && touch.enabled;
    const bool changed=ready!=touch.ready || (ready && (touch.windowId!=state.windowId
        || touch.width!=geometry.drawableWidth || touch.height!=geometry.drawableHeight
        || touch.gameplay!=gameplay));
    if(changed)invalidateTouchLocked();
    if(touch.cancel) { releaseTouchHeld();touch.cancel=false; }
    touch.ready=ready;touch.gameplay=gameplay;
    touch.windowId=usable?state.windowId:0;
    touch.width=usable?geometry.drawableWidth:0;touch.height=usable?geometry.drawableHeight:0;
    if(!ready)return;
    // Preserve down/up order even when a complete tap arrived between frames.
    // Holding the mailbox lock makes cancel/admission atomic with consumption.
    for(unsigned i=0;i<touch.count;++i) {
        const auto &event=touch.events[i];
        if(event.action==15 || event.action<0) {
            const int x=static_cast<int>(event.x*(geometry.drawableWidth-1));
            const int y=static_cast<int>(event.y*(geometry.drawableHeight-1));
            // Absolute position and its button edge travel as ONE scalar
            // event. Com_EventLoop applies position immediately before its key,
            // so later clicks/hover cannot move an earlier click's cursor.
            const int type=event.action<0?RenderedTouchHoverEvent
                :(event.down?RenderedTouchDownEvent:RenderedTouchUpEvent);
            Sys_QueEvent(Sys_Milliseconds(),static_cast<sysEventType_t>(type),x,y,touch.epoch,nullptr);
            continue; // Cursor key ownership begins at actual event dispatch.
        }
        if(event.action<0)continue; // Pointer-only motion.
        int &stored=touchHeld[event.action];
        if(event.down) { if(!stored) { stored=touchKey(event.action,gameplay);if(stored)retain(stored); } }
        else if(stored) { release(stored);stored=0; }
    }
    touch.count=0;
    if(gameplay && (touch.dx || touch.dy)) {
        // UI deltas are fractions of the full view. Convert to real drawable
        // pixels, matching physical mouse units, with subpixel carry per epoch.
        const double x=touch.dx*geometry.drawableWidth+touchRemainderX;
        const double y=touch.dy*geometry.drawableHeight+touchRemainderY;
        if(!std::isfinite(x) || !std::isfinite(y) || x>INT_MAX || x<INT_MIN || y>INT_MAX || y<INT_MIN)
            fail("touch delta exceeds canonical frame capacity");
        const int dx=static_cast<int>(x),dy=static_cast<int>(y);
        touchRemainderX=x-dx;touchRemainderY=y-dy;
        accumulate(window,geometry.drawableWidth/2,geometry.drawableHeight/2,dx,dy);
    }
    touch.dx=touch.dy=0;
}
bool validUtf8(const char *text,std::size_t size,std::size_t *length) {
    std::size_t i=0;
    while(i<size && text[i]) {
        const unsigned a=static_cast<unsigned char>(text[i]);
        unsigned count=0,code=0,minimum=0;
        if(a<128){++i;continue;}
        if(a>=0xc2 && a<=0xdf){count=1;code=a&31;minimum=0x80;}
        else if(a>=0xe0 && a<=0xef){count=2;code=a&15;minimum=0x800;}
        else if(a>=0xf0 && a<=0xf4){count=3;code=a&7;minimum=0x10000;}
        else return false;
        if(i+count>=size)return false;
        for(unsigned j=1;j<=count;++j){const unsigned b=static_cast<unsigned char>(text[i+j]);if((b&0xc0)!=0x80)return false;code=(code<<6)|(b&63);}
        if(code<minimum || code>0x10ffff || (code>=0xd800 && code<=0xdfff))return false;
        i+=count+1;
    }
    if(i==size)return false;*length=i;return true;
}
void consume(const SDL_Event &event,void *) {
    requireOwner();if(!running)return;
    if(event.type==SDL_QUIT || event.type==SDL_APP_TERMINATING || event.type==SDL_APP_WILLENTERBACKGROUND || event.type==SDL_APP_DIDENTERBACKGROUND) { releaseAll();return; }
    if(event.type==SDL_WINDOWEVENT) {
        SDL_Window *window=SDL_GetWindowFromID(event.window.windowID);WindowLifecycleState state{};
        if(window && ReadSDLWindow(window,&state) && (!state.focused || !state.foreground || state.closing))releaseAll();
        return;
    }
    SDL_Window *window=nullptr;
    if(event.type==SDL_KEYDOWN || event.type==SDL_KEYUP) {
        const auto &key=event.key;
        if(key.keysym.scancode<=SDL_SCANCODE_UNKNOWN || key.keysym.scancode>=SDL_NUM_SCANCODES)return;
        int &stored=scanKeys[key.keysym.scancode];
        if(event.type==SDL_KEYUP) { if(stored>0)release(stored);stored=0;return; }
        if(!liveWindow(key.windowID,&window))return;
        if(key.keysym.sym==SDLK_RETURN && (key.keysym.mod&KMOD_ALT)) {
            if(!key.repeat && CL_GetLocalClientConnectionState(0)!=CA_LOADING && r_fullscreen && Dvar_GetInt("developer")) { Dvar_SetBool(const_cast<dvar_t *>(r_fullscreen),!r_fullscreen->current.enabled);Cbuf_AddText(0,"vid_restart\n"); }
            stored=-1;return;
        }
        if(stored<0)return;
        if(!stored) { stored=mapKey(key.keysym);if(stored)retain(stored); }
        else if(key.repeat)queueKey(stored,true);
        if(stored==127)character(8);
        else if((key.keysym.mod&KMOD_CTRL) && !(key.keysym.mod&KMOD_ALT)) {
            if(key.keysym.sym==SDLK_a)character(1);else if(key.keysym.sym==SDLK_c)character(3);else if(key.keysym.sym==SDLK_e)character(5);else if(key.keysym.sym==SDLK_v)character(22);
        }
        return;
    }
    if(event.type==SDL_TEXTINPUT) {
        if(!liveWindow(event.text.windowID,&window))return;
        std::size_t length=0;
        if(!validUtf8(event.text.text,sizeof(event.text.text),&length)) { Com_PrintWarning(16,"Rejected malformed SDL committed UTF-8 text\n");return; }
        // Canonical fields store bytes. Preserve the committed UTF-8 byte stream;
        // do not truncate a Unicode code point into a fictional wide-char field.
        for(std::size_t i=0;i<length;++i)character(static_cast<unsigned char>(event.text.text[i]));
        return;
    }
    if(!mouseEnabled()) { releaseMouse();return; }
    if(event.type==SDL_MOUSEBUTTONDOWN || event.type==SDL_MOUSEBUTTONUP) {
        const auto &button=event.button;if(button.which==SDL_TOUCH_MOUSEID)return;
        unsigned index=5;
        if(button.button==SDL_BUTTON_LEFT)index=0;else if(button.button==SDL_BUTTON_RIGHT)index=1;else if(button.button==SDL_BUTTON_MIDDLE)index=2;else if(button.button==SDL_BUTTON_X1)index=3;else if(button.button==SDL_BUTTON_X2)index=4;
        if(index==5)return;
        if(event.type==SDL_MOUSEBUTTONUP) { if(buttons[index]) { buttons[index]=false;release(200+index); }return; }
        if(!liveWindow(button.windowID,&window))return;
        if(!buttons[index]) { buttons[index]=true;retain(200+index); }return;
    }
    if(event.type==SDL_MOUSEMOTION) {
        const auto &m=event.motion;RendererWindowInfo info{};
        if(m.which==SDL_TOUCH_MOUSEID || !liveWindow(m.windowID,&window,&info))return;
        touchCursorPositioned=false; // A real physical mouse takes cursor ownership back.
        accumulate(window,scale(m.x,info.width,info.drawableWidth),scale(m.y,info.height,info.drawableHeight),scale(m.xrel,info.width,info.drawableWidth),scale(m.yrel,info.height,info.drawableHeight));return;
    }
    if(event.type==SDL_MOUSEWHEEL) {
        const auto &wheel=event.wheel;
        if(wheel.which==SDL_TOUCH_MOUSEID || !liveWindow(wheel.windowID,&window))return;
        const auto amount=std::int64_t(wheel.y)*(wheel.direction==SDL_MOUSEWHEEL_FLIPPED?-1:1);
        if(amount>1024 || amount<-1024)fail("wheel event exceeds bounded canonical key queue");
        const int key=amount>0?206:205;
        for(std::int64_t i=0;i<std::abs(amount);++i){queueKey(key,true);queueKey(key,false);}return;
    }
    if(event.type==SDL_FINGERDOWN || event.type==SDL_FINGERUP || event.type==SDL_FINGERMOTION) {
        const auto &finger=event.tfinger;
        if(finger.touchId==SDL_MOUSE_TOUCHID)return;
        const bool same=primary && primaryTouch==finger.touchId && primaryFinger==finger.fingerId;
        if(event.type==SDL_FINGERUP) { if(same) { primary=false;release(200); }return; }
        if(primary && !same)return; // Original desktop client has one cursor.
        RendererWindowInfo info{};
        if(!liveWindow(finger.windowID,&window,&info))return;
        if(!std::isfinite(finger.x) || !std::isfinite(finger.y) || finger.x<0 || finger.x>1 || finger.y<0 || finger.y>1) { Com_PrintWarning(16,"Rejected invalid normalized SDL touch coordinates\n");return; }
        const int x=static_cast<int>(double(finger.x)*(info.drawableWidth-1));
        const int y=static_cast<int>(double(finger.y)*(info.drawableHeight-1));
        const int dx=same?x-touchX:0,dy=same?y-touchY:0;
        touchX=x;touchY=y;
        if(!primary) { if(event.type!=SDL_FINGERDOWN)return;primary=true;primaryTouch=finger.touchId;primaryFinger=finger.fingerId;retain(200);relative(false); }
        accumulate(window,x,y,dx,dy);
    }
}
}
bool InitializeRenderedInput() {
    if(installed) {
        requireOwner();
        if(running){SDL_SetError("rendered input already running");return false;}
        running=true;active=true;InstallAndroidConsoleCommandOwner();return true;
    }
    if(!SDLJavaSetupComplete()) { SDL_SetError("rendered input requires completed actual Java setup and an unbound event owner");return false; }
    // Disable SDK synthesized duplicate streams; primary touch is translated
    // explicitly into the original client's single cursor/button contract.
    if(!SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS,"0") || !SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS,"0")) { SDL_SetError("SDL duplicate touch/mouse stream hints rejected");return false; }
    owner=std::this_thread::get_id();installed=true;running=true;active=true;
    InstallSDLEventOwner(consume,nullptr);
    InstallAndroidConsoleCommandOwner();
    return true;
}
void ActivateRenderedInput(bool value) { requireOwner();if(!running)fail("activation after input retirement");active=value;if(!value)releaseAll(); }
void FrameRenderedInput() {
    requireOwner();if(!running)fail("frame after input retirement");
    frameTouch();
    if(!active)return;
    if(!mouseEnabled())releaseMouse();
    if(!motion && mouseEnabled() && !touchCursorPositioned) {
        SDL_Window *mouseFocus=SDL_GetMouseFocus();WindowLifecycleState snapshot{};
        if(mouseFocus && ReadSDLWindow(mouseFocus,&snapshot) && snapshot.focused && snapshot.foreground && !snapshot.closing) {
            RendererWindowInfo geometry{};
            if(!InspectRendererWindow(mouseFocus,&geometry) || geometry.width<=0 || geometry.height<=0)fail("actual focused mouse geometry unavailable");
            int x{},y{};SDL_GetMouseState(&x,&y);
            accumulate(mouseFocus,scale(x,geometry.width,geometry.drawableWidth),scale(y,geometry.height,geometry.drawableHeight),0,0);
        }
    }
    if(motion) {
        WindowLifecycleState state{};
        if(ReadSDLWindow(motionWindow,&state) && state.focused && state.foreground && !state.closing) {
            const bool recenter=CL_MouseEvent(mouseX,mouseY,static_cast<int>(deltaX),static_cast<int>(deltaY))!=0;
            relative(recenter && !primary);
        } else releaseAll();
        motion=false;deltaX=deltaY=0;
    }
    SDL_Window *focus=SDL_GetKeyboardFocus();WindowLifecycleState state{};
    const bool usable=focus && ReadSDLWindow(focus,&state) && state.focused && state.foreground && !state.closing;
    const int catchers=clientUIActives[0].keyCatchers;
    // UI capture includes ordinary menus. Only the canonical active edit
    // field requests IME; match CL_CharEvent's UI-vs-bind-capture routing.
    const bool editing=usable && ((catchers&(1|0x20))
        || ((catchers&0x10) && !(catchers&0x100) && g_editingField));
    if(editing && !SDL_IsTextInputActive())SDL_StartTextInput();
    else if(!editing && SDL_IsTextInputActive())SDL_StopTextInput();
    if(usable)IN_GamepadsMove();
}
bool DispatchRenderedTouchPointerEvent(int type,unsigned time,int x,int y,int epoch) {
    if(type!=RenderedTouchHoverEvent && type!=RenderedTouchDownEvent && type!=RenderedTouchUpEvent)return false;
    // Scalar-only private events own no allocation, even if the canonical
    // system queue overflows, loading discards events, or input has retired.
    if(!installed || !running)return true;
    requireOwner();
    Uint32 id{};int width{},height{};
    {
        std::lock_guard<std::mutex> lock(touch.mutex);
        if(!touch.ready || touch.gameplay || touch.epoch!=epoch)return true;
        id=touch.windowId;width=touch.width;height=touch.height;
    }
    SDL_Window *window=nullptr;RendererWindowInfo geometry{};
    const bool gameplay=CL_GetLocalClientConnectionState(0)==CA_ACTIVE && clientUIActives[0].keyCatchers==0;
    if(gameplay || !mouseEnabled() || !liveWindow(id,&window,&geometry)
        || geometry.drawableWidth!=width || geometry.drawableHeight!=height
        || x<0 || y<0 || x>=width || y>=height) {
        invalidateTouch();return true;
    }
    const int key=named("MOUSE1");
    int edge=-1;
    {
        std::lock_guard<std::mutex> lock(touch.mutex);
        // UI cancellation can invalidate admission during the actual window
        // query. Pending cursor edges never retain a key before this check.
        if(!touch.ready || touch.gameplay || touch.epoch!=epoch)return true;
        int &stored=touchHeld[15];
        if(type==RenderedTouchDownEvent && !stored) {
            stored=key;if(!held[key]++)edge=1;
        } else if(type==RenderedTouchUpEvent && stored) {
            if(stored!=key || !held[key])fail("unbalanced queued touch cursor release");
            stored=0;if(!--held[key])edge=0;
        }
    }
    // Engine APIs execute only at the real event-consumer point, outside the
    // mailbox lock (canonical UI handlers may synchronously call Java).
    CL_MouseEvent(x,y,0,0);
    touchCursorPositioned=true; // Preserve stationary touch hover until a real handoff/reset.
    if(edge>=0)CL_KeyEvent(0,key,edge,time);
    return true;
}
void ShutdownRenderedInput() {
    requireOwner();if(!running)fail("duplicate rendered input shutdown");
    releaseAll();RetireAndroidConsoleCommandOwner();running=false;active=false;
    RetireRenderedInputEventsIfStopped();
}
void RetireRenderedInputEventsIfStopped() {
    if(!installed)return;requireOwner();
    if(running || HasAttachedSDLWindow() || RendererVideoResourcesOwned())return;
    RetireSDLEventOwner();installed=false;owner={};
}
}
void IN_ShowSystemCursor(bool show) { bo1::android::RequestSDLCursor(show); }

// These JNI methods never call SDL, engine APIs, or retain activity references.
extern "C" JNIEXPORT void JNICALL
Java_com_blackopsarm_dev_EngineActivity_nativeSetTouchControlsEnabled(JNIEnv *,jobject,jboolean enabled) {
    std::lock_guard<std::mutex> lock(bo1::android::touch.mutex);
    if(bo1::android::touch.enabled!=(enabled==JNI_TRUE)) {
        bo1::android::touch.enabled=enabled==JNI_TRUE;bo1::android::invalidateTouchLocked();
    }
}
extern "C" JNIEXPORT jboolean JNICALL
Java_com_blackopsarm_dev_EngineActivity_nativeReadTouchState(JNIEnv *env,jobject,jintArray output) {
    if(!output || env->GetArrayLength(output)<3)return JNI_FALSE;
    jint values[3];
    { std::lock_guard<std::mutex> lock(bo1::android::touch.mutex);
      values[0]=bo1::android::touch.epoch;values[1]=bo1::android::touch.ready;
      values[2]=bo1::android::touch.gameplay; }
    env->SetIntArrayRegion(output,0,3,values);
    return env->ExceptionCheck()?JNI_FALSE:JNI_TRUE;
}
extern "C" JNIEXPORT jboolean JNICALL
Java_com_blackopsarm_dev_EngineActivity_nativeTouchButton(JNIEnv *,jobject,jint epoch,jint action,jboolean down,jfloat x,jfloat y) {
    using namespace bo1::android;
    if(action<0 || action>=int(touchActionCount) || !std::isfinite(x) || !std::isfinite(y)
        || x<0 || x>1 || y<0 || y>1)return JNI_FALSE;
    std::lock_guard<std::mutex> lock(touch.mutex);
    if(!touch.ready || epoch!=touch.epoch)return JNI_FALSE;
    if(touch.count==touchQueueCapacity) { invalidateTouchLocked();return JNI_FALSE; }
    touch.events[touch.count++]={action,down==JNI_TRUE,x,y};return JNI_TRUE;
}
extern "C" JNIEXPORT jboolean JNICALL
Java_com_blackopsarm_dev_EngineActivity_nativeTouchMotion(JNIEnv *,jobject,jint epoch,jboolean look,jfloat x,jfloat y) {
    using namespace bo1::android;
    if(!std::isfinite(x) || !std::isfinite(y))return JNI_FALSE;
    std::lock_guard<std::mutex> lock(touch.mutex);
    if(!touch.ready || epoch!=touch.epoch)return JNI_FALSE;
    if(look==JNI_TRUE) {
        if(!touch.gameplay || x<-1 || x>1 || y<-1 || y>1)return JNI_FALSE;
        const double dx=touch.dx+x,dy=touch.dy+y;
        if(!std::isfinite(dx) || !std::isfinite(dy)
            || std::abs(dx)*touch.width>INT_MAX-1 || std::abs(dy)*touch.height>INT_MAX-1) {
            invalidateTouchLocked();return JNI_FALSE;
        }
        touch.dx=dx;touch.dy=dy;
    } else {
        if(x<0 || x>1 || y<0 || y>1 || touch.gameplay)return JNI_FALSE;
        // Pointer movement coalesces only adjacent motion, never key transitions.
        if(touch.count && touch.events[touch.count-1].action==-1) {
            auto &event=touch.events[touch.count-1];event.x=x;event.y=y;
        } else {
            if(touch.count==touchQueueCapacity) { invalidateTouchLocked();return JNI_FALSE; }
            touch.events[touch.count++]={-1,false,x,y};
        }
    }
    return JNI_TRUE;
}
