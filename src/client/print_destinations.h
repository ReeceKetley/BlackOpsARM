#pragma once

enum print_msg_dest_t : __int32
{                                       // XREF: ?Con_IsChannelVisible@@YA_NW4print_msg_dest_t@@HH@Z/r
                                        // Con_InitChannelsForDestFromList/r ...
    CON_DEST_CONSOLE    = 0x0,
    CON_DEST_MINICON    = 0x1,
    CON_DEST_ERROR      = 0x2,
    CON_DEST_GAME_FIRST = 0x3,
    CON_DEST_GAME1      = 0x3,
    CON_DEST_GAME2      = 0x4,
    CON_DEST_GAME3      = 0x5,
    CON_DEST_GAME_LAST  = 0x5,
    CON_DEST_COUNT      = 0x6,
};
inline print_msg_dest_t &operator++(print_msg_dest_t &t)
{
    t = static_cast<print_msg_dest_t>((static_cast<int>(t) + 1));
    return t;
}
inline print_msg_dest_t operator++(print_msg_dest_t &t, int)
{
    print_msg_dest_t old = t;
    t = static_cast<print_msg_dest_t>((static_cast<int>(t) + 1));
    return old;
}

