#include "Catch.hpp"

#include <utils/ScopeExit.h>
#include <utility>
#include <functional>


SCENARIO("ScopeExit calls its invokable when destroyed", "[ScopeExit]")
{
	GIVEN("A ScopeExit constructed using its factory")
	{
		int numTimesCalled = 0;

		WHEN("it goes out of scope")
		{
			// Introduce a scope so ScopeExit's destructor is called.
			{
				auto se = utils::makeScopeExit([&numTimesCalled] {
					++numTimesCalled;
				});
			}

			THEN("its invokable is called once.")
			{
				REQUIRE(numTimesCalled == 1);
			}
		}
	}
}


SCENARIO("ScopeExit does not call its invokable when reset", "[ScopeExit]")
{
	GIVEN("A ScopeExit constructed using its factory")
	{
		int numTimesCalled = 0;

		WHEN("it goes out of scope after being reset")
		{
			// Introduce a scope so ScopeExit's destructor is called.
			{
				auto se = utils::makeScopeExit([&numTimesCalled] {
					++numTimesCalled;
				});

				se.reset();
			}
			
			THEN("its invokable is not called.")
			{
				REQUIRE(numTimesCalled == 0);
			}
		}
	}
}


SCENARIO("ScopeExit does not call its invokable when moved", "[ScopeExit]")
{

	GIVEN("A ScopeExit constructed with a lambda")
	{
		int numTimesCalled = 0;
		auto se = utils::makeScopeExit([&numTimesCalled] {
			++numTimesCalled;
		});

		WHEN("it hasn't been destroyed yet")
		{
			THEN("its invokable has not yet been called.")
			{
				REQUIRE(numTimesCalled == 0);
			}
		}

		WHEN("it is move assigned to another scope_exit")
		{
			auto movedSe = std::move(se);

			THEN("its invokable has not yet been called.")
			{
				REQUIRE(numTimesCalled == 0);
			}
		}

		WHEN("another ScopeExit is move constructed from it")
		{
			auto movedSe{ std::move(se) };

			THEN("its invokable has not yet been called.")
			{
				REQUIRE(numTimesCalled == 0);
			}
		}
	}
}


SCENARIO("ScopeExit does not call its invokable when its invokable is changed", "[ScopeExit]")
{
	GIVEN("A ScopeExit constructed a with type-erased function")
	{
		int numTimesCalled = 0;
		auto se = utils::ScopeExit<std::function<void()>>([&numTimesCalled] {
			++numTimesCalled;
		});

		WHEN("its function is changed out for another of the same type")
		{
			std::function<void()> countCalls2 = [&numTimesCalled] {
				++numTimesCalled;
			};

			se.set(std::move(countCalls2));

			THEN("neither the previous, nor the new function are called.")
			{
				REQUIRE(numTimesCalled == 0);
			}
		}
	}
}