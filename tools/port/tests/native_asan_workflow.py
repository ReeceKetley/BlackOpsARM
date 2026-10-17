"""Configure-only test of the production ASan target policy; no engine build."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[3]
source = (root / 'android/cmake/native-engine-entry/CMakeLists.txt').read_text()
block = source[source.index('# Diagnostic-only, deliberately AFTER'):]
cmake = 'D:/Android/Sdk/cmake/3.22.1/bin/cmake.exe'
with tempfile.TemporaryDirectory(prefix='asan-workflow-', dir=root/'build-android-audit') as temp:
    directory = Path(temp)
    (directory/'child').mkdir()
    (directory/'dummy.c').write_text('int diagnostic_fixture;')
    (directory/'child/CMakeLists.txt').write_text('add_library(child STATIC ../dummy.c)\n')
    (directory/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.22.1)
project(AsanWorkflow NONE)
set(ANDROID TRUE)
set(BO1_NATIVE_SDL_APP ON)
set(BO1_NATIVE_FULL_DATABASE ON)
add_library(engine SHARED dummy.c)
set_target_properties(engine PROPERTIES LINKER_LANGUAGE C)
target_compile_options(engine PRIVATE -fno-sanitize=all)
add_subdirectory(child)
set_target_properties(child PROPERTIES LINKER_LANGUAGE C)
add_library(external SHARED IMPORTED)
''' + block + '''
get_target_property(engine_flags engine COMPILE_OPTIONS)
get_target_property(child_flags child COMPILE_OPTIONS)
get_target_property(link_flags engine LINK_OPTIONS)
get_target_property(external_flags external COMPILE_OPTIONS)
file(WRITE "${CMAKE_BINARY_DIR}/checked.txt" "${engine_flags}\\n${child_flags}\\n${link_flags}\\n${external_flags}\\n")
''')
    def configure(name, *args):
        return subprocess.run([cmake, '-S', str(directory), '-B', str(directory/name), '-G', 'Ninja', '-DCMAKE_MAKE_PROGRAM=D:/Android/Sdk/cmake/3.22.1/bin/ninja.exe', *args], capture_output=True, text=True)
    bad = configure('bad', '-DBO1_NATIVE_ASAN=ON', '-DANDROID_STL=c++_static')
    assert bad.returncode and 'c++_shared' in bad.stderr
    # Language NONE prevents compilation; flags are emitted before generation.
    configure('on', '-DBO1_NATIVE_ASAN=ON', '-DANDROID_STL=c++_shared')
    flags = (directory/'on/checked.txt').read_text().splitlines()
    assert flags[0].index('-fno-sanitize=all') < flags[0].index('-fsanitize=address')
    assert '-fsanitize=address' in flags[1] and '-fsanitize=address' in flags[2]
    assert flags[3] == 'external_flags-NOTFOUND'
    configure('off')
    flags = (directory/'off/checked.txt').read_text().splitlines()
    assert flags[0] == '-fno-sanitize=all' and 'address' not in ''.join(flags)
print('native_asan_workflow=PASS off_unchanged=1 after_perf_reset=1 recursive_archive=1 shared_link=1 imported_untouched=1 static_stl_rejected=1 no_engine_build=1')
