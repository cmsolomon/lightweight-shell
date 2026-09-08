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

Feature: Tab Completion for Command Names

  Scenario: Tab on empty buffer does nothing
    When I create a line buffer with ""
    And I press tab
    Then the buffer should remain unchanged

  Scenario: Tab with no matches does nothing
    When I create a line buffer with "xyz"
    And I press tab with commands "cat" "echo" "help"
    Then the buffer should remain unchanged
    And no output should be produced

  Scenario: Tab completes partial command - single match
    When I create a line buffer with "hel"
    And I press tab with commands "help" "history"
    Then the buffer should be "help "
    And no output should be produced

  Scenario: Tab with exact command match does nothing
    When I create a line buffer with "help"
    And I press tab with commands "help" "history"
    Then the buffer should remain unchanged
    And no output should be produced

  Scenario: Tab after space (command portion done) does nothing
    When I create a line buffer with "help "
    And I press tab with commands "help" "history"
    Then the buffer should remain unchanged
    And no output should be produced

  Scenario: Tab with parameters present does nothing
    When I create a line buffer with "help file"
    And I press tab with commands "help"
    Then the buffer should remain unchanged
    And no output should be produced

  Scenario: Tab completion in first segment only
    When I create a line buffer with "cat file ; hel"
    And I press tab with commands "help" "history"
    Then the buffer should be "cat file ; help "
    And no output should be produced

  Scenario: Tab completion after semicolon separator
    When I create a line buffer with "help ; ec"
    And I press tab with commands "echo" "edit"
    Then the buffer should be "help ; echo "
    And no output should be produced

  Scenario: Tab completion after && separator
    When I create a line buffer with "cat && ec"
    And I press tab with commands "echo" "edit"
    Then the buffer should be "cat && echo "
    And no output should be produced

  Scenario: Tab ignores earlier segments
    When I create a line buffer with "help xyz ; ca"
    And I press tab with commands "cat" "echo"
    Then the buffer should be "help xyz ; cat "
    And no output should be produced

  Scenario: Case-insensitive completion
    When I create a line buffer with "HEL"
    And I press tab with commands "help" "history"
    Then the buffer should be "help "
    And no output should be produced

  Scenario: Multiple segments - only last one is tab-completed
    When I create a line buffer with "help file1 ; hel"
    And I press tab with commands "help" "history"
    Then the buffer should be "help file1 ; help "
    And the first segment should remain "help file1 ;"

  Scenario: Multiple matching commands are listed
    When I create a line buffer with "he"
    And I set the prompt to "> "
    And I press tab with commands "help" "hello" "heading"
    Then the output should list "help" "hello" "heading"
    And the buffer should remain unchanged

  Scenario: Tab completion does nothing if buffer would overflow
    When I create a line buffer with "abcdefghijklmnopqrstuvwxyz1234567890123456789012345"
    And I press tab with commands "help" "history"
    Then the buffer should remain unchanged
    And no output should be produced

  Scenario: Tab completion works with leading spaces
    When I create a line buffer with "    hel"
    And I press tab with commands "help" "history"
    Then the buffer should be "    help "
    And no output should be produced

  Scenario: Tab with prefix that matches no commands in list
    When I create a line buffer with "xyz"
    And I press tab with commands "help" "history" "hello"
    Then the buffer should remain unchanged
    And no output should be produced

  Scenario: Command unavailable in current mode is skipped in single-match search
    When I create a line buffer with "ad"
    And I set the permission mode to Mode0
    And I press tab with commands "help" "admin"
    Then the buffer should remain unchanged
    And no output should be produced

  Scenario: Command available in current mode completes despite unavailable match
    When I create a line buffer with "hel"
    And I set the permission mode to Mode0
    And I press tab with commands "help" "admin"
    Then the buffer should be "help "
    And no output should be produced

  Scenario: Multiple matching commands lists only those available in current mode
    When I create a line buffer with "he"
    And I set the permission mode to Mode0
    And I press tab with commands "hello" "help" "admin"
    Then the buffer should remain unchanged
    And the output should list "hello" and "help"

  Scenario: Mode1-only commands are included when in Mode1
    When I create a line buffer with "a"
    And I set the permission mode to Mode1
    And I press tab with commands "admin" "service"
    Then the buffer should be "admin "
    And no output should be produced

  Scenario: Multiple matching commands skips non-matching commands in list
    When I create a line buffer with "he"
    And I set the permission mode to Mode0
    And I press tab with commands "hello" "help" "heading"
    Then the buffer should remain unchanged
    And the output should list "hello" "help" "heading"

