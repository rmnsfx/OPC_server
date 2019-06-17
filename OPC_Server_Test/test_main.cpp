#include <cstdio>
#include <stdlib.h>
#include "gtest/gtest.h"
#include "crc.h"

TEST(TestCRC, calculate_crc)
{
	uint8_t arr[] = { 0x01, 0x03, 0x07, 0xC8, 0x00, 0x7D };

	uint16_t result = calculate_crc((uint8_t*)&arr, 6);

	EXPECT_EQ(result, 0x6105);
}

int main(int argc, char** argv)
{
    //printf("hello from OPC_Server_Test!\n");
	int returnValue;
	::testing::InitGoogleTest(&argc, argv);

	returnValue = RUN_ALL_TESTS();


    return 0;
}