#ifndef LIBMCSTATUS_SRVRESOLVER_HPP
#define LIBMCSTATUS_SRVRESOLVER_HPP

#include <compare>
#include <cstdint>
#include <string>
#include <string_view>
#include <set>
#include <random>

namespace libmcstatus::_impl {

struct SrvRecord {
	const uint16_t priority;
	const uint16_t weight;
	const uint16_t port;
	const std::string target;

	std::weak_ordering operator<=>(const SrvRecord&) const;
};

using records_t = std::multiset<SrvRecord>;

extern std::minstd_rand rng;

records_t resolve_srv(std::string_view service, std::string_view proto, std::string_view domain);
const SrvRecord& pick_record(const records_t& records);

}  // namespace libmcstatus::_impl

#endif  // LIBMCSTATUS_SRVRESOLVER_HPP
