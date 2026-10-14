#pragma once
struct MessageLine // sizeof=0x18
{                                       // XREF: MessageBuffer/r
                                        // MessageBuffer/r ...
    int messageIndex;
    int textBufPos;
    int textBufSize;
    int typingStartTime;
    int lastTypingSoundTime;
    int flags;
};

struct Message // sizeof=0x8
{                                       // XREF: MessageBuffer/r
                                        // MessageBuffer/r ...
    int startTime;
    int endTime;
};

struct MessageWindow // sizeof=0x34
{                                       // XREF: MessageBuffer/r
                                        // MessageBuffer/r ...
    MessageLine *lines;                 // XREF: Con_GetTextCopy(char *,int)+23/r
                                        // Con_DrawOutputText+BE/r
    Message *messages;
    char *circularTextBuffer;           // XREF: Con_GetTextCopy(char *,int)+9B/r
                                        // Con_GetTextCopy(char *,int)+BC/r ...
    int textBufSize;                    // XREF: Con_GetTextCopy(char *,int)+47/r
                                        // Con_GetTextCopy(char *,int)+6F/r ...
    int lineCount;                      // XREF: Con_DrawMiniConsole(int,int,int,float)+7F/r
                                        // Con_DrawMiniConsole(int,int,int,float)+92/w ...
    int padding;
    int scrollTime;
    int fadeIn;
    int fadeOut;
    int textBufPos;                     // XREF: Con_GetTextCopy(char *,int)+30/r
    int firstLineIndex;                 // XREF: Con_GetTextCopy(char *,int):loc_54555A/r
                                        // Con_DrawOutputText+A3/r
    int activeLineCount;                // XREF: Con_GetTextCopy(char *,int)+6/r
                                        // Con_FreeFirstMessageWindowLine+8F/r ...
    int messageIndex;
};

struct MessageBuffer // sizeof=0x3EA4
{                                       // XREF: Console/r
    char gamemsgText[3][2048];
    MessageWindow gamemsgWindows[3];
    MessageLine gamemsgLines[3][12];
    Message gamemsgMessages[3][12];
    char miniconText[4096];
    MessageWindow miniconWindow;        // XREF: Con_DrawMiniConsole(int,int,int,float)+7F/r
                                        // Con_DrawMiniConsole(int,int,int,float)+92/w
    MessageLine miniconLines[100];
    Message miniconMessages[100];
    char errorText[1024];
    MessageWindow errorWindow;
    MessageLine errorLines[5];
    Message errorMessages[5];
};

struct Console // sizeof=0x14118
{                                       // XREF: .data:con/r
    int initialized;                    // XREF: Con_OneTimeInit(void)+C47/w
                                        // Con_Init(void)+15F/r ...
    MessageWindow consoleWindow;        // XREF: Con_GetTextCopy(char *,int)+6/r
                                        // Con_GetTextCopy(char *,int):loc_54555A/r ...
    MessageLine consoleLines[1024];     // XREF: Con_OneTimeInit(void)+BE6/o
    Message consoleMessages[1024];      // XREF: Con_OneTimeInit(void)+BEB/o
    char consoleText[32768];            // XREF: Con_OneTimeInit(void)+BE1/o
    char textTempLine[512];             // XREF: CL_ConsolePrint_AddLine+369/w
                                        // CL_ConsolePrint_AddLine+386/w ...
    unsigned int lineOffset;            // XREF: Con_Clear_f+10/w
                                        // CL_ConsolePrint_AddLine+B1/r ...
    int displayLineOffset;              // XREF: Con_Clear_f+1A/w
                                        // Con_FreeFirstMessageWindowLine+72/r ...
    int prevChannel;                    // XREF: CL_ConsolePrint_AddLine+A9/r
                                        // CL_ConsolePrint_AddLine+BE/r ...
    bool outputVisible;                 // XREF: Con_ToggleConsole(void)+3F/w
                                        // Con_ToggleConsoleOutput(void)+3/r ...
    // padding byte
    // padding byte
    // padding byte
    int fontHeight;                     // XREF: Con_CheckResize(void)+11B/w
                                        // Con_CheckResize(void)+120/r ...
    int visibleLineCount;               // XREF: Con_CheckResize(void)+18D/w
                                        // Con_CheckResize(void)+1D4/w ...
    int visiblePixelWidth;              // XREF: Con_CheckResize(void)+1C2/w
                                        // Con_CheckResize(void)+1DE/w ...
    float screenMin[2];                 // XREF: Con_CheckResize(void)+39/w
                                        // Con_CheckResize(void)+72/w ...
    float screenMax[2];                 // XREF: Con_CheckResize(void)+B6/w
                                        // Con_CheckResize(void)+FA/w ...
    MessageBuffer messageBuffer[1];     // XREF: Con_InitMessageBuffer(void)+2B/o
                                        // Con_DrawMiniConsole(int,int,int,float)+7F/r ...
    float color[4];                     // XREF: Con_OneTimeInit(void)+C0A/w
                                        // Con_OneTimeInit(void)+C1A/w ...
};


extern Console con;
