#include "libmcstatus/impl/SrvResolver.hpp"

#include <gtest/gtest.h>

#include <map>
#include <random>

#include "GlobalMocks.hpp"

using namespace libmcstatus::_impl;

using ::testing::_;
using ::testing::Return;

class SrvResolverTest : public ::testing::Test {
protected:
	ResolverFuncs mock;
	void SetUp() override {
		g_mock = &mock;
	}
	void TearDown() override {
		g_mock = nullptr;
	}
};

TEST_F(SrvResolverTest, QueryFailsThrows) {
	EXPECT_CALL(mock, res_query(_, _, _, _, _)).WillOnce(Return(-1));

	EXPECT_THROW({ resolve_srv("minecraft", "tcp", "example.com"); }, std::runtime_error);
}

TEST_F(SrvResolverTest, QuerySucceedsButParseInitFails) {
	EXPECT_CALL(mock, res_query(_, _, _, _, _)).WillOnce(Return(512));
	EXPECT_CALL(mock, ns_initparse(_, _, _)).WillOnce(Return(-1));

	EXPECT_THROW({ resolve_srv("minecraft", "tcp", "example.com"); }, std::runtime_error);
}

TEST_F(SrvResolverTest, QuerySucceedsButNoAnswerRecords) {
	ns_msg handle;
	EXPECT_CALL(mock, res_query(_, _, _, _, _)).WillOnce(Return(512));
	EXPECT_CALL(mock, ns_initparse(_, _, _)).WillOnce([&](const unsigned char* msg, int msglen, ns_msg* h) {
		*h = handle;
		return 0;
	});

	// Mock no records
	handle._counts[ns_s_an] = 0;

	auto result = resolve_srv("minecraft", "tcp", "example.com");
	EXPECT_TRUE(result.empty());
}

TEST_F(SrvResolverTest, QuerySucceedsWithSingleSrvRecord) {
	ns_msg handle;

	EXPECT_CALL(mock, res_query(_, _, _, _, _)).WillOnce(Return(512));
	EXPECT_CALL(mock, ns_initparse(_, _, _)).WillOnce([&](const unsigned char* msg, int msglen, ns_msg* h) {
		*h = handle;
		return 0;
	});

	// Mock a single SRV record
	handle._counts[ns_s_an] = 1;
	EXPECT_CALL(mock, ns_parserr(_, ns_s_an, 0, _)).WillOnce([&](ns_msg* handle, ns_sect section, int rrnum, ns_rr* r) {
		// Simulate SRV record data: priority=10, weight=5, port=25565, target="mc.example.com"
		static unsigned char srv_data[] = {0x00, 0x0A,  // Priority: 10
		                                   0x00, 0x05,  // Weight: 5
		                                   0x63, 0xDD,  // Port: 25565
		                                   0x02, 'm',  'c', 0x07, 'e', 'x', 'a', 'm',
		                                   'p',  'l',  'e', 0x03, 'c', 'o', 'm', 0x00};
		r->type = T_SRV;
		r->rdlength = sizeof(srv_data);
		r->rdata = srv_data;
		handle->_msg = r->rdata;
		handle->_eom = r->rdata + r->rdlength;
		return 0;
	});

	auto result = resolve_srv("minecraft", "tcp", "example.com");

	EXPECT_EQ(result.size(), 1);
	const auto& record = *result.begin();
	EXPECT_EQ(record.priority, 10);
	EXPECT_EQ(record.weight, 5);
	EXPECT_EQ(record.port, 25565);
	EXPECT_EQ(record.target, "mc.example.com");
}

