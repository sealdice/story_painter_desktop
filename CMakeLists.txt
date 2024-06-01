# CMakeList.txt: cpp_story_painter 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required(VERSION 3.8)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif ()

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

project("cpp_story_painter" VERSION 0.0.0 LANGUAGES C CXX)


# minidocx 库
set(MINIDOCX_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/minidocx")
set(MINIDOCX_THIRD_PARTY_DIR "${MINIDOCX_DIR}/3rdparty" CACHE PATH "3rdparty")
set(ZIP_DIR "${MINIDOCX_THIRD_PARTY_DIR}/zip-0.2.1" CACHE PATH "zip")
set(PUGIXML_DIR "${MINIDOCX_THIRD_PARTY_DIR}/pugixml-1.13" CACHE PATH "pugixml")

add_library(zip INTERFACE)
set_target_properties(zip PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${ZIP_DIR}"
        INTERFACE_SOURCES "${ZIP_DIR}/zip.c"
)

add_library(pugixml INTERFACE)
set_target_properties(pugixml PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${PUGIXML_DIR}"
        INTERFACE_SOURCES "${PUGIXML_DIR}/pugixml.cpp"
)

add_library(minidocx STATIC)
list(APPEND sources "${MINIDOCX_DIR}/src/minidocx.cpp")
list(APPEND headers "${MINIDOCX_DIR}/src/minidocx.hpp")
source_group("Header Files" FILES ${headers})
target_sources(minidocx PRIVATE ${sources} ${headers})
target_include_directories(minidocx PUBLIC "${MINIDOCX_DIR}/src")
target_link_libraries(minidocx PRIVATE zip pugixml)
if (WITH_STATIC_CRT)
    set_target_properties(minidocx PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()


add_subdirectory(lib/fltk)

set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(lib/nlohmann_json)

add_library(random_color STATIC)
target_sources(random_color PRIVATE "lib/random_color/randomcolor.cpp" "lib/random_color/randomcolor.h")
target_include_directories(random_color PUBLIC "lib/random_color")

# 将源代码添加到此项目的可执行文件。
if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    add_executable(${PROJECT_NAME} "main.cpp" "main.h")
else ()
    add_executable(${PROJECT_NAME} WIN32 "main.cpp" "main.h")
endif ()
set_property(TARGET cpp_story_painter PROPERTY CXX_STANDARD 20)


target_link_libraries(${PROJECT_NAME} PRIVATE minidocx)
target_link_libraries(${PROJECT_NAME} PRIVATE fltk::fltk)
target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(${PROJECT_NAME} PRIVATE random_color)




# TODO: 如有需要，请添加测试并安装目标。
