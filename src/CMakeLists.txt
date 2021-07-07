#    Copyright (c) 2021 on-keyday
#    Released under the MIT license
#    https://opensource.org/licenses/mit-license.php
if(WIN32)
include_directories("D:\\OpenSSL\\build-x64-debug\\include")
link_directories("D:\\OpenSSL\\build-x64-debug\\lib")
add_executable(httpc "httpc.cpp"  )
file(GLOB HTTPC "httpc/**")
add_library(netclient SHARED ${HTTPC}  "httpc/refactering/httpc.cpp"  "httpc2.cpp")
target_link_libraries(httpc netclient)
else()
find_package(OpenSSL REQUIRED)
endif()

include_directories("commonlib")

file(GLOB EASY_H "easyscript/*.h")
file(GLOB EASY_CPP "easyscript/*.cpp")
file(GLOB MODTEST "module_test/**")
#file(GLOB HARD_H "hardscript/*.h")
#file(GLOB HARD_SC "hardscript/*" "hardscript/*/*")


add_library(easyscript SHARED ${EASY_CPP} ${EASY_H})
add_executable(program "main.cpp"  ${MODTEST})
target_link_libraries(program easyscript)
if(WIN32)
target_link_libraries(netclient ws2_32)
target_link_libraries(netclient libcrypto libssl)
endif()
