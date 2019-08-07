

all: crc.cpp main.cpp open62541.c poll_optimize.cpp rs485.cpp serialize.cpp serialize.cpp tcp.cpp utils.cpp
	g++ -std=c++11 -w -Wl,--allow-multiple-definition -fpermissive -lpthread -o opc_server crc.cpp main.cpp open62541.c poll_optimize.cpp rs485.cpp serialize.cpp serialize.cpp tcp.cpp utils.cpp 

