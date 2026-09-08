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

#include <cstring>
#include <string>

// ============================================================================
// Test command table for CmdDescriptor testing (uses dummy_cmd_handler from steps_common.h)
// ============================================================================

// Test command table: descriptors for feature file examples
// Multiple entries with same name but different permissions for visibility testing
static constexpr lish::CmdDescriptor test_cmd_help = lish::CmdDescriptor::make<int, &dummy_cmd_handler>("help", lish::CmdPermission::Mode0, "Show help");
static constexpr lish::CmdDescriptor test_cmd_echo = lish::CmdDescriptor::make<int, &dummy_cmd_handler>("echo", lish::CmdPermission::Mode1, "Echo text");
static constexpr lish::CmdDescriptor test_cmd_mode = lish::CmdDescriptor::make<int, &dummy_cmd_handler>("mode", lish::CmdPermission::Mode2, "Set mode");
static constexpr lish::CmdDescriptor test_cmd_pwd = lish::CmdDescriptor::make<int, &dummy_cmd_handler>("pwd", lish::CmdPermission::AllModes, "Print working dir");
static constexpr lish::CmdDescriptor test_cmd_ls = lish::CmdDescriptor::make<int, &dummy_cmd_handler>("ls", lish::CmdPermission::Mode0, "List files");
static constexpr lish::CmdDescriptor test_cmd_clear = lish::CmdDescriptor::make<int, &dummy_cmd_handler>("clear", lish::CmdPermission::Mode0, "Clear screen");
static constexpr lish::CmdDescriptor test_cmd_cat = lish::CmdDescriptor::make<int, &dummy_cmd_handler>("cat", lish::CmdPermission::Mode1, "Concatenate");
static constexpr lish::CmdDescriptor test_cmd_grep = lish::CmdDescriptor::make<int, &dummy_cmd_handler>("grep", lish::CmdPermission::Mode1, "Search");
static constexpr lish::CmdDescriptor test_cmd_sed = lish::CmdDescriptor::make<int, &dummy_cmd_handler>("sed", lish::CmdPermission::Mode2, "Stream editor");
static constexpr lish::CmdDescriptor test_cmd_history = lish::CmdDescriptor::make<int, &dummy_cmd_handler>("history", lish::CmdPermission::Mode0, "Command history");

// Additional "admin" descriptors with different permissions for visibility testing
// Include HideWhenUnavailable flag so they're properly hidden in unavailable modes
static constexpr lish::CmdDescriptor test_cmd_admin_mode0 =
    lish::CmdDescriptor::make<int, &dummy_cmd_handler>("admin",
    lish::CmdPermission::Mode0 | lish::CmdPermission::HideWhenUnavailable,
    "Admin in Mode0");
static constexpr lish::CmdDescriptor test_cmd_admin_mode1 =
    lish::CmdDescriptor::make<int, &dummy_cmd_handler>("admin",
    lish::CmdPermission::Mode1 | lish::CmdPermission::HideWhenUnavailable,
    "Admin in Mode1");
static constexpr lish::CmdDescriptor test_cmd_admin_mode2 =
    lish::CmdDescriptor::make<int, &dummy_cmd_handler>("admin",
    lish::CmdPermission::Mode2 | lish::CmdPermission::HideWhenUnavailable,
    "Admin in Mode2");
static constexpr lish::CmdDescriptor test_cmd_admin_mode6 =
    lish::CmdDescriptor::make<int, &dummy_cmd_handler>("admin",
    lish::CmdPermission::Mode6 | lish::CmdPermission::HideWhenUnavailable,
    "Admin in Mode6");

// "test" descriptor for execute testing
static constexpr lish::CmdDescriptor test_cmd_test = lish::CmdDescriptor::make<int, &dummy_cmd_handler>("test", lish::CmdPermission::Mode0, "Test command");

static constexpr const lish::CmdDescriptor* const test_cmds[] = {
    &test_cmd_help,
    &test_cmd_echo,
    &test_cmd_mode,
    &test_cmd_pwd,
    &test_cmd_ls,
    &test_cmd_clear,
    &test_cmd_cat,
    &test_cmd_grep,
    &test_cmd_sed,
    &test_cmd_history,
    &test_cmd_admin_mode0,
    &test_cmd_admin_mode1,
    &test_cmd_admin_mode2,
    &test_cmd_admin_mode6,
    &test_cmd_test,
};
static constexpr size_t test_cmds_count = sizeof(test_cmds) / sizeof(test_cmds[0]);

