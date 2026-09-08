///
/// Copyright 2026 Chris Solomon
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///

#include "cucumber.hpp"
#include "lish.h"
#include "steps_common.h"

#include <string>
#include <vector>

// ============================================================================
// PERMISSION TESTS (permissions.feature)
// ============================================================================
GIVEN(given_permissions_set, "permissions are set to {string}") {
    std::string perm_str = CUKE_ARG(1);
    auto& ctx = cuke::context<ScenarioContext>();
    ctx.permission = parse_mode(perm_str);
    ctx.has_hide_flag = false;
}

GIVEN(given_permission_set, "permission is set to {string}") {
    std::string perm_str = CUKE_ARG(1);
    auto& ctx = cuke::context<ScenarioContext>();
    ctx.permission = parse_mode(perm_str);
    ctx.has_hide_flag = false;
}

GIVEN(given_permission_with_hide_flag, "permission is set to {string} with HideWhenUnavailable flag") {
    std::string perm_str = CUKE_ARG(1);
    auto& ctx = cuke::context<ScenarioContext>();
    ctx.permission = parse_mode(perm_str) | lish::CmdPermission::HideWhenUnavailable;
    ctx.has_hide_flag = true;
}

GIVEN(given_perm_a, "permission A is set to {string}") {
    std::string perm_str = CUKE_ARG(1);
    cuke::context<ScenarioContext>().perm_a = parse_mode(perm_str);
}

GIVEN(given_perm_b, "permission B is set to {string}") {
    std::string perm_str = CUKE_ARG(1);
    cuke::context<ScenarioContext>().perm_b = parse_mode(perm_str);
}

WHEN(when_check_permissions, "I check permissions") {
    auto& ctx = cuke::context<ScenarioContext>();
    ctx.available_modes.clear();

    // Check each mode to see if it's available
    for (int i = 0; i < 7; i++) {
        auto mode = static_cast<lish::CmdPermission>(1 << i);
        if (lish::is_available(ctx.permission, mode)) {
            ctx.available_modes.push_back("Mode" + std::to_string(i));
        }
    }
}

WHEN(when_check_in_mode, "I check in mode {string}") {
    auto& ctx = cuke::context<ScenarioContext>();
    ctx.current_mode = parse_mode(CUKE_ARG(1));
    ctx.visibility_result = !lish::is_hidden(ctx.permission, ctx.current_mode);
}

WHEN(when_compute_a_and_b, "I compute A AND B") {
    auto& ctx = cuke::context<ScenarioContext>();
    ctx.bitwise_result = ctx.perm_a & ctx.perm_b;
    ctx.available_modes.clear();

    for (int i = 0; i < 7; i++) {
        auto mode = static_cast<lish::CmdPermission>(1 << i);
        if (lish::is_available(ctx.bitwise_result, mode)) {
            ctx.available_modes.push_back("Mode" + std::to_string(i));
        }
    }
}

WHEN(when_compute_a_or_b, "I compute A OR B") {
    auto& ctx = cuke::context<ScenarioContext>();
    ctx.bitwise_result = ctx.perm_a | ctx.perm_b;
    ctx.available_modes.clear();

    for (int i = 0; i < 7; i++) {
        auto mode = static_cast<lish::CmdPermission>(1 << i);
        if (lish::is_available(ctx.bitwise_result, mode)) {
            ctx.available_modes.push_back("Mode" + std::to_string(i));
        }
    }
}

WHEN(when_compute_not, "I compute NOT") {
    auto& ctx = cuke::context<ScenarioContext>();
    ctx.bitwise_result = ~ctx.permission;
}

THEN(then_modes_available, "only the following modes should be available: {string}") {
    auto& ctx = cuke::context<ScenarioContext>();
    std::string expected_str = CUKE_ARG(1);
    std::vector<std::string> expected;

    // Parse comma-separated list of modes (trim whitespace)
    if (!expected_str.empty()) {
        size_t start = 0;
        while (start < expected_str.length()) {
            size_t end = expected_str.find(",", start);
            if (end == std::string::npos) end = expected_str.length();

            std::string mode = expected_str.substr(start, end - start);
            mode.erase(0, mode.find_first_not_of(" "));
            mode.erase(mode.find_last_not_of(" ") + 1);

            if (!mode.empty()) {
                expected.push_back(mode);
            }
            start = end + 1;
        }
    }

    cuke::equal(ctx.available_modes.size(), expected.size());
    for (size_t i = 0; i < expected.size() && i < ctx.available_modes.size(); i++) {
        cuke::equal(ctx.available_modes[i], expected[i]);
    }
}

THEN(then_modes_available_literal, "only the following modes should be available: \"\"") {
    auto& ctx = cuke::context<ScenarioContext>();
    cuke::equal(ctx.available_modes.size(), size_t(0));
}

THEN(then_no_modes_available, "no modes should be available") {
    auto& ctx = cuke::context<ScenarioContext>();
    cuke::equal(ctx.available_modes.size(), size_t(0));
}

THEN(then_bits_flipped, "the result should have all bits flipped in the uint8 representation") {
    auto& ctx = cuke::context<ScenarioContext>();
    auto expected = ~ctx.permission;
    cuke::equal(static_cast<int>(ctx.bitwise_result), static_cast<int>(expected));
}

THEN(then_visibility, "visibility should be {string}") {
    std::string expected = CUKE_ARG(1);
    bool expected_visible = (expected == "visible");
    cuke::equal(cuke::context<ScenarioContext>().visibility_result, expected_visible);
}
