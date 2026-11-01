"""Generate an actual-body fixture for the bounded stock GSC lifetime repair."""
from pathlib import Path
import hashlib
import json

root = Path(__file__).resolve().parents[3]
source = root / 'src/clientscript/cscr_parser.cpp'
text = source.read_text(encoding='utf-8')
body = text[text.index('static bool Scr_ModIsSpace('):text.index('// A level-owned theft voice worker')]
evidence = root / 'build-android-audit/headless-map-audit-20261018/retail-scripts'
samples = []
hashes = {}
for name in ['common_zombie-maps__zombiemode.gsc', 'common_zombie_patch-maps__zombiemode.gsc']:
    data = (evidence / name).read_bytes()
    hashes[name] = hashlib.sha256(data).hexdigest()
    script = data.decode('utf-8').rstrip('\0').replace('\r\n', '\n')
    start = script.index('round_think()\n{')
    end = script.index('maxreward =', start)
    samples.append(script[start:end] + 'maxreward = 50 * level.round_number;\n}\n}\n')

fixture = r'''
#include <clientscript/cscr_main.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
static bool sp = true, zombie = true;
static int logs;
bool Com_IsSPLevel() { return sp; }
bool G_SP_IsZombieMode() { return zombie; }
int I_stricmp(const char *a, const char *b) {
    for (; *a && *b; ++a, ++b) {
        int d = std::tolower((unsigned char)*a) - std::tolower((unsigned char)*b);
        if (d) return d;
    }
    return (unsigned char)*a - (unsigned char)*b;
}
void Com_Printf(int, const char*, ...) { ++logs; }
ACTUAL_BODY
static void check(const std::string &input, scriptInstance_t inst, const char *file, bool expected) {
    std::string data = input;
    logs = 0;
    Scr_ZombieRoundLifetimeSource(inst, file, data.data(), (int)data.size());
    bool patched = data.find("level endon(\"end_game\");") != std::string::npos;
    const bool already = input.find("level endon(\"end_game\");") != std::string::npos;
    if ((data != input) != expected || patched != (expected || already) || data.size() != input.size()
        || std::count(data.begin(), data.end(), '\n') != std::count(input.begin(), input.end(), '\n')) {
        std::fprintf(stderr, "FAIL input=%zu expected=%d changed=%d patched=%d\n", input.size(), expected, data != input, patched);
        std::abort();
    }
    if (expected) {
        auto a = input.find("round_think()");
        auto b = input.find("maxreward =");
        if (data.substr(0, a) != input.substr(0, a) || data.substr(b) != input.substr(b) || logs != 1) std::abort();
    }
}
int main() {
    const std::string samples[] = { SAMPLE_INPUTS };
    unsigned cases = 0;
    for (const auto &sample : samples) {
        std::string input = "prefix // unchanged\n" + sample + "suffix // unchanged\n";
        sp = zombie = true;
        check(input, SCRIPTINSTANCE_SERVER, "maps/_zombiemode.gsc", true); ++cases;
        auto crlf = input;
        for (size_t p = 0; (p = crlf.find('\n',p)) != std::string::npos; p += 2) crlf.insert(p,"\r");
        check(crlf, SCRIPTINSTANCE_SERVER, "maps/_zombiemode.gsc", true); ++cases;
        auto applied = input;
        Scr_ZombieRoundLifetimeSource(SCRIPTINSTANCE_SERVER, "maps/_zombiemode.gsc", applied.data(), (int)applied.size());
        check(applied, SCRIPTINSTANCE_SERVER, "maps/_zombiemode.gsc", false); ++cases;
        check(input, SCRIPTINSTANCE_CLIENT, "maps/_zombiemode.gsc", false); ++cases;
        check(input, SCRIPTINSTANCE_SERVER, "maps/custom.gsc", false); ++cases;
        check(input, SCRIPTINSTANCE_SERVER, "maps/_zombiemode.gsc.custom", false); ++cases;
        sp = false; check(input, SCRIPTINSTANCE_SERVER, "maps/_zombiemode.gsc", false); ++cases;
        sp = true; zombie = false; check(input, SCRIPTINSTANCE_SERVER, "maps/_zombiemode.gsc", false); ++cases;
        zombie = true;
        check(input + input, SCRIPTINSTANCE_SERVER, "maps/_zombiemode.gsc", false); ++cases;
        auto altered = input; altered.replace(altered.find("DT#36173"), 8, "CUSTOM!!");
        check(altered, SCRIPTINSTANCE_SERVER, "maps/_zombiemode.gsc", false); ++cases;
        auto identifier = input; identifier.insert(identifier.find("round_think()"), "custom_");
        check(identifier, SCRIPTINSTANCE_SERVER, "maps/_zombiemode.gsc", false); ++cases;
        for (size_t n = 0; n < input.find("DT#36173") + 8; ++n) {
            check(input.substr(0,n), SCRIPTINSTANCE_SERVER, "maps/_zombiemode.gsc", false); ++cases;
        }
    }
    std::printf("PASS actual round-lifetime source repair: %u cases\n", cases);
}
'''
inputs = ',\n'.join('R"BO1GSC(' + sample + ')BO1GSC"' for sample in samples)
fixture = fixture.replace('ACTUAL_BODY', body).replace('SAMPLE_INPUTS', inputs)
out = root / 'build-android-audit/zombie-round-lifetime-regression'
out.mkdir(exist_ok=True)
(out / 'fixture.cpp').write_text(fixture, encoding='utf-8')
(out / 'generation.json').write_text(json.dumps({
    'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
    'actual_body_sha256': hashlib.sha256(body.encode()).hexdigest(),
    'retail_source_sha256': hashes,
    'scope': 'Actual patch code, two stock headers, mode/file/uniqueness/custom guards and every truncated prefix; unchanged size/lines/suffix'
}, indent=2), encoding='utf-8')
print(out)
