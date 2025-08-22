#pragma once

#include <arpa/nameser.h>
#include <gmock/gmock.h>
#include <resolv.h>

// gMock interface for the functions
struct ResolverFuncs {
	MOCK_METHOD(int, res_query, (const char* dname, int class_, int type, unsigned char* answer, int anslen));
	MOCK_METHOD(int, ns_initparse, (const unsigned char* msg, int msglen, ns_msg* handle));
	MOCK_METHOD(int, ns_parserr, (ns_msg * handle, ns_sect section, int rrnum, ns_rr* rr));
};

// Global test double
static ResolverFuncs* g_mock = nullptr;

// C wrappers that forward to gMock
#pragma clang diagnostic push
#pragma ide diagnostic ignored "NullDereference"
extern "C" int res_query(const char* dname, int class_, int type, unsigned char* answer, int anslen) __THROW {
	return g_mock->res_query(dname, class_, type, answer, anslen);
}
extern "C" int ns_initparse(const unsigned char* msg, int msglen, ns_msg* handle) __THROW {
	return g_mock->ns_initparse(msg, msglen, handle);
}
extern "C" int ns_parserr(ns_msg* handle, ns_sect section, int rrnum, ns_rr* rr) __THROW {
	return g_mock->ns_parserr(handle, section, rrnum, rr);
}
#pragma clang diagnostic pop
