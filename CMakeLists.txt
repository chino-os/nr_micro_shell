cmake_minimum_required(VERSION 3.13)

set(SRCS src/ansi.c
         src/ansi_port.c
         src/main.cpp
         src/nr_micro_shell.c
         src/commands/cat.cpp
         src/commands/echo.cpp
         src/commands/ping.cpp
         src/commands/serial.cpp
         src/commands/uname.cpp
)

add_executable(chino.shell.sh ${SRCS})
target_link_libraries(chino.shell.sh PRIVATE c c.host cxx_static)
target_include_directories(chino.shell.sh PRIVATE inc)
target_include_directories(chino.shell.sh PRIVATE ${WIN_SDK_INCLUDE_DIR})
set_target_properties(chino.shell.sh PROPERTIES OUTPUT_NAME sh)

install(TARGETS chino.shell.sh
        COMPONENT chino.shell
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        )
        