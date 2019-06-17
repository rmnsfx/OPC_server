#include <cstdio>
#include <stdlib.h>
#include "gtest/gtest.h"
#include "crc.h"
#include "serialize.h"
#include "main.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/filereadstream.h"



class TestSerialization : public ::testing::Test {
public:
	TestSerialization() { /* init protected members here */ }
	~TestSerialization() { /* free protected members here */ }
	void SetUp() { /* called before every test */ }
	void TearDown() { /* called after every test */ }

protected:
	/* none yet */
};



TEST(Test_crc, calculate_crc)
{
	uint8_t arr[] = { 0x01, 0x03, 0x07, 0xC8, 0x00, 0x7D };

	uint16_t result = calculate_crc((uint8_t*)&arr, 6);

	EXPECT_EQ(result, 0x6105);
};

TEST(Test_serialize, serializeFromJSON)
{
	Controller asis;
	Controller tobe;

	//asis = serializeFromJSON("opc.json");
	
	//EXPECT_EQ(correct_controller, current_controller);
};

TEST(Test_main, workerOPC)
{

};

TEST(Test_main, pollingEngine)
{

};

TEST(Test_main, type_converter)
{

};

TEST(Test_main, interface_converter)
{

};

TEST(Test_poll_optimize, splitRegs)
{

};

TEST(Test_poll_optimize, checkFloatType)
{

};

TEST(Test_poll_optimize, reorganizeNodeIntoPolls)
{

};

TEST(Test_rs485, pathToPort)
{

};

TEST(Test_rs485, pollingDeviceRS485)
{

};

TEST(Test_tcp, connectDeviceTCP)
{

};

TEST(Test_tcp, pollingDeviceTCP)
{

};


int main(int argc, char** argv)
{
    //printf("hello from OPC_Server_Test!\n");
	int returnValue;
	::testing::InitGoogleTest(&argc, argv);

	returnValue = RUN_ALL_TESTS();


    return 0;
}