TEST_F(SrvResolverTest, QuerySucceedsWithMultipleSrvRecords) {
	ns_msg handle;

	EXPECT_CALL(mock, res_query(_, _, _, _, _)).WillOnce(Return(1024));
	EXPECT_CALL(mock, ns_initparse(_, _, _)).WillOnce([&](const unsigned char* msg, int msglen, ns_msg* h) {
		*h = handle;
		return 0;
	});

	handle._counts[ns_s_an] = 2;
	
	// Mock first SRV record: priority=10, weight=5, port=25565, target="mc1.example.com"
	EXPECT_CALL(mock, ns_parserr(_, ns_s_an, 0, _)).WillOnce([&](ns_msg* handle, ns_sect section, int rrnum, ns_rr* r) {
		static unsigned char srv_data1[] = {0x00, 0x0A,  // Priority: 10
		                                    0x00, 0x05,  // Weight: 5
		                                    0x63, 0xDD,  // Port: 25565
		                                    0x03, 'm',  'c', '1',  0x07, 'e', 'x', 'a', 'm',
		                                    'p',  'l',  'e', 0x03, 'c',  'o', 'm', 0x00};
		r->type = ns_t_srv;
		r->rdlength = sizeof(srv_data1);
		r->rdata = srv_data1;
		handle->_msg = r->rdata;
		handle->_eom = r->rdata + r->rdlength;
		return 0;
	});

	// Mock second SRV record: priority=5, weight=10, port=25566, target="mc2.example.com"
	EXPECT_CALL(mock, ns_parserr(_, ns_s_an, 1, _)).WillOnce([&](ns_msg* handle, ns_sect section, int rrnum, ns_rr* r) {
		static unsigned char srv_data2[] = {0x00, 0x05,  // Priority: 5
		                                    0x00, 0x0A,  // Weight: 10
		                                    0x63, 0xDE,  // Port: 25566
		                                    0x03, 'm',  'c', '2',  0x07, 'e', 'x', 'a', 'm',
		                                    'p',  'l',  'e', 0x03, 'c',  'o', 'm', 0x00};
		r->type = ns_t_srv;
		r->rdlength = sizeof(srv_data2);
		r->rdata = srv_data2;
		handle->_msg = r->rdata;
		handle->_eom = r->rdata + r->rdlength;
		return 0;
	});

	auto result = resolve_srv("minecraft", "tcp", "example.com");

	EXPECT_EQ(result.size(), 2);

	// Records should be sorted by priority (5 comes before 10)
	auto it = result.begin();
	EXPECT_EQ(it->priority, 5);
	EXPECT_EQ(it->weight, 10);
	EXPECT_EQ(it->port, 25566);
	EXPECT_EQ(it->target, "mc2.example.com");

	++it;
	EXPECT_EQ(it->priority, 10);
	EXPECT_EQ(it->weight, 5);
	EXPECT_EQ(it->port, 25565);
	EXPECT_EQ(it->target, "mc1.example.com");
}

TEST_F(SrvResolverTest, QuerySucceedsWithNonSrvRecordsIgnored) {
	ns_msg handle;

	EXPECT_CALL(mock, res_query(_, _, _, _, _)).WillOnce(Return(512));
	EXPECT_CALL(mock, ns_initparse(_, _, _)).WillOnce([&](const unsigned char* msg, int msglen, ns_msg* h) {
		*h = handle;
		return 0;
	});
	
	handle._counts[ns_s_an] = 2;

	// Mock first record as A record (should be ignored)
	EXPECT_CALL(mock, ns_parserr(_, ns_s_an, 0, _)).WillOnce([&](ns_msg* handle, ns_sect section, int rrnum, ns_rr* r) {
		r->type = ns_t_a;  // A record, not SRV
		return 0;
	});

	// Mock second record as proper SRV record
	EXPECT_CALL(mock, ns_parserr(_, ns_s_an, 1, _)).WillOnce([&](ns_msg* handle, ns_sect section, int rrnum, ns_rr* r) {
		static unsigned char srv_data[] = {0x00, 0x0A,  // Priority: 10
		                                   0x00, 0x05,  // Weight: 5
		                                   0x63, 0xDD,  // Port: 25565
		                                   0x02, 'm',  'c', 0x07, 'e', 'x', 'a', 'm',
		                                   'p',  'l',  'e', 0x03, 'c', 'o', 'm', 0x00};
		r->type = ns_t_srv;
		r->rdlength = sizeof(srv_data);
		r->rdata = srv_data;
		handle->_msg = r->rdata;
		handle->_eom = r->rdata + r->rdlength;
		return 0;
	});

	auto result = resolve_srv("minecraft", "tcp", "example.com");

	// Should only contain the SRV record, A record should be ignored
	EXPECT_EQ(result.size(), 1);
	const auto& record = *result.begin();
	EXPECT_EQ(record.priority, 10);
	EXPECT_EQ(record.weight, 5);
	EXPECT_EQ(record.port, 25565);
	EXPECT_EQ(record.target, "mc.example.com");
}

