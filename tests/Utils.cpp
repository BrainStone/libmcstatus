#include "libmcstatus/impl/Utils.hpp"

#include <gtest/gtest.h>

#include <string>

using namespace libmcstatus::_impl;

// Test valid port numbers
TEST(ParsePortTest, ValidPortNumbers) {
	// Test common valid ports
	EXPECT_EQ(parse_port("80"), 80);
	EXPECT_EQ(parse_port("443"), 443);
	EXPECT_EQ(parse_port("25565"), 25565);  // Minecraft default
	EXPECT_EQ(parse_port("8080"), 8080);

	// Test edge cases for valid ports
	EXPECT_EQ(parse_port("1"), 1);          // Minimum valid port
	EXPECT_EQ(parse_port("65535"), 65535);  // Maximum valid port
}

// Test invalid port numbers - empty and non-numeric
TEST(ParsePortTest, InvalidInputs) {
	// Test empty string
	EXPECT_EQ(parse_port(""), 0);

	// Test non-numeric strings
	EXPECT_EQ(parse_port("abc"), 0);
	EXPECT_EQ(parse_port("port"), 0);
	EXPECT_EQ(parse_port("http"), 0);

	// Test strings starting with non-digit
	EXPECT_EQ(parse_port("-80"), 0);  // Negative number
	EXPECT_EQ(parse_port("+80"), 0);  // Plus sign
	EXPECT_EQ(parse_port(" 80"), 0);  // Leading space
}

// Test boundary conditions
TEST(ParsePortTest, BoundaryConditions) {
	// Test port 0 (technically valid but often reserved)
	EXPECT_EQ(parse_port("0"), 0);

	// Test numbers beyond valid port range
	EXPECT_EQ(parse_port("65536"), 0);       // Just above max port
	EXPECT_EQ(parse_port("99999"), 0);       // Way above max port
	EXPECT_EQ(parse_port("4294967295"), 0);  // Max uint32
}

// Test partial parsing behavior (digits followed by non-digits)
TEST(ParsePortTest, PartialParsing) {
	// Test mixed alphanumeric - should parse leading digits
	EXPECT_EQ(parse_port("80abc"), 80);
	EXPECT_EQ(parse_port("8080xyz"), 8080);
	EXPECT_EQ(parse_port("443.5"), 443);     // Decimal point stops parsing
	EXPECT_EQ(parse_port("25565 "), 25565);  // Trailing space stops parsing

	// Test single digit followed by non-digit
	EXPECT_EQ(parse_port("8a0"), 8);
}

// Test string_view functionality
TEST(ParsePortTest, StringViewSupport) {
	// Test with std::string conversion to string_view
	std::string port_str = "25565";
	std::string_view port_sv = port_str;
	EXPECT_EQ(parse_port(port_sv), 25565);

	// Test with substring extraction
	std::string full_address = "server:25565:extra";
	std::string_view port_part = std::string_view(full_address).substr(7, 5);
	EXPECT_EQ(parse_port(port_part), 25565);
}

// Test leading zeros handling
TEST(ParsePortTest, LeadingZeros) {
	EXPECT_EQ(parse_port("0080"), 80);
	EXPECT_EQ(parse_port("00443"), 443);
	EXPECT_EQ(parse_port("000000000025565"), 25565);
}
