#include <cstdio>
#include <stdlib.h>
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "crc.h"
#include "serialize.h"
#include "main.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/filereadstream.h"
#include "poll_optimize.h"
#include "rs485.h"
#include "tcp.h"
#include <vector>
#include <unistd.h>


//https://github.com/google/googletest/blob/master/googletest/samples/sample1_unittest.cc
//https://eax.me/cpp-gtest/

class TestSerialization : public testing::Test {
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
	const std::string tcp = "TCP";
	const std::string rs485 = "RS-485";
		
	Interface_type tobe_tcp = Interface_type::tcp;
	Interface_type tobe_485 = Interface_type::rs485;

	Interface_type asis_tcp = interface_converter(tcp);
	Interface_type asis_485 = interface_converter(rs485);

	EXPECT_EQ(tobe_tcp, asis_tcp);
	EXPECT_EQ(tobe_485, asis_485);
};

TEST(Test_main, interface_converter)
{
	EXPECT_EQ(type_converter("int16"), Data_type::int16);
	EXPECT_EQ(type_converter("uint16"), Data_type::uint16);
	EXPECT_EQ(type_converter("int32"), Data_type::int32);
	EXPECT_EQ(type_converter("uint32"), Data_type::uint32);
	EXPECT_EQ(type_converter("float_BE"), Data_type::float_BE);
	EXPECT_EQ(type_converter("float_BE_swap"), Data_type::float_BE_swap);
	EXPECT_EQ(type_converter("float_LE"), Data_type::float_LE);
	EXPECT_EQ(type_converter("float_LE_swap"), Data_type::float_LE_swap);
};

TEST(Test_poll_optimize, splitRegs)
{
	std::vector<int> regs1 = { 1 };
	std::vector<std::vector<int>> result1; 
	result1 = splitRegs(regs1);
	
	EXPECT_EQ(result1[0][0], 0);

	std::vector<int> regs2 = { 5, 3, 8 };
	std::vector<std::vector<int>> result2;
	result2 = splitRegs(regs2);

	EXPECT_EQ(result2[0][0], 2);
	EXPECT_EQ(result2[0][1], 7);

	std::vector<int> regs3 = { 5, 300, 8 };
	std::vector<std::vector<int>> result3;
	result3 = splitRegs(regs3);

	EXPECT_EQ(result3[0][0], 4);
	EXPECT_EQ(result3[0][1], 7);
	EXPECT_EQ(result3[1][0], 299);
};

TEST(Test_poll_optimize, checkFloatType)
{
	Tag tag;
	std::vector<Tag> vector_tag;
	
	tag.reg_address = 10;
	tag.enum_data_type = Data_type::float_BE;
	
	vector_tag.push_back(tag);

	EXPECT_EQ(checkFloatType(vector_tag, 9), true);
	
	vector_tag[0].enum_data_type = Data_type::float_BE_swap;
	EXPECT_EQ(checkFloatType(vector_tag, 9), true);

	vector_tag[0].enum_data_type = Data_type::float_LE;
	EXPECT_EQ(checkFloatType(vector_tag, 9), true);

	vector_tag[0].enum_data_type = Data_type::float_LE_swap;
	EXPECT_EQ(checkFloatType(vector_tag, 9), true);

	vector_tag[0].enum_data_type = Data_type::int16;
	EXPECT_EQ(checkFloatType(vector_tag, 9), false);

};