TEST_F(SrvResolverTest, QuerySucceedsWithMalformedSrvRecordIgnored) {
	ns_msg handle;

	EXPECT_CALL(mock, res_query(_, _, _, _, _)).WillOnce(Return(512));
	EXPECT_CALL(mock, ns_initparse(_, _, _)).WillOnce([&](const unsigned char* msg, int msglen, ns_msg* h) {
		*h = handle;
		return 0;
	});

	// Mock SRV record with insufficient data length (should be ignored)
	handle._counts[ns_s_an] = 1;
	EXPECT_CALL(mock, ns_parserr(_, ns_s_an, 0, _)).WillOnce([&](ns_msg* handle, ns_sect section, int rrnum, ns_rr* r) {
		static unsigned char srv_data[] = {0x00, 0x0A, 0x00, 0x05};  // Only 4 bytes, need at least 6
		r->type = ns_t_srv;
		r->rdlength = sizeof(srv_data);
		r->rdata = srv_data;
		handle->_msg = r->rdata;
		handle->_eom = r->rdata + r->rdlength;
		return 0;
	});

	auto result = resolve_srv("minecraft", "tcp", "example.com");

	// Should be empty since the malformed record was ignored
	EXPECT_TRUE(result.empty());
}

TEST_F(SrvResolverTest, QueryWithDifferentServiceAndProtocol) {
	EXPECT_CALL(mock, res_query(_, _, _, _, _))
	    .WillOnce([](const char* dname, int class_, int type, unsigned char* answer, int anslen) {
		    // Verify the query name is correctly formatted
		    std::string query_name(dname);
		    EXPECT_EQ(query_name, "_http._tcp.example.org");
		    EXPECT_EQ(class_, ns_c_in);
		    EXPECT_EQ(type, ns_t_srv);
		    return -1;  // Fail the query to avoid complex mocking
	    });

	EXPECT_THROW({ resolve_srv("http", "tcp", "example.org"); }, std::runtime_error);
}

TEST_F(SrvResolverTest, QueryWithUDPProtocol) {
	EXPECT_CALL(mock, res_query(_, _, _, _, _))
	    .WillOnce([](const char* dname, int class_, int type, unsigned char* answer, int anslen) {
		    // Verify the query name is correctly formatted for UDP
		    std::string query_name(dname);
		    EXPECT_EQ(query_name, "_sip._udp.voip.example.net");
		    return -1;  // Fail the query to avoid complex mocking
	    });

	EXPECT_THROW({ resolve_srv("sip", "udp", "voip.example.net"); }, std::runtime_error);
}

// =====================================================================================================================
constexpr int base_num_trials = 10'000;

class PickRecordTest : public ::testing::Test {
protected:
	ResolverFuncs mock;
	void SetUp() override {
		rng.seed(42);
	}
};

TEST_F(PickRecordTest, EmptyRecordsThrowsException) {
	records_t empty_records;

	EXPECT_THROW(pick_record(empty_records), std::runtime_error);
}

TEST_F(PickRecordTest, SingleRecordIsSelected) {
	records_t records;
	records.emplace(10, 5, 80, "server1.example.com");

	const SrvRecord& result = pick_record(records);

	EXPECT_EQ(result.priority, 10);
	EXPECT_EQ(result.weight, 5);
	EXPECT_EQ(result.port, 80);
	EXPECT_EQ(result.target, "server1.example.com");
}

TEST_F(PickRecordTest, LowestPriorityIsSelected) {
	records_t records;
	records.emplace(20, 5, 80, "server1.example.com");
	records.emplace(10, 3, 443, "server2.example.com");
	records.emplace(15, 7, 8080, "server3.example.com");

	const SrvRecord& result = pick_record(records);

	// Should select the record with priority 10 (lowest)
	EXPECT_EQ(result.priority, 10);
	EXPECT_EQ(result.target, "server2.example.com");
}

TEST_F(PickRecordTest, ZeroWeightRecordsWithSamePriority) {
	records_t records;
	records.emplace(10, 0, 80, "server1.example.com");
	records.emplace(10, 0, 443, "server2.example.com");
	records.emplace(10, 0, 8080, "server3.example.com");

	const SrvRecord& result = pick_record(records);

	// When all weights are 0, should return the first record (nondeterministic, so we're not testing that)
	EXPECT_EQ(result.priority, 10);
	EXPECT_EQ(result.weight, 0);
}

