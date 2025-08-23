#include "libmcstatus/McPacket.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>
#include <vector>

using namespace libmcstatus;

class McPacketAccessor : public McPacket {
public:
	explicit McPacketAccessor(const buffer_t& buffer) : McPacket{buffer} {}
};

// Test constructors
TEST(McPacketTest, DefaultConstructor) {
	McPacket packet{};
	EXPECT_NO_THROW(packet.reset());
}

// Test reset functionality
TEST(McPacketTest, Reset) {
	McPacket packet{};

	packet.write_bool(true);
	packet.write_bool(false);

	// Read one value
	EXPECT_EQ(packet.read_bool(), true);

	// Reset and read from the beginning again
	packet.reset();
	EXPECT_EQ(packet.read_bool(), true);
	EXPECT_EQ(packet.read_bool(), false);
}

// Test varint functionality
TEST(McPacketTest, WriteReadVarint) {
	// Test various varint values
	std::vector<std::int32_t> test_values = {0,     1,       127,     128,       255,       256,      16383,
	                                         16384, 2097151, 2097152, 268435455, 268435456, -1,       -127,
	                                         -128,  -129,    -255,    -256,      INT32_MAX, INT32_MIN};

	for (std::int32_t value : test_values) {
		McPacket packet;
		packet.write_varint(value);
		EXPECT_EQ(packet.read_varint(), value) << "Failed for value: " << value;
	}
}

TEST(McPacketTest, ReadVarintUnexpectedEnd) {
	McPacket packet{};

	// Test reading varint from empty buffer
	EXPECT_THROW(std::ignore = packet.read_varint(), McPacket::PacketDecodingError);

	// Test reading incomplete varint
	packet = McPacketAccessor{{0x80}};
	EXPECT_THROW(std::ignore = packet.read_varint(), McPacket::PacketDecodingError);
}

TEST(McPacketTest, ReadVarintTooBig) {
	McPacket packet = McPacketAccessor{{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x80}};

	EXPECT_THROW(std::ignore = packet.read_varint(), McPacket::PacketDecodingError);
}

// Test varlong functionality
TEST(McPacketTest, WriteReadVarlong) {
	// Test various varlong values
	std::vector<std::int64_t> test_values = {0,          1,  127,  128,  16383, 16384,     2097151,  2097152,
	                                         4294967296, -1, -127, -128, -129,  INT64_MIN, INT64_MIN};

	for (std::int64_t value : test_values) {
		McPacket packet;
		packet.write_varlong(value);
		EXPECT_EQ(packet.read_varlong(), value) << "Failed for value: " << value;
	}
}

TEST(McPacketTest, ReadVarlongUnexpectedEnd) {
	McPacket packet{};

	EXPECT_THROW(std::ignore = packet.read_varlong(), McPacket::PacketDecodingError);
}

TEST(McPacketTest, ReadVarlongTooBig) {
	McPacket packet = McPacketAccessor{{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x80}};

	EXPECT_THROW(std::ignore = packet.read_varlong(), McPacket::PacketDecodingError);
}

// Test string functionality
TEST(McPacketTest, WriteReadUtf) {
	std::string test_strings[] = {
	    "", "Hello", "Hello, World!", "Special chars: éñ中文", std::string(1000, 'A')  // Long string
	};

	for (const auto& str : test_strings) {
		McPacket test_packet;
		test_packet.write_utf(str);
		EXPECT_EQ(test_packet.read_utf(), str) << "Failed for string: " << str;
	}
}

TEST(McPacketTest, WriteReadAscii) {
	std::string test_strings[] = {"", "Hello", "Hello, World!", "ASCII only string"};

	for (const auto& str : test_strings) {
		McPacket test_packet;
		test_packet.write_ascii(str);
		EXPECT_EQ(test_packet.read_ascii(), str) << "Failed for string: " << str;
	}
}

