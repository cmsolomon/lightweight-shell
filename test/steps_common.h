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

#ifndef STEPS_COMMON_H
#define STEPS_COMMON_H

#include "lish.h"

#include <cstdint>
#include <string>
#include <vector>

/// Fixtures and helpers shared by more than one steps_*.cpp file. Everything
/// here is declared `static` (internal linkage) or is a plain struct/type, so
/// each translation unit that includes this header gets its own private copy -
/// safe to include from multiple .cpp files with no ODR concerns, but nothing
/// in this header may register a GIVEN/WHEN/THEN step: doing so from a header
/// included by more than one .cpp would register the same step twice.

/// @brief A no-op command handler used to build throwaway CmdDescriptor
/// instances for tests that only need a valid, callable handler - the
/// descriptor tests (steps_descriptors.cpp) and tab completion tests
/// (steps_tabcomplete.cpp) each build their own descriptor tables against it.
static int8_t dummy_cmd_handler(const lish::Args& args, lish::IShell& shell, int& ctx) {
    lish::unused(args);
    lish::unused(shell);
    lish::unused(ctx);
    return 0;
}

/// @brief Parses a permission-mode expression from a feature file into a CmdPermission.
/// @details Handles single modes (`Mode0`-`Mode6`, `AllModes`, `None`), comma-separated
/// lists (`Mode0,Mode1`), bitwise AND (`AllModes & ~Mode0`), NOT (`~Mode0`), and
/// parenthesized grouping (`~(Mode0,Mode1)`). Used by permissions.feature,
/// descriptors.feature, and shell.feature scenarios that specify a mode as a string.
/// @param perm_str The mode expression, exactly as it appears in the feature file.
/// @return The resulting CmdPermission; an unrecognized leaf token contributes
/// CmdPermission::None rather than failing the scenario outright.
static lish::CmdPermission parse_mode(const std::string& perm_str) {
    std::string trimmed = perm_str;
    trimmed.erase(0, trimmed.find_first_not_of(" "));
    trimmed.erase(trimmed.find_last_not_of(" ") + 1);

    // Base cases
    if (trimmed == "Mode0") {
        return lish::CmdPermission::Mode0;
    }
    if (trimmed == "Mode1") {
        return lish::CmdPermission::Mode1;
    }
    if (trimmed == "Mode2") {
        return lish::CmdPermission::Mode2;
    }
    if (trimmed == "Mode3") {
        return lish::CmdPermission::Mode3;
    }
    if (trimmed == "Mode4") {
        return lish::CmdPermission::Mode4;
    }
    if (trimmed == "Mode5") {
        return lish::CmdPermission::Mode5;
    }
    if (trimmed == "Mode6") {
        return lish::CmdPermission::Mode6;
    }
    if (trimmed == "AllModes") {
        return lish::CmdPermission::AllModes;
    }
    if (trimmed == "None") {
        return lish::CmdPermission::None;
    }

    // Handle parentheses: (Mode0,Mode1) -> parse inner
    if (trimmed[0] == '(' && trimmed[trimmed.length() - 1] == ')') {
        std::string inner = trimmed.substr(1, trimmed.length() - 2);
        return parse_mode(inner);
    }

    // Handle NOT operator: ~... or ~(...)
    if (trimmed[0] == '~') {
        std::string inner = trimmed.substr(1);
        inner.erase(0, inner.find_first_not_of(" "));
        return ~parse_mode(inner);
    }

    // Handle bitwise AND: ... & ...
    if (trimmed.find("&") != std::string::npos) {
        size_t pos = trimmed.find("&");
        std::string left = trimmed.substr(0, pos);
        std::string right = trimmed.substr(pos + 1);
        left.erase(left.find_last_not_of(" ") + 1);
        right.erase(0, right.find_first_not_of(" "));

        auto left_perm = parse_mode(left);
        auto right_perm = parse_mode(right);
        return left_perm & right_perm;
    }

    // Handle comma-separated list: Mode0,Mode1,Mode2
    if (trimmed.find(",") != std::string::npos) {
        lish::CmdPermission result = lish::CmdPermission::None;
        size_t start = 0;
        while (start < trimmed.length()) {
            size_t end = trimmed.find(",", start);
            if (end == std::string::npos) {
                end = trimmed.length();
            }

            std::string mode = trimmed.substr(start, end - start);
            mode.erase(0, mode.find_first_not_of(" "));
            mode.erase(mode.find_last_not_of(" ") + 1);

            result = result | parse_mode(mode);
            start = end + 1;
        }
        return result;
    }

    return lish::CmdPermission::None;
}

/// @brief A single parsed/constructed command descriptor's data, as recorded by
/// descriptor-creation steps for later THEN-step assertions (name/hash/permission/help).
struct DescriptorData {
    std::string name;
    uint32_t hash;
    lish::CmdPermission permission;
    std::string help;
    bool has_help;
};

/// @brief Scenario state shared by defs.feature, permissions.feature, and
/// descriptors.feature steps (steps_defs.cpp, steps_permissions.cpp,
/// steps_descriptors.cpp) - a single struct because cwt-cucumber looks up
/// scenario context by type (`cuke::context<ScenarioContext>()`), and these
/// three feature files' steps evolved together from one shared fixture.
///
/// @par Invariant: use_overflow_history is the one field here actually read by
/// a fourth file, steps_history.cpp - it is not history state itself, only a
/// routing flag. history.feature's GIVEN steps pick between two independently
/// -typed contexts (HistoryContext or OverflowHistoryContext), and every later
/// WHEN/THEN step needs to know which one to look up before it can look it
/// up - a flag stored on either candidate context can't answer that, so it
/// lives here instead, on a type every history step can reach regardless of
/// which one was chosen.
struct ScenarioContext {
    char char_result;
    uint32_t hash_result;
    uint32_t previous_hash_result;
    lish::CmdPermission permission;
    lish::CmdPermission current_mode;
    bool visibility_result;
    DescriptorData descriptor;
    std::vector<DescriptorData> descriptors;
    lish::CmdPermission perm_a;
    lish::CmdPermission perm_b;
    lish::CmdPermission bitwise_result;
    std::vector<std::string> available_modes;
    bool has_hide_flag;

    // For testing actual CmdDescriptor instances: pointer to one from test table
    const lish::CmdDescriptor* current_descriptor = nullptr;

    // Track which history context type is in use for this scenario
    bool use_overflow_history = false;
};

/// @brief Minimal IOAdapter that just accumulates everything written to it.
/// @details Used wherever a test only needs to inspect output, not drive a
/// real Shell/InputProcessor (which need SharedTestIOAdapter instead - see
/// shared_test_io.h) - defs.feature's write-function tests and
/// tabcomplete.feature's tab_complete() tests both use this.
struct SimpleIOAdapter {
    std::string output;

    void write(const char c) {
        output += c;
    }
};

/// @brief Scenario context for defs.feature's I/O adapter and write-function tests.
/// @details Also read (not written) by steps_shell.cpp's merged "the output
/// should contain {string}" step, which checks this before falling back to
/// ShellContext - see the comment on that step for why it's one definition
/// instead of two.
struct WriteContext {
    SimpleIOAdapter io;
};

#endif  // STEPS_COMMON_H