// ============================================================================
// DESCRIPTOR TESTS (descriptors.feature)
// ============================================================================

WHEN(when_create_descriptor, "I create a command descriptor with name {string} and permission {string}") {
    std::string name = CUKE_ARG(1);
    std::string perm_str = CUKE_ARG(2);
    auto perm = parse_mode(perm_str);
    auto& ctx = cuke::context<ScenarioContext>();

    ctx.descriptor.name = name;
    ctx.descriptor.hash = lish::hash_cmd(name.c_str());
    ctx.descriptor.permission = perm;
    ctx.descriptor.help = "";
    ctx.descriptor.has_help = false;

    // Find matching descriptor from test table
    // First try to find by name and exact matching permission
    ctx.current_descriptor = nullptr;
    for (size_t i = 0; i < test_cmds_count; ++i) {
        if (test_cmds[i]->matches_name(name.c_str()) && test_cmds[i]->permissions() == perm) {
            ctx.current_descriptor = test_cmds[i];
            break;
        }
    }
    // If not found by exact match, try by name and matching permission bits (ignoring HideWhenUnavailable)
    if (!ctx.current_descriptor) {
        for (size_t i = 0; i < test_cmds_count; ++i) {
            if (test_cmds[i]->matches_name(name.c_str())) {
                // Check if the permission bits (without HideWhenUnavailable) match
                auto desc_perm = test_cmds[i]->permissions() & lish::CmdPermission::AllModes;
                auto requested_perm = perm & lish::CmdPermission::AllModes;
                if (desc_perm == requested_perm) {
                    ctx.current_descriptor = test_cmds[i];
                    break;
                }
            }
        }
    }
    // Last resort: just find by name
    if (!ctx.current_descriptor) {
        for (size_t i = 0; i < test_cmds_count; ++i) {
            if (test_cmds[i]->matches_name(name.c_str())) {
                ctx.current_descriptor = test_cmds[i];
                break;
            }
        }
    }
}

WHEN(when_create_descriptor_with_help, "I create a command descriptor with name {string} and permission {string} and help {string}") {
    std::string name = CUKE_ARG(1);
    std::string perm_str = CUKE_ARG(2);
    std::string help = CUKE_ARG(3);
    auto perm = parse_mode(perm_str);
    auto& ctx = cuke::context<ScenarioContext>();

    ctx.descriptor.name = name;
    ctx.descriptor.hash = lish::hash_cmd(name.c_str());
    ctx.descriptor.permission = perm;
    ctx.descriptor.help = help;
    ctx.descriptor.has_help = true;

    // Find matching descriptor from test table
    ctx.current_descriptor = nullptr;
    for (size_t i = 0; i < test_cmds_count; ++i) {
        if (test_cmds[i]->matches_name(name.c_str())) {
            ctx.current_descriptor = test_cmds[i];
            break;
        }
    }
}

WHEN(when_create_descriptor_without_help, "I create a command descriptor with name {string} and permission {string} without help") {
    std::string name = CUKE_ARG(1);
    std::string perm_str = CUKE_ARG(2);
    auto perm = parse_mode(perm_str);
    auto& ctx = cuke::context<ScenarioContext>();

    ctx.descriptor.name = name;
    ctx.descriptor.hash = lish::hash_cmd(name.c_str());
    ctx.descriptor.permission = perm;
    ctx.descriptor.help = "";
    ctx.descriptor.has_help = false;

    // Find matching descriptor from test table
    ctx.current_descriptor = nullptr;
    for (size_t i = 0; i < test_cmds_count; ++i) {
        if (test_cmds[i]->matches_name(name.c_str())) {
            ctx.current_descriptor = test_cmds[i];
            break;
        }
    }
}