// Test integer functionality
TEST(McPacketTest, WriteReadShort) {
	std::vector<std::int16_t> test_values = {0, 1, -1, 127, -128, 32767, -32768};

	for (std::int16_t value : test_values) {
		McPacket test_packet;
		test_packet.write_short(value);
		EXPECT_EQ(test_packet.read_short(), value) << "Failed for value: " << value;
	}
}

TEST(McPacketTest, WriteReadUShort) {
	std::vector<std::uint16_t> test_values = {0, 1, 255, 256, 32767, 65535};

	for (std::uint16_t value : test_values) {
		McPacket test_packet;
		test_packet.write_ushort(value);
		EXPECT_EQ(test_packet.read_ushort(), value) << "Failed for value: " << value;
	}
}

TEST(McPacketTest, WriteReadInt) {
	std::vector<std::int32_t> test_values = {0, 1, -1, 127, -128, 32767, -32768, 2147483647, -2147483648};

	for (std::int32_t value : test_values) {
		McPacket test_packet;
		test_packet.write_int(value);
		EXPECT_EQ(test_packet.read_int(), value) << "Failed for value: " << value;
	}
}

TEST(McPacketTest, WriteReadUInt) {
	std::vector<std::uint32_t> test_values = {0, 1, 255, 256, 65535, 65536, 2147483647U, 4294967295U};

	for (std::uint32_t value : test_values) {
		McPacket test_packet;
		test_packet.write_uint(value);
		EXPECT_EQ(test_packet.read_uint(), value) << "Failed for value: " << value;
	}
}

TEST(McPacketTest, WriteReadLong) {
	std::vector<std::int64_t> test_values = {0,      1,          -1,          127,       -128,     32767,
	                                         -32768, 2147483647, -2147483648, INT64_MIN, INT64_MIN};

	for (std::int64_t value : test_values) {
		McPacket test_packet;
		test_packet.write_long(value);
		EXPECT_EQ(test_packet.read_long(), value) << "Failed for value: " << value;
	}
}

TEST(McPacketTest, WriteReadULong) {
	std::vector<std::uint64_t> test_values = {
	    0, 1, 255, 256, 65535, 65536, 4294967295ULL, 9223372036854775807ULL, 18446744073709551615ULL};

	for (std::uint64_t value : test_values) {
		McPacket test_packet;
		test_packet.write_ulong(value);
		EXPECT_EQ(test_packet.read_ulong(), value) << "Failed for value: " << value;
	}
}

TEST(McPacketTest, WriteReadBool) {
	McPacket test_packet;
	test_packet.write_bool(true);
	test_packet.write_bool(false);
	test_packet.write_bool(true);

	EXPECT_EQ(test_packet.read_bool(), true);
	EXPECT_EQ(test_packet.read_bool(), false);
	EXPECT_EQ(test_packet.read_bool(), true);
}

// Test buffer operations
TEST(McPacketTest, WriteToBuffer) {
	McPacket packet{};

	packet.write_varint(42);
	packet.write_bool(true);
	packet.write_short(1337);

	auto buffer = packet.write_to_buffer();

	// Buffer should contain: length varint + original data
	EXPECT_FALSE(buffer.empty());

	// Test that we can reconstruct the packet from the buffer
	McPacket reconstructed = McPacket::read_from_buffer(buffer);

	EXPECT_EQ(reconstructed.read_varint(), 42);
	EXPECT_EQ(reconstructed.read_bool(), true);
	EXPECT_EQ(reconstructed.read_short(), 1337);
}

TEST(McPacketTest, ReadFromBuffer) {
	// Create a buffer manually with a length prefix
	std::vector<std::uint8_t> buffer;

	// Add length (3 bytes: 1 byte for bool, 2 bytes for short)
	buffer.push_back(3);

	// Add data
	buffer.push_back(1);     // bool true
	buffer.push_back(0x05);  // short 1337 (big-endian)
	buffer.push_back(0x39);

	McPacket reconstructed = McPacket::read_from_buffer(buffer);

	EXPECT_EQ(reconstructed.read_bool(), true);
	EXPECT_EQ(reconstructed.read_short(), 1337);
}

