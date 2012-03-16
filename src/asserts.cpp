#include "asserts.hpp"

namespace {
	bool throw_validation_failure = false;
}

bool throw_validation_failure_on_assert()
{
	return throw_validation_failure;
}

assert_recover_scope::assert_recover_scope() : value(throw_validation_failure)
{
	throw_validation_failure = true;
}

assert_recover_scope::~assert_recover_scope()
{
	throw_validation_failure = value;
}