TEST(Test_poll_optimize, reorganizeNodeIntoPolls_TCP_INT16)
{
	std::vector<int> vectorReference_tcp_holding = {0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x03, 0x00, 0x02, 0x00, 0x06};
	std::vector<int> vectorReference_tcp_input = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x04, 0x00, 0x02, 0x00, 0x06 };
	
	Node node;
	Device device;
	Tag tag;

	node.on = 1;
	node.enum_interface_type = Interface_type::tcp; 
	device.on = 1;
	
	tag.function = 3;
	tag.enum_data_type = Data_type::int16;
	tag.on = 1;
	
	tag.reg_address = 3;		
	tag.reg_position = 0;
	device.vectorTag.push_back(tag);

	tag.reg_address = 5;
	tag.reg_position = 1;
	device.vectorTag.push_back(tag);

	tag.reg_address = 8;
	tag.reg_position = 2;
	device.vectorTag.push_back(tag);

	node.vectorDevice.push_back(device);

	//Тестируем holding 
	std::vector<Optimize> vector_optimize_holding = reorganizeNodeIntoPolls(&node);
		
	for (int i = 0; i < vectorReference_tcp_input.size(); i++)
	{
		SCOPED_TRACE(i); //write to the console in which iteration the error occurred
		EXPECT_EQ(vector_optimize_holding[0].request[0][i], vectorReference_tcp_holding[i]);		
	}


	//Тестируем input 
	node.vectorDevice[0].vectorTag[0].function = 4;
	node.vectorDevice[0].vectorTag[1].function = 4;
	node.vectorDevice[0].vectorTag[2].function = 4;

	std::vector<Optimize> vector_optimize_input = reorganizeNodeIntoPolls(&node);

	for (int i = 0; i < vectorReference_tcp_holding.size(); i++)
	{
		SCOPED_TRACE(i); 
		EXPECT_EQ(vector_optimize_input[0].request[0][i], vectorReference_tcp_input[i]);
	}
};

TEST(Test_poll_optimize, reorganizeNodeIntoPolls_TCP_FLOAT_BE)
{
	std::vector<int> vectorReference_tcp_holding = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x03, 0x00, 0x02, 0x00, 0x07 };
	std::vector<int> vectorReference_tcp_input = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x04, 0x00, 0x02, 0x00, 0x07 };

	Node node;
	Device device;
	Tag tag;

	node.on = 1;
	node.enum_interface_type = Interface_type::tcp; 
	device.on = 1;

	tag.function = 3;
	tag.enum_data_type = Data_type::float_BE;
	tag.on = 1;

	tag.reg_address = 3;
	tag.reg_position = 0;
	device.vectorTag.push_back(tag);

	tag.reg_address = 5;
	tag.reg_position = 1;
	device.vectorTag.push_back(tag);

	tag.reg_address = 8;
	tag.reg_position = 2;
	device.vectorTag.push_back(tag);

	node.vectorDevice.push_back(device);

	//Тестируем holding 
	std::vector<Optimize> vector_optimize_holding = reorganizeNodeIntoPolls(&node);

	for (int i = 0; i < vectorReference_tcp_holding.size(); i++)
	{
		SCOPED_TRACE(i); //write to the console in which iteration the error occurred
		EXPECT_EQ(vector_optimize_holding[0].request[0][i], vectorReference_tcp_holding[i]);
	}


	//Тестируем input 
	node.vectorDevice[0].vectorTag[0].function = 4;
	node.vectorDevice[0].vectorTag[1].function = 4;
	node.vectorDevice[0].vectorTag[2].function = 4;

	std::vector<Optimize> vector_optimize_input = reorganizeNodeIntoPolls(&node);

	for (int i = 0; i < vectorReference_tcp_input.size(); i++)
	{
		SCOPED_TRACE(i); //write to the console in which iteration the error occurred
		EXPECT_EQ(vector_optimize_input[0].request[0][i], vectorReference_tcp_input[i]);
	}
};

TEST(Test_poll_optimize, reorganizeNodeIntoPolls_RS485_INT16)
{	
	std::vector<int> vectorReference_rs485_holding = { 0x00, 0x03, 0x00, 0x02, 0x00, 0x06, 0x65, 0xD9 };
	std::vector<int> vectorReference_rs485_input = { 0x00, 0x04, 0x00, 0x02, 0x00, 0x06, 0xD0, 0x19 };

	Node node;
	Device device;
	Tag tag;

	node.on = 1;
	node.enum_interface_type = Interface_type::rs485; //Interface_type::rs485;
	device.on = 1;

	tag.function = 3;
	tag.enum_data_type = Data_type::int16;
	tag.on = 1;

	tag.reg_address = 3;
	tag.reg_position = 0;
	device.vectorTag.push_back(tag);

	tag.reg_address = 5;
	tag.reg_position = 1;
	device.vectorTag.push_back(tag);

	tag.reg_address = 8;
	tag.reg_position = 2;
	device.vectorTag.push_back(tag);

	node.vectorDevice.push_back(device);

	//Тестируем holding 
	std::vector<Optimize> vector_optimize_holding = reorganizeNodeIntoPolls(&node);

	for (int i = 0; i < vectorReference_rs485_holding.size(); i++)
	{
		SCOPED_TRACE(i); 
		EXPECT_EQ(vector_optimize_holding[0].request[0][i], vectorReference_rs485_holding[i]);
	}


	//Тестируем input 
	node.vectorDevice[0].vectorTag[0].function = 4;
	node.vectorDevice[0].vectorTag[1].function = 4;
	node.vectorDevice[0].vectorTag[2].function = 4;

	std::vector<Optimize> vector_optimize_input = reorganizeNodeIntoPolls(&node);

	for (int i = 0; i < vectorReference_rs485_input.size(); i++)
	{
		SCOPED_TRACE(i); 
		EXPECT_EQ(vector_optimize_input[0].request[0][i], vectorReference_rs485_input[i]);
	}
};


