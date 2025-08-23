#ifndef LIBMCSTATUS_JAVASERVER_HPP
#define LIBMCSTATUS_JAVASERVER_HPP

#include <boost/asio/ip/tcp.hpp>
#include <boost/uuid/uuid.hpp>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>

#include "McServer.hpp"

namespace libmcstatus {

class JavaServer : public McServer {
public:
	struct JavaServerResponse : public McStatusResponse {
		struct JavaStatusPlayers : public McStatusPlayers {
			struct JavaStatusPlayer {
				std::string name{};
				boost::uuids::uuid id{};
			};

			std::optional<std::vector<JavaStatusPlayer>> sample{};
		};
		struct JavaStatusVersion : public McStatusVersion {};

		inline explicit JavaServerResponse(
		    std::shared_ptr<JavaStatusPlayers> players = std::make_shared<JavaStatusPlayers>(),
		    std::shared_ptr<JavaStatusVersion> version = std::make_shared<JavaStatusVersion>())
		    : McStatusResponse{std::static_pointer_cast<McStatusPlayers>(players),
		                       std::static_pointer_cast<McStatusVersion>(version)},
		      players{std::move(players)},
		      version{std::move(version)} {}

		std::shared_ptr<JavaStatusPlayers> players;
		std::shared_ptr<JavaStatusVersion> version;

		std::optional<bool> enforces_secure_chat{};
		std::optional<std::string> icon{};
		std::optional<std::string> forge_data{};  // TODO: own class?
	};

protected:
	boost::asio::ip::tcp::endpoint server_address;

	void handshake(boost::asio::ip::tcp::socket& socket) const;

public:
	static constexpr boost::asio::ip::port_type DEFAULT_PORT{25565};

	explicit JavaServer(boost::asio::ip::tcp::endpoint server_address);
	JavaServer(const boost::asio::ip::address& ip_address, boost::asio::ip::port_type port);

	using McServer::ping;
	[[nodiscard]] latency_t ping(std::chrono::milliseconds timeout) const override;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "HidingNonVirtualFunction"
	[[nodiscard]] inline std::unique_ptr<JavaServerResponse> status() const {
		return status(DEFAULT_TIMEOUT);
	}
	[[nodiscard]] inline std::unique_ptr<JavaServerResponse> status(std::chrono::milliseconds timeout) const {
		return std::unique_ptr<JavaServerResponse>{status_impl(timeout)};
	}
#pragma clang diagnostic pop

protected:
	[[nodiscard]] JavaServerResponse* status_impl(std::chrono::milliseconds timeout) const override;
	[[nodiscard]] static JavaServerResponse* parse_status(latency_t latency, std::string_view status_response);

public:
	static JavaServer lookup(std::string_view host_address);

	[[nodiscard]] std::string to_string() const override;
};

}  // namespace libmcstatus

#endif  // LIBMCSTATUS_JAVASERVER_HPP
