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

Feature: Line Buffer Management

  Scenario: LineBuffer initializes to empty state
    When I create a new line buffer
    Then the line length should be 0
    And the cursor position should be 0
    And the buffer should contain only null terminator

  Scenario: clear() resets buffer to empty state
    When I create a new line buffer
    And I copy "hello" into the buffer
    Then the line length should be 5
    When I clear the buffer
    Then the line length should be 0
    And the cursor position should be 0

  Scenario Outline: copy_from() copies string and positions cursor at end
    When I create a new line buffer
    And I copy "<source>" into the buffer
    Then the line length should be <length>
    And the cursor position should be <length>
    And the buffer should contain "<source>"

    Examples: Various strings
      | source           | length |
      | hello            | 5      |
      | ls -la           | 6      |
      | echo test        | 9      |
      | a                | 1      |
      | 12345            | 5      |

  Scenario: copy_from() with empty string clears buffer
    When I create a new line buffer
    And I copy "hello" into the buffer
    Then the line length should be 5
    When I copy "" into the buffer
    Then the line length should be 0
    And the cursor position should be 0

  Scenario: Cursor position is always at end after copy_from()
    When I create a new line buffer
    And I copy "cat file.txt" into the buffer
    Then the cursor position should equal the line length
    And the cursor position should be 12

  Scenario: Buffer is null-terminated after copy_from()
    When I create a new line buffer
    And I copy "test" into the buffer
    Then the buffer should be null-terminated at position 4

  Scenario: is_blank() returns true for empty buffer
    When I create a new line buffer
    Then the buffer should be blank

  Scenario: is_blank() returns true for spaces only
    When I create a new line buffer
    And I copy "   " into the buffer
    Then the buffer should be blank

  Scenario: is_blank() returns true for tabs only
    When I create a new line buffer
    And I copy "		" into the buffer
    Then the buffer should be blank

  Scenario: is_blank() returns true for mixed whitespace
    When I create a new line buffer
    And I copy " 	 	 " into the buffer
    Then the buffer should be blank

  Scenario: is_blank() returns false for content
    When I create a new line buffer
    And I copy "hello" into the buffer
    Then the buffer should not be blank

  Scenario: is_blank() returns false for content with whitespace
    When I create a new line buffer
    And I copy "  hello  " into the buffer
    Then the buffer should not be blank
