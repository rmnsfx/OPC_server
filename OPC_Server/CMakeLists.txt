cmake_minimum_required(VERSION 3.13.2)
project (opc_server)

#find_package( Threads REQUIRED )
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_USE_PTHREADS_INIT)
set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS} -Wl,--allow-multiple-definition -fpermissive -lpthread")

add_executable(crc.cpp main.cpp open62541.c poll_optimize.cpp rs485.cpp serialize.cpp serialize.cpp tcp.cpp utils.cpp)

#target_link_libraries("-Wl,--allow-multiple-definition -fpermissive -lpthread")