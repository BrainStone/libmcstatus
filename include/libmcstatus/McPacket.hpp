#ifndef LIBMCSTATUS_MCPACKET_HPP
#define LIBMCSTATUS_MCPACKET_HPP

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace libmcstatus {

class McPacket {
public:
	using buffer_t = std::vector<std::uint8_t>;
	using buffer_iterator_t = buffer_t::const_iterator;

protected:
	buffer_t buffer;
	std::size_t head_offset;

	explicit McPacket(const std::vector<std::uint8_t>& buffer);

	McPacket(buffer_iterator_t begin, buffer_iterator_t end);

	buffer_iterator_t get_head();
	buffer_iterator_t advance_head();

public:
	// Public Constructors
	McPacket();

	// Helpers
	void reset();

	// Write Functions
	void write_varint(std::int32_t value);
	void write_varlong(std::int64_t value);

	void write_utf(std::string_view value);
	void write_ascii(std::string_view value);

	void write_short(std::int16_t value);
	void write_ushort(std::uint16_t value);

	void write_int(std::int32_t value);
	void write_uint(std::uint32_t value);

	void write_long(std::int64_t value);
	void write_ulong(std::uint64_t value);

	void write_bool(bool value);

	buffer_t write_to_buffer();
	void write_to_socket(boost::asio::ip::tcp::socket& socket);
	void write_to_socket(boost::asio::ip::udp::socket& socket);

	// Read Functions
	[[nodiscard]] bool eof() const;

	[[nodiscard]] std::int32_t read_varint();
	[[nodiscard]] std::int64_t read_varlong();

	[[nodiscard]] std::string read_utf();
	[[nodiscard]] std::string read_ascii();

	[[nodiscard]] std::int16_t read_short();
	[[nodiscard]] std::uint16_t read_ushort();

	[[nodiscard]] std::int32_t read_int();
	[[nodiscard]] std::uint32_t read_uint();

	[[nodiscard]] std::int64_t read_long();
	[[nodiscard]] std::uint64_t read_ulong();

	[[nodiscard]] bool read_bool();

	[[nodiscard]] static McPacket read_from_buffer(const buffer_t& buffer);
	[[nodiscard]] static McPacket read_from_socket(boost::asio::ip::tcp::socket& socket);
	[[nodiscard]] static McPacket read_from_socket(boost::asio::ip::udp::socket& socket);
};

}  // namespace libmcstatus

#endif  // LIBMCSTATUS_MCPACKET_HPP
