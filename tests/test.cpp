// test/test_basic.cpp
#include <gtest/gtest.h>

#include "libmcstatus/libmcstatus.hpp"

// Basic test to verify GoogleTest is working
TEST(BasicTest, SampleTest) {
	EXPECT_EQ(1 + 1, 2);
	EXPECT_TRUE(true);
}

TEST(DummyTest, IP) {
	auto address = boost::asio::ip::make_address( "127.0.0.1");
	
	EXPECT_EQ(address.to_string(), "127.0.0.1");
}

// You can add tests for your library functions here
// Example:
// #include "mcstatus/your_header.h"
//
// TEST(McStatusTest, YourFunctionTest) {
//     // Test your library functions
// }
