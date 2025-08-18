#ifndef LIBMCSTATUS_UTILS_HPP
#define LIBMCSTATUS_UTILS_HPP

#include <boost/asio/ip/basic_endpoint.hpp>
#include <string_view>

namespace libmcstatus::_impl {

boost::asio::ip::port_type parse_port(std::string_view port_string);

}

#endif  // LIBMCSTATUS_UTILS_HPP
