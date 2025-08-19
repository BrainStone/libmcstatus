// test/test_basic.cpp
#include <gtest/gtest.h>

#include "libmcstatus/JavaServer.hpp"

// Basic test to verify GoogleTest is working
TEST(BasicTest, SampleTest) {
	EXPECT_EQ(1 + 1, 2);
	EXPECT_TRUE(true);
}

TEST(DummyTest, IP) {
	auto address = boost::asio::ip::make_address( "127.0.0.1");
	
	EXPECT_EQ(address.to_string(), "127.0.0.1");
}

TEST(DummyTest, XXX) {
	using namespace std::chrono_literals;
	
	auto server = libmcstatus::JavaServer( boost::asio::ip::make_address( "127.0.0.1"), 25565);
	
	EXPECT_LT(server.ping(), 1ms);
}

TEST(DummyTest, lookup) {
	auto server1 = libmcstatus::JavaServer::lookup("127.0.0.1");
	auto server2= libmcstatus::JavaServer::lookup("127.0.0.1:12345");
	auto server3 = libmcstatus::JavaServer::lookup("mc02.root.project-creative.net");
//	auto server4 = libmcstatus::JavaServer::lookup("foo.bar");
}

// You can add tests for your library functions here
// Example:
// #include "mcstatus/your_header.h"
//
// TEST(McStatusTest, YourFunctionTest) {
//     // Test your library functions
// }
