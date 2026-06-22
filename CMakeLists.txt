cmake_minimum_required(VERSION 3.20)
project(CASA LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── 의존성 ────────────────────────────────────────────────────
find_package(yaml-cpp REQUIRED)
find_package(nlohmann_json REQUIRED)

find_package(GTest REQUIRED)
include(GoogleTest)

# ── 인클루드 경로 ─────────────────────────────────────────────
set(CASA_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/include
)

# ── 라이브러리 소스 (main.cpp 제외) ───────────────────────────
file(GLOB_RECURSE CASA_LIB_SOURCES
    src/ap/*.cpp
    src/memory/*.cpp
    src/cache/*.cpp
    src/analysis/*.cpp
    src/report/*.cpp
    src/pipeline/*.cpp
    src/cli/*.cpp
)

if(CASA_LIB_SOURCES)
    add_library(casa_lib STATIC ${CASA_LIB_SOURCES})
    target_include_directories(casa_lib PUBLIC ${CASA_INCLUDE_DIRS})
    target_link_libraries(casa_lib PUBLIC yaml-cpp nlohmann_json::nlohmann_json)
endif()

# ── 실행 파일 ────────────────────────────────────────────────
add_executable(casa src/main.cpp)
target_include_directories(casa PRIVATE ${CASA_INCLUDE_DIRS})
if(TARGET casa_lib)
    target_link_libraries(casa PRIVATE casa_lib)
endif()
target_link_libraries(casa PRIVATE yaml-cpp nlohmann_json::nlohmann_json)

# ── 테스트 ───────────────────────────────────────────────────
enable_testing()

function(casa_add_test target source)
    add_executable(${target} ${source})
    target_include_directories(${target} PRIVATE ${CASA_INCLUDE_DIRS})
    if(TARGET casa_lib)
        target_link_libraries(${target} PRIVATE casa_lib)
    endif()
    target_link_libraries(${target} PRIVATE yaml-cpp nlohmann_json::nlohmann_json GTest::GTest GTest::Main)
    gtest_discover_tests(${target})
endfunction()

# Phase 2 — AP Layer
file(GLOB AP_TESTS tests/ap/*.cpp)
foreach(src ${AP_TESTS})
    get_filename_component(name ${src} NAME_WE)
    casa_add_test(${name} ${src})
endforeach()

# Phase 3 — Memory Layer
file(GLOB MEM_TESTS tests/memory/*.cpp)
foreach(src ${MEM_TESTS})
    get_filename_component(name ${src} NAME_WE)
    casa_add_test(${name} ${src})
endforeach()

# Phase 4 — Cache Layer
file(GLOB CACHE_TESTS tests/cache/*.cpp)
foreach(src ${CACHE_TESTS})
    get_filename_component(name ${src} NAME_WE)
    casa_add_test(${name} ${src})
endforeach()

# Phase 5 — Analysis Layer
file(GLOB ANALYSIS_TESTS tests/analysis/*.cpp)
foreach(src ${ANALYSIS_TESTS})
    get_filename_component(name ${src} NAME_WE)
    casa_add_test(${name} ${src})
endforeach()

# Phase 6 — Report Layer
file(GLOB REPORT_TESTS tests/report/*.cpp)
foreach(src ${REPORT_TESTS})
    get_filename_component(name ${src} NAME_WE)
    casa_add_test(${name} ${src})
endforeach()

# Phase 7 — Integration
file(GLOB INTEGRATION_TESTS tests/integration/*.cpp)
foreach(src ${INTEGRATION_TESTS})
    get_filename_component(name ${src} NAME_WE)
    casa_add_test(${name} ${src})
endforeach()

# CLI
file(GLOB CLI_TESTS tests/cli/*.cpp)
foreach(src ${CLI_TESTS})
    get_filename_component(name ${src} NAME_WE)
    casa_add_test(${name} ${src})
endforeach()