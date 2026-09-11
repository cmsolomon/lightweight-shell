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

#ifndef SHARED_TEST_IO_H
#define SHARED_TEST_IO_H

#include <cstdint>
#include <string>
#include <vector>

/// I/O adapter for driving real lish::Shell/InputProcessor instances in tests.
/// Uses STATIC storage because lish::Shell always default-constructs its own
/// IOAdapter internally and never exposes it - there is no way to inject a
/// pre-built adapter instance or read back what an internally-owned instance
/// wrote via the IShell interface (write/read only proxy single characters in
/// each direction). Since only one shell/input-processor runs at a time in
/// this single-threaded test process, static storage lets every instance -
/// including ones constructed internally by Shell - share the same buffers,
/// so test code can queue input and read captured output without ever
/// needing a reference to the owning object's private instance.
///
/// Shared across shell.feature and input_processor.feature (steps_shell.cpp
/// and steps_input_processor.cpp) so their real-type instantiations of
/// Shell/InputProcessor match exactly and their test coverage merges onto the
/// same compiled code. tabcomplete.feature (steps_tabcomplete.cpp) calls
/// lish::tab_complete() directly rather than through a Shell/InputProcessor,
/// so it uses the simpler SimpleIOAdapter (steps_common.h) instead.
struct SharedTestIOAdapter {
    static inline std::vector<char> written;
    static inline std::vector<char> read_queue;
    static inline size_t read_pos = 0;

    void write(const char c) {
        written.push_back(c);
    }

    bool read(char& c, const uint16_t timeout_ms = 0) {
        (void)timeout_ms;
        if (read_pos >= read_queue.size()) {
            return false;
        }
        c = read_queue[read_pos++];
        return true;
    }

    static void reset() {
        written.clear();
        read_queue.clear();
        read_pos = 0;
    }

    static void queue_line(const std::string& line) {
        for (char c : line) {
            read_queue.push_back(c);
        }
        read_queue.push_back('\n');
    }

    static std::string get_written() {
        return std::string(written.begin(), written.end());
    }
};

#endif  // SHARED_TEST_IO_H
