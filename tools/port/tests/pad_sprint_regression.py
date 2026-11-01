"""Compile the actual two sprint blocks to check their combined frame behavior.

Usage: python tools/port/tests/pad_sprint_regression.py
Writes a small C++ fixture under build-android-audit; compile and run it with
the host compiler or Android NDK. No replacement sprint implementation.
"""
from pathlib import Path

root = Path(__file__).resolve().parents[3]
text = (root / 'src/client_mp/cl_input_mp.cpp').read_text(encoding='utf-8')

def block(function):
    start = text.index('void __cdecl ' + function + '(')
    start = text.index('if ( !kb[3].active )', start)
    brace = text.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]

fixture = '''#include <cassert>
#include <cstdio>
#define __ANDROID__ 1
struct Button { bool active=false, wasPressed=false; };
struct Bits { unsigned bits=0; void setBit(int i){bits|=1u<<i;}
 void resetBit(int i){bits&=~(1u<<i);} bool testBit(int i){return bits&(1u<<i);} };
struct Command { Bits button_bits; };
void Keyboard(Button* kb, Command* cmd) { KEYBOARD }
void Pad(Button* kb, Command* cmd) { PAD }
int main() {
 Button kb[31]; Command cmd;
 // Down and up in one frame: active is false, but the press must survive.
 kb[30].wasPressed=true; Keyboard(kb,&cmd); assert(cmd.button_bits.testBit(1));
 assert(!kb[30].wasPressed); Pad(kb,&cmd); assert(cmd.button_bits.testBit(1));
 // Next frame must not latch sprint forever.
 cmd={}; Keyboard(kb,&cmd); Pad(kb,&cmd); assert(!cmd.button_bits.testBit(1));
 // Holding the click continues to request sprint.
 kb[30].active=true; cmd={}; Keyboard(kb,&cmd); Pad(kb,&cmd);
 assert(cmd.button_bits.testBit(1));
 // A backward key still suppresses sprint.
 kb[3].active=true; cmd={}; Keyboard(kb,&cmd); Pad(kb,&cmd);
 assert(!cmd.button_bits.testBit(1));
 puts("PASS actual CL_KeyMove + CL_GamepadMove sprint blocks: tap preserved, release, hold, backward suppression");
}
'''.replace('KEYBOARD', block('CL_KeyMove')).replace('PAD', block('CL_GamepadMove'))
output = root / 'build-android-audit/pad_sprint_regression.cpp'
output.write_text(fixture, encoding='utf-8')
print(output)