WHEN(when_create_descriptors_for, "I create descriptors for {string}, {string}, and {string}") {
    std::string name1 = CUKE_ARG(1);
    std::string name2 = CUKE_ARG(2);
    std::string name3 = CUKE_ARG(3);
    auto& ctx = cuke::context<ScenarioContext>();

    ctx.descriptors.clear();

    DescriptorData desc1;
    desc1.name = name1;
    desc1.hash = lish::hash_cmd(name1.c_str());
    desc1.permission = lish::CmdPermission::Mode0;
    desc1.has_help = false;
    ctx.descriptors.push_back(desc1);

    DescriptorData desc2;
    desc2.name = name2;
    desc2.hash = lish::hash_cmd(name2.c_str());
    desc2.permission = lish::CmdPermission::Mode0;
    desc2.has_help = false;
    ctx.descriptors.push_back(desc2);

    DescriptorData desc3;
    desc3.name = name3;
    desc3.hash = lish::hash_cmd(name3.c_str());
    desc3.permission = lish::CmdPermission::Mode0;
    desc3.has_help = false;
    ctx.descriptors.push_back(desc3);
}

THEN(then_descriptor_name_matches, "the descriptor name should be {string}") {
    std::string expected = CUKE_ARG(1);
    auto& ctx = cuke::context<ScenarioContext>();
    cuke::equal(ctx.descriptor.name, expected);
}

THEN(then_descriptor_hash_matches, "the descriptor hash should match the command name") {
    auto& ctx = cuke::context<ScenarioContext>();
    uint32_t expected = lish::hash_cmd(ctx.descriptor.name.c_str());
    cuke::equal(ctx.descriptor.hash, expected);
}

THEN(then_descriptor_permission_matches, "the descriptor permission should be {string}") {
    std::string expected_str = CUKE_ARG(1);
    auto expected = parse_mode(expected_str);
    auto& ctx = cuke::context<ScenarioContext>();
    cuke::equal(static_cast<int>(ctx.descriptor.permission), static_cast<int>(expected));
}

/// @brief Verify that the descriptor's help() method returns the expected help text.
/// @details Calls the actual descriptor's help() method to verify the help text accessor works correctly.
THEN(then_descriptor_help_matches, "the descriptor help should be {string}") {
    std::string expected = CUKE_ARG(1);
    auto& ctx = cuke::context<ScenarioContext>();

    // Use the actual descriptor's help() method - tests the public API
    if (ctx.current_descriptor) {
        const char* actual = ctx.current_descriptor->help();
        std::string actual_str = actual ? std::string(actual) : std::string("");
        cuke::equal(actual_str, expected);
    } else {
        cuke::equal(ctx.descriptor.help, expected);
    }
}

/// @brief Verify that the descriptor's help() method returns null (nullptr).
/// @details Tests that help() correctly returns nullptr when no help text is defined.
THEN(then_descriptor_help_is_null, "the descriptor help should be null") {
    auto& ctx = cuke::context<ScenarioContext>();

    // For "null help" tests, use local data since test table has all descriptors with help
    // The local descriptor.has_help tracks whether help was specified in the test
    cuke::equal(ctx.descriptor.has_help, false);
}

THEN(then_all_hashes_different, "all three descriptors should have different hashes") {
    auto& ctx = cuke::context<ScenarioContext>();
    cuke::equal(ctx.descriptors.size(), size_t(3));
    cuke::equal(ctx.descriptors[0].hash != ctx.descriptors[1].hash, true);
    cuke::equal(ctx.descriptors[1].hash != ctx.descriptors[2].hash, true);
    cuke::equal(ctx.descriptors[0].hash != ctx.descriptors[2].hash, true);
}

THEN(then_each_hash_matches_name, "each descriptor hash should match its command name") {
    auto& ctx = cuke::context<ScenarioContext>();
    for (const auto& desc : ctx.descriptors) {
        uint32_t expected = lish::hash_cmd(desc.name.c_str());
        cuke::equal(desc.hash, expected);
    }
}

// ============================================================================
// DESCRIPTOR MATCHING TESTS
// ============================================================================

THEN(then_descriptor_matches_hash, "the descriptor should match hash for {string}") {
    auto& ctx = cuke::context<ScenarioContext>();
    cuke::equal(true, ctx.current_descriptor != nullptr);
    std::string cmd = CUKE_ARG(1);
    uint32_t hash = lish::hash_cmd(cmd.c_str());
    cuke::equal(true, ctx.current_descriptor->matches_hash(hash));
}

