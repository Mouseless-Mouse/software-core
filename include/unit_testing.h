#pragma once

#include <Arduino.h>
#include <unordered_map>
#include <vector>

#include "virtual_callback.h"

namespace UnitTest {

extern std::unordered_map<std::string, BaseCallback*> testCbRegistry;

/// @brief Register an invocable object as a unit test to be made available via the `test` shell command.
/// @tparam F The underlying type of the invocable object; this should be deduced automatically.
/// @param name The name by which the unit test will be registered (i.e. `test <name>` will run the test)
/// @param invocable The invocable object containing the unit test code
template <typename F>
void add(std::string&& name, F&& invocable) {
    testCbRegistry.emplace(
        std::forward<std::string>(name), new Callback<F>(std::forward<F>(invocable))
    );
}

void run(std::vector<const char*>& args);

};  // namespace UnitTest