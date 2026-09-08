#
# Copyright 2026 Chris Solomon
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

Feature: Command History Management

  Scenario: Empty history - prev does nothing
    Given history is empty
    When I press up arrow
    Then I am not browsing history

  Scenario: Single entry - prev browses it, next exits browsing
    Given history is empty
    And I push "ls" to history
    When I press up arrow
    Then I am browsing history
    And current entry is "ls"
    When I press down arrow
    Then I am not browsing history

  Scenario: Multiple entries - navigate up through oldest to newest
    Given history is empty
    And I push "ls" to history
    And I push "cat file" to history
    And I push "pwd" to history
    When I press up arrow
    Then I am browsing history
    And current entry is "pwd"
    When I press up arrow
    Then current entry is "cat file"
    When I press up arrow
    Then current entry is "ls"
    When I press up arrow
    Then current entry is still "ls"

  Scenario: Navigate down through history back to empty
    Given history is empty
    And I push "ls" to history
    And I push "cat file" to history
    And I push "pwd" to history
    And I press up arrow 3 times
    When I press down arrow
    Then I am browsing history
    And current entry is "cat file"
    When I press down arrow
    Then current entry is "pwd"
    When I press down arrow
    Then I am not browsing history

  Scenario: Typing same command again promotes it to most recent
    Given history is empty
    And I push "ls" to history
    And I push "cat file" to history
    And I push "pwd" to history
    When I push "ls" to history
    Then I am not browsing history
    And newest history entry is "ls"

  Scenario: Pushing new command adds entry
    Given history is empty
    And I push "ls" to history
    And I push "cat" to history
    When I push "cat file" to history
    Then I am not browsing history

  Scenario: Pushing new command after several entries
    Given history is empty
    And I push "cmd1" to history
    And I push "cmd2" to history
    And I push "cmd3" to history
    And I push "cmd4" to history
    When I push "cmd5" to history
    Then I am not browsing history

  Scenario: Typing same command with different case preserves most recent case
    Given history is empty
    And I push "ls" to history
    And I push "cat" to history
    When I push "LS" to history
    Then I am not browsing history
    And newest history entry is "LS"

  Scenario: Cancel browsing exits browsing mode
    Given history is empty
    And I push "ls" to history
    And I push "cat" to history
    And I push "pwd" to history
    When I press up arrow
    Then I am browsing history
    When I cancel browsing
    Then I am not browsing history

  Scenario: Navigation boundaries (single entry)
    Given history is empty
    And I push "ls" to history
    When I press up arrow 1 times
    Then I am browsing history
    And current entry is "ls"

  Scenario: Large entries exceed buffer capacity - evict oldest
    Given history is empty with buffer size 50 and typical line 10
    And I push "12345678901234567890123456789012" to history
    And I push "abcdefghijklmnopqrstuvwxyzabcdef" to history
    When I press up arrow
    Then current entry is "abcdefghijklmnopqrstuvwxyzabcdef"
    When I press up arrow
    Then current entry is still "abcdefghijklmnopqrstuvwxyzabcdef"

  Scenario: Multiple small entries plus large entry evicts older small entries
    Given history is empty with buffer size 50 and typical line 10
    And I push "cmd1" to history
    And I push "cmd2" to history
    And I push "cmd3" to history
    And I push "cmd4" to history
    And I push "cmd5" to history
    And I push "1234567890123456789012345678901234567890" to history
    When I press up arrow
    Then current entry is "1234567890123456789012345678901234567890"
    When I press up arrow
    Then current entry is "cmd5"
    When I press up arrow
    Then current entry is still "cmd5"

  Scenario: Evicted entries are no longer accessible via navigation
    Given history is empty with buffer size 50 and typical line 10
    And I push "old_entry_long_1" to history
    And I push "old_entry_long_2" to history
    And I push "12345678901234567890123456789012" to history
    When I press up arrow
    Then current entry is "12345678901234567890123456789012"
    When I press up arrow
    Then current entry is "old_entry_long_2"
    And I cannot navigate to older entries

  Scenario: Eviction loop iterates multiple times for large entry
    Given history is empty with buffer size 50 and typical line 10
    And I push "a1" to history
    And I push "a2" to history
    And I push "a3" to history
    And I push "a4" to history
    And I push "a5" to history
    And I push "b1" to history
    And I push "b2" to history
    And I push "b3" to history
    When I push "12345678901234567890123456789012345" to history
    Then I am not browsing history
    When I press up arrow
    Then current entry is "12345678901234567890123456789012345"
    When I press up arrow
    Then current entry is "b3"
    When I press up arrow
    Then current entry is "b2"
    When I press up arrow
    Then current entry is "b1"

  Scenario: Entry capacity check when near buffer limit
    Given history is empty with buffer size 50 and typical line 10
    And I push "start" to history
    And I push "middle" to history
    When I push "end_entry" to history
    Then I am not browsing history
    When I cancel browsing
    And I press up arrow
    Then current entry is "end_entry"
    When I press up arrow
    Then current entry is "middle"
    When I press up arrow
    Then current entry is "start"

  @debug
  Scenario: Entries longer than typical length are stored completely
    Given history is empty
    And I push "short" to history
    And I push "this command is very long and exceeds typical" to history
    When I press up arrow
    Then current entry is "this command is very long and exceeds typical"
    When I press up arrow
    Then current entry is "short"

  Scenario: Out-of-bounds navigation stops at oldest entry
    Given history is empty
    And I push "first" to history
    And I push "second" to history
    When I press up arrow
    Then I am browsing history
    And current entry is "second"
    When I press up arrow
    Then current entry is "first"
    When I press up arrow
    Then current entry is still "first"

  Scenario: Duplicate in middle of history is promoted to front
    Given history is empty
    And I push "cmd1" to history
    And I push "cmd2" to history
    And I push "cmd3" to history
    When I push "cmd2" to history
    Then newest history entry is "cmd2"
    When I cancel browsing
    And I press up arrow
    Then current entry is "cmd2"
    When I press up arrow
    Then current entry is "cmd3"
    When I press up arrow
    Then current entry is "cmd1"

  Scenario: Large entry evicts multiple smaller entries
    Given history is empty with buffer size 50 and typical line 10
    And I push "aa" to history
    And I push "bb" to history
    And I push "cc" to history
    When I push "12345678901234567890123456789" to history
    Then newest history entry is "12345678901234567890123456789"
    When I cancel browsing
    And I press up arrow
    Then current entry is "12345678901234567890123456789"
    When I press up arrow
    Then current entry is "cc"

  Scenario: Case-insensitive duplicate detection preserves latest case
    Given history is empty
    And I push "command" to history
    And I push "other" to history
    When I push "COMMAND" to history
    Then newest history entry is "COMMAND"

  Scenario: Empty input is silently ignored
    Given history is empty
    And I push "first" to history
    When I push "" to history
    Then newest history entry is "first"

  Scenario: Duplicate as first entry then new entry after
    Given history is empty
    And I push "first" to history
    And I push "second" to history
    When I push "first" to history
    Then newest history entry is "first"
    When I push "third" to history
    Then newest history entry is "third"
    When I cancel browsing
    And I press up arrow
    Then current entry is "third"
    When I press up arrow
    Then current entry is "first"
    When I press up arrow
    Then current entry is "second"

  # "ABCDEFGHIJ" and "ZYXWVUTSRQ" are pushed immediately after their own
  # case-insensitive duplicate is already the most recent entry - push()'s
  # Case 1 (already-most-recent) is a pure no-op then, so the originally
  # pushed casing ("abcdefghij"/"zyxwvutsrq") is preserved rather than
  # overwritten by the later push's casing.
  Scenario: New entry evicts oldest entries when buffer is full
    Given history is empty with buffer size 50 and typical line 10
    And I push "0123456789" to history
    And I push "abcdefghij" to history
    And I push "ABCDEFGHIJ" to history
    And I push "zyxwvutsrq" to history
    And history entry count is 3
    When I push "ZYXWVUTSRQ" to history
    Then I am not browsing history
    And history entry count is 3
    When I press up arrow
    Then current entry is "zyxwvutsrq"
