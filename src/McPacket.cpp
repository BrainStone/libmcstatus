#include "libmcstatus/McPacket.hpp"

#include <algorithm>
#include <bit>
#include <concepts>
#include <stdexcept>

namespace libmcstatus {

template <std::integral T>
void write_int_be(McPacket::buffer_t& buffer, T value) {
	// Clang-tidy doesn't understand that this is essentially a compile-time check
#pragma clang diagnostic push
#pragma ide diagnostic ignored "Simplify"
	if constexpr (std::endian::native != std::endian::big) {
		value = std::byteswap(value);
	}
#pragma clang diagnostic pop

	constexpr std::size_t bytes = sizeof(T);
	const std::uint8_t* ptr = reinterpret_cast<std::uint8_t*>(&value);

	buffer.insert(buffer.end(), ptr, ptr + bytes);
}

template <std::integral T>
T read_int_be(const McPacket::buffer_iterator_t& head, std::size_t& head_offset) {
	constexpr std::size_t bytes = sizeof(T);
	T value;
	std::uint8_t* ptr = reinterpret_cast<std::uint8_t*>(&value);

	std::copy_n(head, bytes, ptr);
	head_offset += bytes;

	// Clang-tidy doesn't understand that this is essentially a compile-time check
#pragma clang diagnostic push
#pragma ide diagnostic ignored "Simplify"
	if constexpr (std::endian::native != std::endian::big) {
		value = std::byteswap(value);
	}
#pragma clang diagnostic pop

	return value;
}

McPacket::McPacket(const std::vector<std::uint8_t>& buffer) : buffer{buffer}, head_offset{0} {}

McPacket::McPacket(McPacket::buffer_iterator_t begin, McPacket::buffer_iterator_t end)
    : buffer{begin, end}, head_offset{0} {}

auto McPacket::get_head() -> buffer_iterator_t {
	return buffer.begin() + head_offset;
}

auto McPacket::advance_head() -> buffer_iterator_t {
	return buffer.begin() + head_offset++;
}

McPacket::McPacket() : buffer{}, head_offset{0} {}

void McPacket::reset() {
	head_offset = 0;
}

void McPacket::write_varint(std::int32_t value) {
	std::uint32_t remaining = static_cast<std::uint32_t>(value);

	for (int i = 0; i < 5; ++i) {
		if ((remaining & ~0x7F) == 0) {
			buffer.push_back(static_cast<std::uint8_t>(remaining));
			return;
		}

		buffer.push_back(static_cast<std::uint8_t>((remaining & 0x7F) | 0x80));
		remaining >>= 7;
	}

	throw std::runtime_error("The value \"" + std::to_string(value) + "\" is too big to send in a varint");
}

void McPacket::write_varlong(std::int64_t value) {
	std::uint64_t remaining = static_cast<std::uint64_t>(value);

	for (int i = 0; i < 10; ++i) {
		if ((remaining & ~0x7F) == 0) {
			buffer.push_back(static_cast<std::uint8_t>(remaining));
			return;
		}
		buffer.push_back(static_cast<std::uint8_t>((remaining & 0x7F) | 0x80));
		remaining >>= 7;
	}

	throw std::runtime_error("The value \"" + std::to_string(value) + "\" is too big to send in a varlong");
}

void McPacket::write_utf(std::string_view value) {
	// We really don't have a concept of encodings, so we just store the string as-is with a varint length prefix
	write_varint(value.size());
	buffer.insert(buffer.end(), value.begin(), value.end());
}

void McPacket::write_ascii(std::string_view value) {
	// We really don't have a concept of encodings, so we just store the string as-is with a trailing null byte
	buffer.insert(get_head(), value.begin(), value.end());
	buffer.push_back('\0');
}

void McPacket::write_short(std::int16_t value) {
	write_int_be(buffer, value);
}

void McPacket::write_ushort(std::uint16_t value) {
	write_int_be(buffer, value);
}

void McPacket::write_int(std::int32_t value) {
	write_int_be(buffer, value);
}

void McPacket::write_uint(std::uint32_t value) {
	write_int_be(buffer, value);
}

void McPacket::write_long(std::int64_t value) {
	write_int_be(buffer, value);
}

void McPacket::write_ulong(std::uint64_t value) {
	write_int_be(buffer, value);
}

void McPacket::write_bool(bool value) {
	buffer.push_back(value);
}

auto McPacket::write_to_buffer() -> buffer_t {
	McPacket length_packet{};
	length_packet.write_varint(buffer.size());

	length_packet.buffer.insert(length_packet.buffer.end(), buffer.begin(), buffer.end());

	return length_packet.buffer;
}

void McPacket::write_to_socket(boost::asio::ip::tcp::socket& socket) {
	socket.write_some(boost::asio::buffer(write_to_buffer()));
}

void McPacket::write_to_socket(boost::asio::ip::udp::socket& socket) {
	socket.send(boost::asio::buffer(write_to_buffer()));
}

bool McPacket::eof() const {
	return head_offset >= buffer.size();
}

std::int32_t McPacket::read_varint() {
	std::uint32_t result = 0;

	for (int i = 0; i < 5; ++i) {
		if (eof()) {
			throw std::runtime_error("Unexpected end of buffer while reading varint");
		}

		std::uint8_t part = *advance_head();
		result |= static_cast<std::uint32_t>(part & 0x7F) << (7 * i);

		if ((part & 0x80) == 0) {
			return static_cast<std::int32_t>(result);
		}
	}

	throw std::runtime_error("Received varint is too big!");
}

std::int64_t McPacket::read_varlong() {
	std::uint64_t result = 0;

	for (int i = 0; i < 10; ++i) {
		if (eof()) {
			throw std::runtime_error("Unexpected end of buffer while reading varlong");
		}

		std::uint8_t part = *advance_head();
		result |= static_cast<std::uint64_t>(part & 0x7F) << (7 * i);

		if ((part & 0x80) == 0) {
			return static_cast<std::int64_t>(result);
		}
	}

	throw std::runtime_error("Received varlong is too big!");
}

std::string McPacket::read_utf() {
	const std::int32_t length = read_varint();
	std::string res{get_head(), get_head() + length};

	head_offset += length;
	return res;
}

std::string McPacket::read_ascii() {
	const buffer_iterator_t head = get_head();
	const buffer_iterator_t it = std::find(head, buffer.cend(), '\0');
	std::string res{head, it};

	head_offset += (it - head) + 1;
	return res;
}

std::int16_t McPacket::read_short() {
	return read_int_be<std::int16_t>(get_head(), head_offset);
}

std::uint16_t McPacket::read_ushort() {
	return read_int_be<std::uint16_t>(get_head(), head_offset);
}

std::int32_t McPacket::read_int() {
	return read_int_be<std::int32_t>(get_head(), head_offset);
}

std::uint32_t McPacket::read_uint() {
	return read_int_be<std::uint32_t>(get_head(), head_offset);
}

std::int64_t McPacket::read_long() {
	return read_int_be<std::int64_t>(get_head(), head_offset);
}

std::uint64_t McPacket::read_ulong() {
	return read_int_be<std::uint64_t>(get_head(), head_offset);
}

bool McPacket::read_bool() {
	return *advance_head();
}

McPacket McPacket::read_from_buffer(const buffer_t& buffer) {
	McPacket length_packet{buffer};
	std::int32_t length = length_packet.read_varint();

	return McPacket{length_packet.get_head(), length_packet.get_head() + length};
}

McPacket McPacket::read_from_socket(boost::asio::ip::tcp::socket& socket) {
	buffer_t buffer;
	socket.read_some(boost::asio::buffer(buffer));

	return read_from_buffer(buffer);
}
McPacket McPacket::read_from_socket(boost::asio::ip::udp::socket& socket) {
	buffer_t buffer;
	socket.receive(boost::asio::buffer(buffer));

	return read_from_buffer(buffer);
}

}  // namespace libmcstatus::_impl