TEST(McPacketTest, ReadFromBufferShorterThanExpected) {
	// Create a buffer with length that claims more data than actually present
	std::vector<std::uint8_t> buffer;

	// Add length claiming 5 bytes of data
	buffer.push_back(5);

	// But only add 3 bytes of actual data
	buffer.push_back(1);  // byte 1
	buffer.push_back(2);  // byte 2
	buffer.push_back(3);  // byte 3

	// Should throw runtime_error because the buffer is shorter than expected
	EXPECT_THROW(std::ignore = McPacket::read_from_buffer(buffer), McPacket::PacketDecodingError);
}

// Test mixed operations
TEST(McPacketTest, MixedWriteRead) {
	McPacket packet{};

	packet.write_varint(100);
	packet.write_utf("test string");
	packet.write_bool(false);
	packet.write_int(999888777);
	packet.write_varlong(123456789012345LL);

	EXPECT_EQ(packet.read_varint(), 100);
	EXPECT_EQ(packet.read_utf(), "test string");
	EXPECT_EQ(packet.read_bool(), false);
	EXPECT_EQ(packet.read_int(), 999888777);
	EXPECT_EQ(packet.read_varlong(), 123456789012345LL);
}

// Test edge cases
TEST(McPacketTest, EmptyBuffer) {
	McPacket packet{};

	auto buffer = packet.write_to_buffer();

	// Should contain just the length (0)
	EXPECT_EQ(buffer.size(), 1);  // Single byte for varint 0
	EXPECT_EQ(buffer[0], 0);
}

TEST(McPacketTest, RoundTripThroughBuffer) {
	McPacket packet{};

	// Write complex data
	packet.write_varint(-42);
	packet.write_utf("Hello, 世界!");
	packet.write_bool(true);
	packet.write_long(-9876543210LL);

	// Convert to buffer and back
	auto buffer = packet.write_to_buffer();
	McPacket reconstructed = McPacket::read_from_buffer(buffer);

	// Verify data integrity
	EXPECT_EQ(reconstructed.read_varint(), -42);
	EXPECT_EQ(reconstructed.read_utf(), "Hello, 世界!");
	EXPECT_EQ(reconstructed.read_bool(), true);
	EXPECT_EQ(reconstructed.read_long(), -9876543210LL);
}

TEST(McPacketSocketTest, TcpWriteAndRead) {
	boost::asio::io_context io_context;
	boost::asio::ip::tcp::acceptor acceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 0));

	auto server_endpoint = acceptor.local_endpoint();

	// Server socket
	boost::asio::ip::tcp::socket server_socket(io_context);

	// Client socket
	boost::asio::ip::tcp::socket client_socket(io_context);
	client_socket.connect(server_endpoint);
	acceptor.accept(server_socket);

	// Test writing from client and reading on server
	McPacket write_packet;
	write_packet.write_varint(42);
	write_packet.write_utf("Hello World");

	write_packet.write_to_socket(client_socket);

	McPacket read_packet = McPacket::read_from_socket(server_socket);

	EXPECT_EQ(read_packet.read_varint(), 42);
	EXPECT_EQ(read_packet.read_utf(), "Hello World");
}

TEST(McPacketSocketTest, UdpWriteAndRead) {
	boost::asio::io_context io_context;

	// Server socket
	boost::asio::ip::udp::socket server_socket(io_context,
	                                           boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 0));
	auto server_endpoint = server_socket.local_endpoint();

	// Client socket
	boost::asio::ip::udp::socket client_socket(io_context);
	client_socket.open(boost::asio::ip::udp::v4());
	client_socket.connect(server_endpoint);

	// Test the UDP methods
	McPacket write_packet;
	write_packet.write_varint(42);
	write_packet.write_utf("Hello World");
	write_packet.write_to_socket(client_socket);

	McPacket read_packet = McPacket::read_from_socket(server_socket);

	EXPECT_EQ(read_packet.read_varint(), 42);
	EXPECT_EQ(read_packet.read_utf(), "Hello World");
}
