#ifndef LIBMCSTATUS_MCSERVER_HPP
#define LIBMCSTATUS_MCSERVER_HPP

#include <chrono>
#include <cstdint>
#include <string_view>

namespace libmcstatus {

class McServer {
public:
	static constexpr std::chrono::seconds DEFAULT_TIMEOUT{3};
	static std::size_t RETRIES; // 3

	[[nodiscard]] virtual std::chrono::high_resolution_clock::duration ping() const final {
		return ping(DEFAULT_TIMEOUT);
	}
	virtual void status() const final {
		return status(DEFAULT_TIMEOUT);
	}

	[[nodiscard]] virtual std::chrono::high_resolution_clock::duration ping(
	    std::chrono::milliseconds timeout) const = 0;
	virtual void status(std::chrono::milliseconds timeout) const = 0;
	
	virtual std::string to_string() const = 0;
	
	friend std::ostream& operator<<(std::ostream& os, const McServer& server);
};

}  // namespace libmcstatus

#endif  // LIBMCSTATUS_MCSERVER_HPP