TEST_F(PickRecordTest, WeightedSelectionWithSamePriority) {
	records_t records;
	records.emplace(10, 100, 80, "server1.example.com");  // High weight
	records.emplace(10, 1, 443, "server2.example.com");   // Low weight
	records.emplace(10, 1, 8080, "server3.example.com");  // Low weight

	// Run the selection multiple times to verify weighted selection
	std::map<std::string, int> selection_count;
	const int num_trials = base_num_trials;

	for (int i = 0; i < num_trials; i++) {
		const SrvRecord& result = pick_record(records);
		selection_count[result.target]++;
	}

	// server1 should be selected much more frequently due to higher weight
	// With weights 100:1:1, server1 should be selected ~98% of the time
	EXPECT_GT(selection_count["server1.example.com"], num_trials * 0.9);
	EXPECT_LT(selection_count["server2.example.com"], num_trials * 0.1);
	EXPECT_LT(selection_count["server3.example.com"], num_trials * 0.1);
}

TEST_F(PickRecordTest, MixedPrioritiesOnlyLowestSelected) {
	records_t records;
	records.emplace(5, 50, 80, "priority5.example.com");
	records.emplace(5, 50, 443, "priority5-2.example.com");
	records.emplace(10, 100, 8080, "priority10.example.com");  // Higher priority, higher weight
	records.emplace(15, 200, 9090, "priority15.example.com");  // Even higher priority and weight

	// Run selection multiple times
	std::map<int, int> priority_count;
	const int num_trials = base_num_trials;

	for (int i = 0; i < num_trials; i++) {
		const SrvRecord& result = pick_record(records);
		priority_count[result.priority]++;
	}

	// Only records with priority 5 should ever be selected
	EXPECT_EQ(priority_count[5], num_trials);
	EXPECT_EQ(priority_count[10], 0);
	EXPECT_EQ(priority_count[15], 0);
}

TEST_F(PickRecordTest, EqualWeightsWithSamePriority) {
	records_t records;
	records.emplace(10, 10, 80, "server1.example.com");
	records.emplace(10, 10, 443, "server2.example.com");
	records.emplace(10, 10, 8080, "server3.example.com");

	// With equal weights, each should be selected roughly equally
	std::map<std::string, int> selection_count;
	const int num_trials = 3 * base_num_trials;  // Use more trials for better statistics

	for (int i = 0; i < num_trials; i++) {
		const SrvRecord& result = pick_record(records);
		selection_count[result.target]++;
	}

	// Each server should be selected roughly 1/3 of the time (within 20% tolerance)
	for (const auto& [server, count] : selection_count) {
		EXPECT_GT(count, num_trials * 0.8 / 3);  // At least 26.7%
		EXPECT_LT(count, num_trials * 1.2 / 3);  // At most 40%
	}
}

TEST_F(PickRecordTest, SingleZeroWeightRecord) {
	records_t records;
	records.emplace(10, 0, 80, "server.example.com");

	const SrvRecord& result = pick_record(records);

	EXPECT_EQ(result.priority, 10);
	EXPECT_EQ(result.weight, 0);
	EXPECT_EQ(result.port, 80);
	EXPECT_EQ(result.target, "server.example.com");
}

TEST_F(PickRecordTest, MaxWeightValues) {
	records_t records;
	records.emplace(1, std::numeric_limits<uint16_t>::max(), 80, "server1.example.com");
	records.emplace(1, 1, 443, "server2.example.com");

	// The server with max weight should be selected almost always
	std::map<std::string, int> selection_count;
	const int num_trials = base_num_trials;

	for (int i = 0; i < num_trials; i++) {
		const SrvRecord& result = pick_record(records);
		selection_count[result.target]++;
	}

	// server1 with max weight should be selected almost every time
	EXPECT_GT(selection_count["server1.example.com"], num_trials * 0.99);
}

TEST_F(PickRecordTest, RecordOrderingByPriority) {
	// Test that records_t(std::multiset) properly orders records by priority
	records_t records;
	records.emplace(30, 1, 80, "server30.example.com");
	records.emplace(10, 1, 443, "server10.example.com");
	records.emplace(20, 1, 8080, "server20.example.com");

	// The first element in the set should be the one with the lowest priority
	EXPECT_EQ(records.begin()->priority, 10);

	const SrvRecord& result = pick_record(records);
	EXPECT_EQ(result.priority, 10);
	EXPECT_EQ(result.target, "server10.example.com");
}
