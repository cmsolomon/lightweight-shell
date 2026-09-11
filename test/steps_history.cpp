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
// HISTORY TESTS (history.feature)
// ============================================================================
/// Tests for command history storage, navigation, and retrieval.

struct HistoryContext {
    static constexpr uint8_t MaxLineLength = 128;
    static constexpr uint8_t HistoryLength = 16;

    char line_buffer_[MaxLineLength];
    lish::History<HistoryLength, MaxLineLength> history_;

    HistoryContext() {
        std::memset(line_buffer_, 0, MaxLineLength);
    }

    void fill_line_buffer(const std::string& cmd) {
        size_t len = cmd.length();
        if (len >= MaxLineLength) {
            len = MaxLineLength - 1;
        }
        std::memcpy(line_buffer_, cmd.c_str(), len);
        line_buffer_[len] = '\0';
    }

};

// Context for overflow testing with small buffer
template <uint8_t HistLen, uint8_t TypLen>
struct SmallHistoryContext {
    static constexpr uint8_t MaxLineLength = 128;
    static constexpr uint8_t HistoryLength = HistLen;
    static constexpr uint8_t TypicalLineLength = TypLen;

    char line_buffer_[MaxLineLength];
    lish::History<HistoryLength, TypicalLineLength> history_;

    SmallHistoryContext() {
        std::memset(line_buffer_, 0, MaxLineLength);
    }

    void fill_line_buffer(const std::string& cmd) {
        size_t len = cmd.length();
        if (len >= MaxLineLength) {
            len = MaxLineLength - 1;
        }
        std::memcpy(line_buffer_, cmd.c_str(), len);
        line_buffer_[len] = '\0';
    }
};

// Use History<5, 10> for buffer overflow tests (5 entries * 10 bytes = 50 byte buffer)
using OverflowHistoryContext = SmallHistoryContext<5, 10>;

GIVEN(given_history_empty, "history is empty") {
    auto& ctx = cuke::context<HistoryContext>();
    auto& scenario_ctx = cuke::context<ScenarioContext>();
    scenario_ctx.use_overflow_history = false;
}

WHEN(when_press_up_arrow, "I press up arrow") {
    auto& scenario_ctx = cuke::context<ScenarioContext>();
    if (scenario_ctx.use_overflow_history) {
        auto& ctx = cuke::context<OverflowHistoryContext>();
        ctx.history_.prev();
    } else {
        auto& ctx = cuke::context<HistoryContext>();
        ctx.history_.prev();
    }
}

WHEN(when_press_down_arrow, "I press down arrow") {
    auto& scenario_ctx = cuke::context<ScenarioContext>();
    if (scenario_ctx.use_overflow_history) {
        auto& ctx = cuke::context<OverflowHistoryContext>();
        ctx.history_.next();
    } else {
        auto& ctx = cuke::context<HistoryContext>();
        ctx.history_.next();
    }
}

WHEN(when_press_up_arrow_times, "I press up arrow {int} times") {
    auto& scenario_ctx = cuke::context<ScenarioContext>();
    int count = CUKE_ARG(1);
    if (scenario_ctx.use_overflow_history) {
        auto& ctx = cuke::context<OverflowHistoryContext>();
        for (int i = 0; i < count; ++i) {
            ctx.history_.prev();
        }
    } else {
        auto& ctx = cuke::context<HistoryContext>();
        for (int i = 0; i < count; ++i) {
            ctx.history_.prev();
        }
    }
}

WHEN(when_press_down_arrow_times, "I press down arrow {int} times") {
    auto& scenario_ctx = cuke::context<ScenarioContext>();
    int count = CUKE_ARG(1);
    if (scenario_ctx.use_overflow_history) {
        auto& ctx = cuke::context<OverflowHistoryContext>();
        for (int i = 0; i < count; ++i) {
            ctx.history_.next();
        }
    } else {
        auto& ctx = cuke::context<HistoryContext>();
        for (int i = 0; i < count; ++i) {
            ctx.history_.next();
        }
    }
}

WHEN(when_cancel_browsing, "I cancel browsing") {
    auto& scenario_ctx = cuke::context<ScenarioContext>();
    if (scenario_ctx.use_overflow_history) {
        auto& ctx = cuke::context<OverflowHistoryContext>();
        ctx.history_.cancel_browsing();
    } else {
        auto& ctx = cuke::context<HistoryContext>();
        ctx.history_.cancel_browsing();
    }
}

THEN(then_browsing, "I am browsing history") {
    auto& scenario_ctx = cuke::context<ScenarioContext>();
    if (scenario_ctx.use_overflow_history) {
        auto& ctx = cuke::context<OverflowHistoryContext>();
        cuke::equal(ctx.history_.is_browsing(), true);
    } else {
        auto& ctx = cuke::context<HistoryContext>();
        cuke::equal(ctx.history_.is_browsing(), true);
    }
}

