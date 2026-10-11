"""Generate a standalone test using the current production tables and functions.

Compile the generated source with the same MSVC headers/defines as the engine,
once as Win32 and once as BO1_PORT_X64. The retained retail offsets validate all
867 mappings at compile time on Win32; the executable checks native writes.
"""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]


def function(text, signature):
    start = text.index(signature)
    opening = text.index("{", start)
    # These selected functions have no braces inside string/comment literals.
    depth = 1
    end = opening + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[start:end]


weapon = (ROOT / "src/bgame/bg_weapons_load_obj.cpp").read_bytes().decode("latin1")
flame = (ROOT / "src/flame/flame_system.cpp").read_bytes().decode("latin1")
shared = (ROOT / "src/universal/q_shared.cpp").read_bytes().decode("latin1")
parts = [(ROOT / "tools/port/test_weapon_fields.cpp").read_text()]
for text, table, count in [(weapon, "weaponDefFields", 748), (flame, "flameTableFields", 119)]:
    match = re.search(r"cspField_t " + table + r"\[" + str(count) + r"\] =\s*\{.*?\n\};", text, re.S)
    assert match, table
    assert match.group().count("CSP_FIELD_OFFSET(") == count, table
    parts.append(match.group())
for signature in ["void __cdecl InitWeaponDef(", "void __cdecl InitFlameTable(",
                  "static void BG_CopyWeaponFullDefForUpdate(",
                  "static void BG_CommitWeaponFullDefUpdate("]:
    parts.append(function(weapon, signature))
parts.append(function(shared, "bool __cdecl KeyValueToField("))
output = ROOT / "build/run/test_weapon_fields.cpp"
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text("\n\n".join(parts).replace("\r\n", "\n"), encoding="utf8")
print(output)
