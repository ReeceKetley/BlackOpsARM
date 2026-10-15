cmake_minimum_required(VERSION 3.22.1)
project(BlackOpsNativeVulkan LANGUAGES C CXX)
get_filename_component(BO1_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
# Stage-one harness may reuse the already built, packaged compiler. This
# option belongs to the harness, not the renderer's production integration.
set(BO1_SHADER_COMPILER_PREBUILT "" CACHE FILEPATH "Existing ARM64 shader compiler for focused harness")
if(NOT TARGET bo1_native_shader_compiler)
    if(BO1_SHADER_COMPILER_PREBUILT)
        add_library(bo1_native_shader_compiler SHARED IMPORTED)
        set_target_properties(bo1_native_shader_compiler PROPERTIES
            IMPORTED_LOCATION "${BO1_SHADER_COMPILER_PREBUILT}"
            INTERFACE_INCLUDE_DIRECTORIES "${BO1_ROOT}/third_party/native-shader-compiler/vkd3d-1.17/include")
    else()
        add_subdirectory("${BO1_ROOT}/android/cmake/native-shader-compiler" shader-compiler)
    endif()
endif()
add_library(bo1_vulkan_shader STATIC "${BO1_ROOT}/src/gfx_vulkan/shader_translate.cpp"
    "${BO1_ROOT}/src/gfx_vulkan/graphics_program.cpp"
    "${BO1_ROOT}/src/gfx_vulkan/depth_bias.cpp"
    "${BO1_ROOT}/src/gfx_vulkan/pipeline_key.cpp"
    "${BO1_ROOT}/src/gfx_vulkan/resources.cpp"
    "${BO1_ROOT}/src/gfx_vulkan/draw_storage.cpp"
    "${BO1_ROOT}/src/gfx_vulkan/image_usage.cpp"
    "${BO1_ROOT}/src/gfx_vulkan/attachment_pass.cpp"
    "${BO1_ROOT}/src/gfx_vulkan/sampler.cpp")
target_compile_features(bo1_vulkan_shader PUBLIC cxx_std_17)
target_include_directories(bo1_vulkan_shader PUBLIC "${BO1_ROOT}/src/gfx_vulkan")
target_link_libraries(bo1_vulkan_shader PUBLIC bo1_native_shader_compiler vulkan)
if(ANDROID)
    target_compile_options(bo1_vulkan_shader PRIVATE -fsanitize=undefined -fno-sanitize-recover=undefined)
endif()
option(BO1_VULKAN_SHADER_SMOKE "Build focused actual-SM3 translator/device harness" ON)
if(BO1_VULKAN_SHADER_SMOKE)
    add_executable(bo1_vulkan_shader_smoke "${BO1_ROOT}/tools/port/tests/native_vulkan_shader_smoke.cpp"
        "${BO1_ROOT}/tools/port/tests/native_vulkan_lrp_draw.cpp"
        "${BO1_ROOT}/tools/port/tests/native_vulkan_depth_compare.cpp"
        "${BO1_ROOT}/tools/port/tests/native_vulkan_ubyte_input.cpp"
        "${BO1_ROOT}/tools/port/tests/native_vulkan_image_copy.cpp"
        "${BO1_ROOT}/tools/port/tests/native_vulkan_depth_copy.cpp"
        "${BO1_ROOT}/tools/port/tests/native_vulkan_upload_flights.cpp"
        "${BO1_ROOT}/tools/port/tests/native_vulkan_draw_storage.cpp")
    target_include_directories(bo1_vulkan_shader_smoke PRIVATE "${BO1_ROOT}/src")
    target_link_libraries(bo1_vulkan_shader_smoke PRIVATE bo1_vulkan_shader vulkan)
    if(ANDROID)
        target_compile_options(bo1_vulkan_shader_smoke PRIVATE -fsanitize=undefined -fno-sanitize-recover=undefined)
        target_link_options(bo1_vulkan_shader_smoke PRIVATE -fsanitize=undefined -fno-sanitize-recover=undefined -static-libsan)
    endif()
endif()
