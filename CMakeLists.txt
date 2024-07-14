cmake_minimum_required(VERSION 3.13)

set(SRCS src/ansi.c
         src/ansi_port.c
         src/main.cpp
         src/nr_micro_shell.c
)

add_executable(chino.shell.sh ${SRCS})
target_link_libraries(chino.shell.sh PRIVATE c)
target_include_directories(chino.shell.sh PRIVATE inc)
target_include_directories(chino.shell.sh PRIVATE ${WIN_SDK_INCLUDE_DIR})

install(TARGETS chino.shell.sh
        COMPONENT chino.shell
        RUNTIME DESTINATION chino/system
        LIBRARY DESTINATION chino/system
        )
        