#ifndef LIBMCSTATUS_MCSERVER_HPP
#define LIBMCSTATUS_MCSERVER_HPP

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace libmcstatus {

class McServer {
public:
	using latency_t = std::chrono::high_resolution_clock::duration;
	using response_int_t = std::int64_t;

	struct McStatusResponse {
		struct McStatusPlayers {
			response_int_t online{-1};
			response_int_t max{-1};

			virtual ~McStatusPlayers() = default;
		};
		struct McStatusVersion {
			std::string name{};
			response_int_t protocol{-1};

			virtual ~McStatusVersion() = default;
		};

		inline explicit McStatusResponse(std::shared_ptr<McStatusPlayers> players = std::make_shared<McStatusPlayers>(),
		                                 std::shared_ptr<McStatusVersion> version = std::make_shared<McStatusVersion>())
		    : players{std::move(players)}, version{std::move(version)} {}
		virtual ~McStatusResponse() = default;

		std::shared_ptr<McStatusPlayers> players;
		std::shared_ptr<McStatusVersion> version;
		std::string motd{};  // TODO: Own class?
		latency_t latency{-1};
	};

	static constexpr std::chrono::seconds DEFAULT_TIMEOUT{3};
	static std::size_t RETRIES;  // 3

	virtual ~McServer() = default;

	[[nodiscard]] inline virtual latency_t ping() const final {
		return ping(DEFAULT_TIMEOUT);
	}
	[[nodiscard]] virtual latency_t ping(std::chrono::milliseconds timeout) const = 0;

	[[nodiscard]] inline std::unique_ptr<McStatusResponse> status() const {
		return status(DEFAULT_TIMEOUT);
	}
	[[nodiscard]] inline std::unique_ptr<McStatusResponse> status(std::chrono::milliseconds timeout) const {
		return std::unique_ptr<McStatusResponse>{status_impl(timeout)};
	}

protected:
	[[nodiscard]] virtual McStatusResponse* status_impl(std::chrono::milliseconds timeout) const = 0;

public:
	[[nodiscard]] virtual std::string to_string() const = 0;

	friend std::ostream& operator<<(std::ostream& os, const McServer& server);
};

}  // namespace libmcstatus

#endif  // LIBMCSTATUS_MCSERVER_HPP
