#include "libmcstatus/impl/Utils.hpp"

#include <charconv>

namespace libmcstatus::_impl {

boost::asio::ip::port_type parse_port(std::string_view port_str) {
	boost::asio::ip::port_type port_value;

	const std::errc ec = std::from_chars(port_str.data(), port_str.data() + port_str.size(), port_value).ec;

	return (ec == std::errc()) ? port_value : 0;
}

}  // namespace libmcstatus::_impl
