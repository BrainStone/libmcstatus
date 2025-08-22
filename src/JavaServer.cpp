#include "libmcstatus/JavaServer.hpp"

#include "libmcstatus/impl/SrvResolver.hpp"
#include "libmcstatus/impl/Utils.hpp"

namespace libmcstatus {

JavaServer::JavaServer(boost::asio::ip::tcp::endpoint server_address) : server_address{std::move(server_address)} {}

JavaServer::JavaServer(const boost::asio::ip::address& ip_address, boost::asio::ip::port_type port)
    : server_address{ip_address, port} {}

std::chrono::high_resolution_clock::duration JavaServer::ping(std::chrono::milliseconds timeout) const {
	const std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

	status(timeout);

	const std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

	return end - start;
}

void JavaServer::status(std::chrono::milliseconds timeout) const {
	// TODO: Implement me
}

JavaServer JavaServer::lookup(std::string_view host_address) {
	boost::system::error_code ec;
	boost::asio::io_context io_context;
	boost::asio::ip::tcp::resolver resolver(io_context);

	const auto colon = host_address.rfind(':');
	const bool colon_found = colon != std::string_view::npos;
	std::string host = std::string{host_address.substr(0, colon)};
	std::string port = colon_found ? std::string{host_address.substr(colon + 1)} : std::to_string(DEFAULT_PORT);

	// Check if the host is an IP address
	const boost::asio::ip::address address = boost::asio::ip::make_address(host, ec);

	if (!ec.failed()) {
		return JavaServer{address, colon_found ? _impl::parse_port(port) : DEFAULT_PORT};
	}

	// Try SRV lookup if we have a plain hostname (without port)
	if (!colon_found) {
		try {
			_impl::SrvRecord record = _impl::pick_record(_impl::resolve_srv("minecraft", "tcp", host));
			host = record.target;
			port = std::to_string(record.port);
		} catch (std::runtime_error& e) {
			// No SRV records found, continue with normal DNS lookup
		}
	}

	// Resolve the host
	auto results = resolver.resolve(host, port, ec);

	if (ec.failed()) {
		throw std::runtime_error("Failed to resolve host \"" + std::string{host_address} + "\": " + ec.message());
	}

	// Prefer IPv4 addresses
	for (const auto& endpoint : results) {
		if (endpoint.endpoint().address().is_v4()) {
			// Use this IPv4 address
			return JavaServer{endpoint};
		}
	}

	// Fallback to IPv6 addresses
	return JavaServer{results.begin()->endpoint()};
}

std::string JavaServer::to_string() const {
	return server_address.address().to_string() + ":" + std::to_string(server_address.port());
}

}  // namespace libmcstatus
