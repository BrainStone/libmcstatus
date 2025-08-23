#ifndef LIBMCSTATUS_JAVASERVER_HPP
#define LIBMCSTATUS_JAVASERVER_HPP

#include <boost/asio/ip/tcp.hpp>
#include <cstdint>
#include <string_view>

#include "McServer.hpp"

namespace libmcstatus {

class JavaServer : public McServer {
private:
	boost::asio::ip::tcp::endpoint server_address;
	
	void handshake(boost::asio::ip::tcp::socket& socket) const;

public:
	static constexpr boost::asio::ip::port_type DEFAULT_PORT{25565};

	explicit JavaServer(boost::asio::ip::tcp::endpoint server_address);
	JavaServer(const boost::asio::ip::address& ip_address, boost::asio::ip::port_type port);

	using McServer::ping;
	using McServer::status;

	[[nodiscard]] std::chrono::high_resolution_clock::duration ping(std::chrono::milliseconds timeout) const override;
	void status(std::chrono::milliseconds timeout) const override;

	static JavaServer lookup(std::string_view host_address);

	[[nodiscard]] std::string to_string() const override;
};

}  // namespace libmcstatus

#endif  // LIBMCSTATUS_JAVASERVER_HPP
