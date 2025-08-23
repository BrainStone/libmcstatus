#include "libmcstatus/JavaServer.hpp"

#include <boost/json.hpp>
#include <boost/uuid/string_generator.hpp>
#include <iostream>
#include <limits>
#include <random>

#include "libmcstatus/impl/SrvResolver.hpp"
#include "libmcstatus/impl/Utils.hpp"
#include "libmcstatus/McPacket.hpp"

namespace libmcstatus {

JavaServer::JavaServer(boost::asio::ip::tcp::endpoint server_address) : server_address{std::move(server_address)} {}

JavaServer::JavaServer(const boost::asio::ip::address& ip_address, boost::asio::ip::port_type port)
    : server_address{ip_address, port} {}

void JavaServer::handshake(boost::asio::ip::tcp::socket& socket) const {
	McPacket packet;
	packet.write_varint(0);
	packet.write_varint(47);  // Protocol version (this is 1.8-1.8.9, since this is the last time the basic
	                          // protocol was changed)
	packet.write_utf(server_address.address().to_string());
	packet.write_ushort(server_address.port());
	packet.write_varint(1);  // Intention to query status

	packet.write_to_socket(socket);
}

auto JavaServer::ping([[maybe_unused]] std::chrono::milliseconds timeout) const -> latency_t {
	static std::minstd_rand rng{std::random_device{}()};
	static std::uniform_int_distribution<std::int64_t> dist{0, std::numeric_limits<std::int64_t>::max()};

	boost::asio::io_context io_context;

	for (std::size_t attempt = 1;; ++attempt) {
		try {
			boost::asio::ip::tcp::socket socket(io_context);
			socket.connect(server_address);
			handshake(socket);

			using ping_token_t = std::int64_t;
			const ping_token_t ping_token = dist(rng);

			McPacket packet;
			packet.write_varint(1);  // Ping packet
			packet.write_long(ping_token);

			const std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

			packet.write_to_socket(socket);
			McPacket response = McPacket::read_from_socket(socket);

			const std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

			if (response.read_varint() != 1) {
				throw std::runtime_error("Invalid ping response packet");
			}

			const ping_token_t response_token = response.read_long();
			if (response_token != ping_token) {
				throw std::runtime_error("Invalid ping response token (expected " + std::to_string(ping_token) +
				                         ", got " + std::to_string(response_token) + ")");
			}

			return end - start;
		} catch (const std::exception&) {
			if (attempt >= RETRIES) {
				throw;
			}
		}
	}
}

auto JavaServer::status_impl([[maybe_unused]] std::chrono::milliseconds timeout) const -> JavaServerResponse* {
	boost::asio::io_context io_context;

	for (std::size_t attempt = 1;; ++attempt) {
		try {
			boost::asio::ip::tcp::socket socket(io_context);
			socket.connect(server_address);
			handshake(socket);

			McPacket packet;
			packet.write_varint(0);  // Request status

			const std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

			packet.write_to_socket(socket);
			McPacket response = McPacket::read_from_socket(socket);

			const std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

			if (response.read_varint() != 0) {
				throw std::runtime_error("Invalid status response packet");
			}

			return parse_status(end - start, response.read_utf());
		} catch (const std::exception&) {
			if (attempt >= RETRIES) {
				throw;
			}
		}
	}
}

JavaServer::JavaServerResponse* JavaServer::parse_status(latency_t latency, std::string_view status_response) {
	boost::json::object parsed_status = boost::json::parse(status_response).as_object();

	boost::json::object& players = parsed_status["players"].as_object();
	boost::json::object& version = parsed_status["version"].as_object();

	auto* status = new JavaServerResponse{};
	status->latency = latency;
	status->players->online = players["online"].as_int64();
	status->players->max = players["max"].as_int64();
	if (players.contains("sample") && players["sample"].is_array()) {
		boost::json::array& sample = players["sample"].as_array();
		status->players->sample = std::vector<JavaServerResponse::JavaStatusPlayers::JavaStatusPlayer>(sample.size());

		boost::uuids::string_generator uuid_gen;
		for (auto& player : sample) {
			status->players->sample->emplace_back(player.as_object()["name"].as_string().c_str(),
			                                      uuid_gen(player.as_object()["id"].as_string().c_str()));
		}
	}
	status->version->name = version["name"].as_string();
	status->version->protocol = version["protocol"].as_int64();
	status->motd = parsed_status["description"].is_string() ? parsed_status["description"].as_string().c_str()
	                                                        : boost::json::serialize(parsed_status["description"]);
	if (parsed_status.contains("enforcesSecureChat") && parsed_status["enforcesSecureChat"].is_bool())
		status->enforces_secure_chat = parsed_status["enforcesSecureChat"].as_bool();
	if (parsed_status.contains("favicon") && parsed_status["favicon"].is_string())
		status->icon = parsed_status["favicon"].as_string();
	if (parsed_status.contains("forgeData") || parsed_status.contains("modinfo")) {
		boost::json::value& forge_data =
		    parsed_status.contains("forgeData") ? parsed_status["forgeData"] : parsed_status["modinfo"];
		status->forge_data = boost::json::serialize(forge_data);
	}

	return status;
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