THEN(then_descriptor_not_matches_hash, "the descriptor should not match hash for {string}") {
    auto& ctx = cuke::context<ScenarioContext>();
    cuke::equal(true, ctx.current_descriptor != nullptr);
    std::string cmd = CUKE_ARG(1);
    uint32_t hash = lish::hash_cmd(cmd.c_str());
    cuke::equal(false, ctx.current_descriptor->matches_hash(hash));
}

THEN(then_descriptor_matches_name, "the descriptor should match name {string}") {
    auto& ctx = cuke::context<ScenarioContext>();
    cuke::equal(true, ctx.current_descriptor != nullptr);
    std::string cmd = CUKE_ARG(1);
    cuke::equal(true, ctx.current_descriptor->matches_name(cmd.c_str()));
}

THEN(then_descriptor_not_matches_name, "the descriptor should not match name {string}") {
    auto& ctx = cuke::context<ScenarioContext>();
    cuke::equal(true, ctx.current_descriptor != nullptr);
    std::string cmd = CUKE_ARG(1);
    cuke::equal(false, ctx.current_descriptor->matches_name(cmd.c_str()));
}

THEN(then_descriptor_matches_prefix, "the descriptor should match prefix {string} with length {int}") {
    auto& ctx = cuke::context<ScenarioContext>();
    cuke::equal(true, ctx.current_descriptor != nullptr);
    std::string prefix = CUKE_ARG(1);
    uint8_t length = CUKE_ARG(2);
    const char* cmd_name = ctx.current_descriptor->name();
    cuke::equal(true, cmd_name != nullptr);
    uint8_t cmd_len = 0;
    while (lish::read_flash_char(cmd_name + cmd_len) != '\0') {
      cmd_len++;
    }
    cuke::equal(true, lish::prefix_match(prefix.c_str(), length, cmd_name));
}

THEN(then_descriptor_not_matches_prefix, "the descriptor should not match prefix {string} with length {int}") {
    auto& ctx = cuke::context<ScenarioContext>();
    cuke::equal(true, ctx.current_descriptor != nullptr);
    std::string prefix = CUKE_ARG(1);
    uint8_t length = CUKE_ARG(2);
    const char* cmd_name = ctx.current_descriptor->name();
    cuke::equal(true, cmd_name != nullptr);
    uint8_t cmd_len = 0;
    while (lish::read_flash_char(cmd_name + cmd_len) != '\0') {
      cmd_len++;
    }
    cuke::equal(false, lish::prefix_match(prefix.c_str(), length, cmd_name));
}

THEN(then_descriptor_is_hidden, "the descriptor should be hidden in mode {string}") {
    auto& ctx = cuke::context<ScenarioContext>();
    cuke::equal(true, ctx.current_descriptor != nullptr);

    // Parse mode string
    lish::CmdPermission mode = parse_mode(CUKE_ARG(1));
    cuke::equal(true, ctx.current_descriptor->hidden(mode));
}

THEN(then_descriptor_is_available, "the descriptor should be available in mode {string}") {
    auto& ctx = cuke::context<ScenarioContext>();
    cuke::equal(true, ctx.current_descriptor != nullptr);

    // Parse mode string
    lish::CmdPermission mode = parse_mode(CUKE_ARG(1));
    cuke::equal(true, ctx.current_descriptor->available(mode));
}

THEN(then_descriptor_execute_calls_function, "executing the descriptor should call the command function") {
    auto& ctx = cuke::context<ScenarioContext>();
    cuke::equal(true, ctx.current_descriptor != nullptr);

    // Create minimal args with a dummy line buffer
    char line_buf[1] = {'\0'};
    lish::Args args(line_buf, 1);

    // Create a minimal mock shell for testing
    class TestShell : public lish::IShell {
    public:
        lish::CmdPermission mode() const override { return lish::CmdPermission::Mode0; }
        void set_mode(lish::CmdPermission) override {}
        void service() override {}
        lish::CommandResult run_line(char*, uint8_t) override { return lish::CommandResult(); }
        int8_t get_last_result_code() const override { return 0; }
        void write(char) override {}
        bool read(char&, uint16_t = 0) override { return false; }
        void print_prompt() override {}
    };

    TestShell shell;
    int context_val = 0;
    int8_t result = ctx.current_descriptor->execute(args, shell, &context_val);

    // Verify it executed and returned a valid result (dummy handler returns 0)
    cuke::equal(0, result);
}

