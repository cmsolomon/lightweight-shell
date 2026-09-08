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

Feature: Line Editor - Interactive Command-Line Editing

  Scenario: Insert single character at start of empty line
    When I create a line editor
    Then cursor position should be 0
    When I insert "a"
    Then line length should be 1
    And cursor position should be 1
    And buffer content should be "a"
    And buffer should be null-terminated at position 1

  Scenario: Insert multiple characters in sequence
    When I create a line editor
    When I insert "h"
    When I insert "i"
    When I insert "!"
    Then line length should be 3
    And cursor position should be 3
    And buffer content should be "hi!"

  Scenario: Insert at cursor in middle of text
    When I create a line editor with "hello"
    When I move cursor to position 2
    When I insert "x"
    Then buffer content should be "hexllo"
    And line length should be 6
    And cursor position should be 3

  Scenario: Insert when buffer is full should be ignored
    When I create a line editor with maximum length "abcdefghijklmnopqrstuvwxyz1234567890123456789012345678901234567"
    And line length should be 63
    When I insert "x"
    Then line length should be 63
    And buffer should not contain "x" at end

  Scenario: Backspace at cursor position
    When I create a line editor with "hello"
    Then cursor position should be 5
    When I backspace
    Then line length should be 4
    And cursor position should be 4
    And buffer content should be "hell"

  Scenario: Backspace in middle of text
    When I create a line editor with "hello"
    When I move cursor to position 3
    When I backspace
    Then line length should be 4
    And cursor position should be 2
    And buffer content should be "helo"

  Scenario: Backspace at start of line should do nothing
    When I create a line editor with "hello"
    When I move cursor to position 0
    When I backspace
    Then line length should be 5
    And cursor position should be 0
    And buffer content should be "hello"

  Scenario: Backspace on empty line should do nothing
    When I create a line editor
    When I backspace
    Then line length should be 0
    And cursor position should be 0

  Scenario: Delete character at cursor
    When I create a line editor with "hello"
    When I move cursor to position 1
    When I delete at cursor
    Then line length should be 4
    And cursor position should be 1
    And buffer content should be "hllo"

  Scenario: Delete at end of line should do nothing
    When I create a line editor with "hello"
    When I move cursor to position 5
    When I delete at cursor
    Then line length should be 5
    And cursor position should be 5
    And buffer content should be "hello"

  Scenario: Delete at start of line
    When I create a line editor with "hello"
    When I move cursor to position 0
    When I delete at cursor
    Then line length should be 4
    And cursor position should be 0
    And buffer content should be "ello"

  Scenario: Cursor move left decreases position
    When I create a line editor with "hello"
    When I move cursor to position 3
    When I move cursor left
    Then cursor position should be 2

  Scenario: Cursor move left at start should do nothing
    When I create a line editor with "hello"
    When I move cursor to position 0
    When I move cursor left
    Then cursor position should be 0

  Scenario: Cursor move right increases position
    When I create a line editor with "hello"
    When I move cursor to position 2
    When I move cursor right
    Then cursor position should be 3

  Scenario: Cursor move right at end should do nothing
    When I create a line editor with "hello"
    When I move cursor to position 5
    When I move cursor right
    Then cursor position should be 5

  Scenario: Cursor move home goes to start
    When I create a line editor with "hello"
    When I move cursor to position 3
    When I move cursor home
    Then cursor position should be 0

  Scenario: Cursor move end goes to line length
    When I create a line editor with "hello"
    When I move cursor to position 2
    When I move cursor end
    Then cursor position should be 5
    And cursor position should equal line length

  Scenario: Buffer always remains null-terminated after insert
    When I create a line editor
    When I insert "a"
    When I insert "b"
    When I insert "c"
    Then buffer should be null-terminated at position 3

  Scenario: Buffer always remains null-terminated after delete
    When I create a line editor with "hello"
    When I move cursor to position 2
    When I delete at cursor
    Then buffer should be null-terminated at position 4

  Scenario: Complex edit sequence - typing with corrections
    When I create a line editor
    When I insert "t"
    When I insert "e"
    When I insert "s"
    When I insert "t"
    When I move cursor to position 2
    When I delete at cursor
    Then buffer content should be "tet"
    And line length should be 3
    And cursor position should be 2
    When I insert "s"
    Then buffer content should be "test"
    And line length should be 4

  Scenario: Cursor position always within valid range
    When I create a line editor with "hello"
    When I move cursor to position 5
    When I move cursor right
    When I move cursor right
    Then cursor position should equal line length

  Scenario: Insert at various positions creates correct result
    When I create a line editor with "hllo"
    When I move cursor to position 1
    When I insert "e"
    Then buffer content should be "hello"
    And cursor position should be 2
    And line length should be 5

  Scenario: Multiple operations maintain null-termination invariant
    When I create a line editor with "test"
    When I move cursor to position 2
    When I delete at cursor
    When I insert "a"
    When I move cursor to position 4
    When I backspace
    Then buffer should be null-terminated at position 3
    And buffer content should be "tea"

  Scenario: Edit empty line sequence
    When I create a line editor
    Then line length should be 0
    And cursor position should be 0
    When I insert "a"
    Then line length should be 1
    When I backspace
    Then line length should be 0
    And cursor position should be 0

  Scenario: Single character line operations
    When I create a line editor with "x"
    Then line length should be 1
    When I move cursor to position 0
    When I delete at cursor
    Then line length should be 0
    When I insert "y"
    Then buffer content should be "y"
    And line length should be 1

  Scenario: Cursor position tracking through sequence
    When I create a line editor with "abc"
    When I move cursor to position 1
    When I insert "X"
    Then cursor position should be 2
    When I move cursor left
    Then cursor position should be 1
    When I delete at cursor
    Then cursor position should be 1
    And buffer content should be "abc"
