#    Copyright (c) 2021 on-keyday
#    Released under the MIT license
#    https://opensource.org/licenses/mit-license.php
if(WIN32)
include_directories("D:\\OpenSSL\\build-x64-debug\\include")
link_directories("D:\\OpenSSL\\build-x64-debug\\lib")
endif()
add_executable(program "main.cpp" "reader.h" "reader_helper.h" "socket.h" "socket.cpp" "sockinterpreter.cpp")
file(GLOB EASY_H "easyscript/*.h")
file(GLOB EASY_CPP "easyscript/*.cpp")
file(GLOB HARD_H "hardscript/*.h")
file(GLOB HARD_CPP "hardscript/*.cpp")
add_library(easyscript SHARED ${EASY_CPP} ${EASY_H} ${HARD_CPP} ${HARD_H})
target_link_libraries(program easyscript)
if(WIN32)
target_link_libraries(program ws2_32 libcrypto libssl)
endif()