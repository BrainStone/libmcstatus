#include "libmcstatus/McServer.hpp"

namespace libmcstatus {

std::size_t McServer::RETRIES = 3;

std::ostream& operator<<(std::ostream& os, const McServer& server) {
	return os << server.to_string();
}

}
