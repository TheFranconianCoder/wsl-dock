cmake_minimum_required(VERSION 3.20)
project(WslDock)

add_definitions(
        -DUNICODE
        -D_UNICODE
)

include_directories(src)

add_executable(WslDock_ANALYSIS
        src/main.c
        src/config.c
        src/tray.c
        src/wsl.c
)

target_compile_options(WslDock_ANALYSIS PRIVATE /std:clatest)

target_link_libraries(WslDock_ANALYSIS
        user32
        shell32
        kernel32
        gdi32
        ole32
        advapi32
)
