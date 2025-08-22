#include "libmcstatus/impl/SrvResolver.hpp"

#include <arpa/nameser.h>
#include <netinet/in.h>
#include <resolv.h>

#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>

namespace libmcstatus::_impl {

std::weak_ordering SrvRecord::operator<=>(const SrvRecord& rhs) const {
	return this->priority <=> rhs.priority;
}

records_t resolve_srv(std::string_view service, std::string_view proto, std::string_view domain) {
	std::ostringstream ss;
	ss << "_" << service << "._" << proto << "." << domain;
	const std::string qname = ss.str();

	// Buffer for the DNS response
	unsigned char answer[NS_PACKETSZ];
	const int len = res_query(qname.c_str(), C_IN, T_SRV, answer, sizeof(answer));
	if (len < 0) {
		throw std::runtime_error("SRV query failed for " + qname);
	}

	ns_msg handle;
	if (ns_initparse(answer, len, &handle) < 0) {
		throw std::runtime_error("ns_initparse failed");
	}

	std::uint16_t count = ns_msg_count(handle, ns_s_an);
	records_t results;

	for (std::uint16_t i = 0; i < count; i++) {
		ns_rr rr;
		if (ns_parserr(&handle, ns_s_an, i, &rr) < 0) {
			continue;  // skip malformed
		}

		if (ns_rr_type(rr) != T_SRV) {
			continue;  // Skip non-SRV records
		}

		const unsigned char* rdata = ns_rr_rdata(rr);

		char name[1024];
		if (dn_expand(ns_msg_base(handle), ns_msg_end(handle), rdata + 6, name, sizeof(name)) < 0) {
			continue;  // bad target, skip
		}

		results.emplace(static_cast<uint16_t>((rdata[0] << 8) | rdata[1]),
		                static_cast<uint16_t>((rdata[2] << 8) | rdata[3]),
		                static_cast<uint16_t>((rdata[4] << 8) | rdata[5]), name);
	}

	return results;
}

// Initialize the random number generator with an actual random seed
std::minstd_rand rng{std::random_device{}()};

const SrvRecord& pick_record(const records_t& records) {
	if (records.empty()) {
		throw std::runtime_error("No SRV records found");
	}

	using weight_t = std::remove_const_t<decltype(SrvRecord::weight)>;

	const SrvRecord& first = *records.begin();
	weight_t total_weight{0};

	for (const SrvRecord& record : records) {
		if (record > first) {
			break;
		}

		total_weight += record.weight;
	}

	if (total_weight == 0) {
		return first;
	}

	std::uniform_int_distribution<weight_t> dist{0, total_weight};
	weight_t rand_weight = dist(rng);

	weight_t weight_running_sum{0};
	for (const SrvRecord& record : records) {
		weight_running_sum += record.weight;
		if (rand_weight <= weight_running_sum) {
			return record;
		}
	}

	return first;
}

}  // namespace libmcstatus::_impl