TEST(Test_poll_optimize, reorganizeNodeIntoPolls_RS485_FLOAT_BE)
{
	std::vector<int> vectorReference_rs485_holding = { 0x00, 0x03, 0x00, 0x02, 0x00, 0x07, 0xA4, 0x19 };
	std::vector<int> vectorReference_rs485_input = { 0x00, 0x04, 0x00, 0x02, 0x00, 0x07, 0x11, 0xD9 };

	Node node;
	Device device;
	Tag tag;

	node.on = 1;
	node.enum_interface_type = Interface_type::rs485; 
	device.on = 1;

	tag.function = 3;
	tag.enum_data_type = Data_type::float_BE;
	tag.on = 1;

	tag.reg_address = 3;
	tag.reg_position = 0;
	device.vectorTag.push_back(tag);

	tag.reg_address = 5;
	tag.reg_position = 1;
	device.vectorTag.push_back(tag);

	tag.reg_address = 8;
	tag.reg_position = 2;
	device.vectorTag.push_back(tag);

	node.vectorDevice.push_back(device);

	//Тестируем holding 
	std::vector<Optimize> vector_optimize_holding = reorganizeNodeIntoPolls(&node);

	for (int i = 0; i < vectorReference_rs485_holding.size(); i++)
	{
		SCOPED_TRACE(i); //write to the console in which iteration the error occurred
		EXPECT_EQ(vector_optimize_holding[0].request[0][i], vectorReference_rs485_holding[i]);
	}


	//Тестируем input 
	node.vectorDevice[0].vectorTag[0].function = 4;
	node.vectorDevice[0].vectorTag[1].function = 4;
	node.vectorDevice[0].vectorTag[2].function = 4;

	std::vector<Optimize> vector_optimize_input = reorganizeNodeIntoPolls(&node);

	for (int i = 0; i < vectorReference_rs485_input.size(); i++)
	{
		SCOPED_TRACE(i); 
		EXPECT_EQ(vector_optimize_input[0].request[0][i], vectorReference_rs485_input[i]);
	}
};

TEST(Test_rs485, pathToPort)
{	
	ASSERT_STREQ(pathToPort(0), "/dev/rs485_0");
	ASSERT_STREQ(pathToPort(1), "/dev/rs485_1");
	ASSERT_STREQ(pathToPort(2), "/dev/rs485_2");
	ASSERT_STREQ(pathToPort(3), "/dev/rs485_3");
	ASSERT_STREQ(pathToPort(4), "/dev/rs485_4");
	ASSERT_STREQ(pathToPort(5), "/dev/rs485_5");
	ASSERT_STREQ(pathToPort(6), "/dev/rs485_6");
	ASSERT_STREQ(pathToPort(7), "/dev/rs485_7");
};

using ::testing::ElementsAre;

TEST(Test_rs485, pollingDeviceRS485)
{
	std::vector<int> v = {5, 10, 15};
	ASSERT_THAT(v, ElementsAre(5, 110, 15));
};

TEST(Test_tcp, connectDeviceTCP)
{

};

TEST(Test_tcp, pollingDeviceTCP)
{

};


int main(int argc, char** argv)
{    	
	testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();    
}