THEN(then_not_browsing, "I am not browsing history") {
    auto& scenario_ctx = cuke::context<ScenarioContext>();
    if (scenario_ctx.use_overflow_history) {
        auto& ctx = cuke::context<OverflowHistoryContext>();
        cuke::equal(ctx.history_.is_browsing(), false);
    } else {
        auto& ctx = cuke::context<HistoryContext>();
        cuke::equal(ctx.history_.is_browsing(), false);
    }
}

THEN(then_current_entry, "current entry is {string}") {
    auto& scenario_ctx = cuke::context<ScenarioContext>();
    std::string expected = CUKE_ARG(1);
    if (scenario_ctx.use_overflow_history) {
        auto& ctx = cuke::context<OverflowHistoryContext>();
        cuke::equal(std::string(ctx.history_.current()), expected);
    } else {
        auto& ctx = cuke::context<HistoryContext>();
        cuke::equal(std::string(ctx.history_.current()), expected);
    }
}

THEN(then_current_entry_still, "current entry is still {string}") {
    auto& scenario_ctx = cuke::context<ScenarioContext>();
    std::string expected = CUKE_ARG(1);
    if (scenario_ctx.use_overflow_history) {
        auto& ctx = cuke::context<OverflowHistoryContext>();
        cuke::equal(std::string(ctx.history_.current()), expected);
    } else {
        auto& ctx = cuke::context<HistoryContext>();
        cuke::equal(std::string(ctx.history_.current()), expected);
    }
}

THEN(then_newest_entry, "newest history entry is {string}") {
    auto& scenario_ctx = cuke::context<ScenarioContext>();
    std::string expected = CUKE_ARG(1);
    if (scenario_ctx.use_overflow_history) {
        auto& ctx = cuke::context<OverflowHistoryContext>();
        ctx.history_.cancel_browsing();
        ctx.history_.prev();
        cuke::equal(std::string(ctx.history_.current()), expected);
    } else {
        auto& ctx = cuke::context<HistoryContext>();
        ctx.history_.cancel_browsing();
        ctx.history_.prev();
        cuke::equal(std::string(ctx.history_.current()), expected);
    }
}

GIVEN(given_history_empty_with_size, "history is empty with buffer size {int} and typical line {int}") {
    auto& ctx = cuke::context<OverflowHistoryContext>();
    auto& scenario_ctx = cuke::context<ScenarioContext>();
    scenario_ctx.use_overflow_history = true;
}

WHEN(when_push_to_overflow_history, "I push {string} to history") {
    auto& scenario_ctx = cuke::context<ScenarioContext>();
    std::string cmd = CUKE_ARG(1);

    if (scenario_ctx.use_overflow_history) {
        auto& ctx = cuke::context<OverflowHistoryContext>();
        char temp_array[128];
        std::memset(temp_array, 0, 128);
        std::strncpy(temp_array, cmd.c_str(), 127);
        temp_array[127] = '\0';

        lish::LineBuffer<128> temp_buf;
        temp_buf.copy_from(temp_array);
        ctx.history_.push(temp_buf);
    } else {
        auto& ctx = cuke::context<HistoryContext>();
        char temp_array[HistoryContext::MaxLineLength];
        std::memset(temp_array, 0, HistoryContext::MaxLineLength);
        std::strncpy(temp_array, cmd.c_str(), HistoryContext::MaxLineLength - 1);
        temp_array[HistoryContext::MaxLineLength - 1] = '\0';

        lish::LineBuffer<HistoryContext::MaxLineLength> temp_buf;
        temp_buf.copy_from(temp_array);
        ctx.history_.push(temp_buf);
    }
}

THEN(then_history_entry_count, "history entry count is {int}") {
    auto& scenario_ctx = cuke::context<ScenarioContext>();
    int expected = CUKE_ARG(1);
    int count = 0;
    if (scenario_ctx.use_overflow_history) {
        auto& ctx = cuke::context<OverflowHistoryContext>();
        ctx.history_.cancel_browsing();
        while (ctx.history_.prev()) {
            count++;
        }
        ctx.history_.cancel_browsing();
    } else {
        auto& ctx = cuke::context<HistoryContext>();
        ctx.history_.cancel_browsing();
        while (ctx.history_.prev()) {
            count++;
        }
        ctx.history_.cancel_browsing();
    }
    cuke::equal(count, expected);
}

THEN(then_cannot_navigate_older, "I cannot navigate to older entries") {
    auto& scenario_ctx = cuke::context<ScenarioContext>();
    if (scenario_ctx.use_overflow_history) {
        auto& ctx = cuke::context<OverflowHistoryContext>();
        std::string current_before = ctx.history_.current();
        ctx.history_.prev();  // Try to go older
        std::string current_after = ctx.history_.current();
        cuke::equal(current_before, current_after);
    } else {
        auto& ctx = cuke::context<HistoryContext>();
        std::string current_before = ctx.history_.current();
        ctx.history_.prev();  // Try to go older
        std::string current_after = ctx.history_.current();
        cuke::equal(current_before, current_after);
    }
}

