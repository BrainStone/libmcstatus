#ifndef LIBMCSTATUS_MCSERVER_HPP
#define LIBMCSTATUS_MCSERVER_HPP

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace libmcstatus {

class McServer {
public:
	using latency_t = std::chrono::high_resolution_clock::duration;

	struct McStatusResponse {
		struct McStatusPlayers {
			std::uint32_t online;
			std::uint32_t max;
		};
		struct McStatusVersion {
			std::string name;
			std::uint32_t protocol;
		};

		McStatusPlayers players;
		McStatusVersion version;
		std::string motd;  // TODO: Own class?
		latency_t latency;
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
