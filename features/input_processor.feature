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

Feature: Input Processor - Character Routing and State Machine

  Background:
    Given I create a new input processor

  # Printable Character Routing
  Scenario: Printable ASCII routes to editor insert
    When I process the character "a"
    Then editor.insert should have been called with "a"
    And process_char should return false

  Scenario: Printable character 0x20 (space) routes to editor insert
    When I process the character with code 0x20
    Then editor.insert should have been called

  Scenario: Printable character 0x7E (~) routes to editor insert
    When I process the character with code 0x7E
    Then editor.insert should have been called

  Scenario: Non-printable character 0x01 does nothing
    When I process the character with code 0x01
    Then no component calls should be made

  Scenario: Null character 0x00 does nothing
    When I process the character with code 0x00
    Then no component calls should be made

  # Backspace/Delete Routing
  Scenario: Backspace (0x08) routes to editor backspace
    When I process the character with code 0x08
    Then editor.backspace should have been called
    And process_char should return false

  Scenario: Delete (0x7F) routes to editor backspace
    When I process the character with code 0x7F
    Then editor.backspace should have been called

  # Tab Routing
  Scenario: Tab (0x09) routes to tab_complete
    When I process the character "h"
    And I process the character with code 0x09
    Then tab_complete should have been called
    And editor.move_end should have been called after tab_complete
    And process_char should return false

  # Line Submission Routing
  Scenario: Enter (0x0D) signals line ready
    When I process the character with code 0x0D
    Then process_char should return true
    And no further input processing should occur

  Scenario: Line Feed (0x0A) signals line ready
    When I process the character with code 0x0A
    Then process_char should return true

  # CR/LF Deduplication
  Scenario: CR followed by LF - first CR returns true
    When I process the character with code 0x0D
    Then process_char should return true

  Scenario: CR followed by LF - LF returns false (deduplicated)
    When I process the character with code 0x0D
    And I process the character with code 0x0A
    Then the second call to process_char should return false

  Scenario: LF followed by CR - first LF returns true
    When I process the character with code 0x0A
    Then process_char should return true

  Scenario: LF followed by CR - CR returns false (deduplicated)
    When I process the character with code 0x0A
    And I process the character with code 0x0D
    Then the second call to process_char should return false

  Scenario: Two separate CRs are not deduplicated
    When I process the character with code 0x0D
    And I process the character with code 0x0A
    And I process the character with code 0x0D
    Then the third call to process_char should return true

  # ANSI State Machine - Arrow Keys
  Scenario: CSI Up Arrow routes to history prev
    When I process ANSI CSI escape "A"
    Then history.prev should have been called
    And process_char should return false

  Scenario: CSI Down Arrow routes to history next
    When I process ANSI CSI escape "B"
    Then history.next should have been called

  Scenario: CSI Right Arrow routes to editor move_right
    When I process ANSI CSI escape "C"
    Then editor.move_right should have been called

  Scenario: CSI Left Arrow routes to editor move_left
    When I process ANSI CSI escape "D"
    Then editor.move_left should have been called

  Scenario: SS3 Up Arrow routes to history prev
    When I process ANSI SS3 escape "A"
    Then history.prev should have been called

  Scenario: SS3 Down Arrow routes to history next
    When I process ANSI SS3 escape "B"
    Then history.next should have been called

  Scenario: SS3 Right Arrow routes to editor move_right
    When I process ANSI SS3 escape "C"
    Then editor.move_right should have been called

  Scenario: SS3 Left Arrow routes to editor move_left
    When I process ANSI SS3 escape "D"
    Then editor.move_left should have been called

  Scenario: SS3 End (F) routes to editor move_end
    When I process ANSI SS3 escape "F"
    Then editor.move_end should have been called

  Scenario: SS3 Home (H) routes to editor move_home
    When I process ANSI SS3 escape "H"
    Then editor.move_home should have been called

  # ANSI State Machine - Function Keys
  Scenario: CSI Home key (1~) routes to editor move_home
    When I process ANSI CSI escape "1~"
    Then editor.move_home should have been called

  Scenario: CSI Home key (7~) also routes to editor move_home
    When I process ANSI CSI escape "7~"
    Then editor.move_home should have been called

  Scenario: CSI End key (4~) routes to editor move_end
    When I process ANSI CSI escape "4~"
    Then editor.move_end should have been called

  Scenario: CSI End key (8~) also routes to editor move_end
    When I process ANSI CSI escape "8~"
    Then editor.move_end should have been called

  Scenario: CSI Delete key (3~) routes to editor delete_at_cursor
    When I process ANSI CSI escape "3~"
    Then editor.delete_at_cursor should have been called

  # ANSI State Machine - State Transitions
  Scenario: ESC (0x1B) is consumed without action
    When I process the character with code 0x1B
    Then no component calls should be made
    And process_char should return false

  Scenario: CSI sequence (ESC+[) is recognized
    When I process the character with code 0x1B
    And I process the character "["
    Then no component calls should be made
    And process_char should return false

  Scenario: ESC followed by O doesn't trigger action immediately
    When I process the character with code 0x1B
    And I process the character "O"
    Then no component calls should be made
    And process_char should return false

  Scenario: ESC followed by unrecognized character is treated as regular character
    When I process the character with code 0x1B
    And I process the character "X"
    Then editor.insert should have been called with "X"

  Scenario: CSI numeric parameter sequence is recognized
    Given the line buffer contains "ab"
    And the cursor is at the start
    When I process the character with code 0x1B
    And I process the character "["
    And I process the character "3"
    And I process the character "~"
    Then editor.delete_at_cursor should have been called

  Scenario: CSI ~ with parameter 1 routes correctly
    When I process ANSI CSI escape "1~"
    Then editor.move_home should have been called

  Scenario: CSI ~ with parameter 3 routes correctly
    When I process ANSI CSI escape "3~"
    Then editor.delete_at_cursor should have been called

  Scenario: CSI unhandled character resets state
    When I process the character with code 0x1B
    And I process the character "["
    And I process the character "X"
    Then no component calls should be made
    And process_char should return false

  Scenario: Insert character in middle of line redraws
    Given the line buffer contains "hello"
    And the cursor is at the end
    When I move cursor left 3 times
    And I insert the character "X"
    Then the line buffer should contain "heXllo"
    And the line should have been redrawn to output

  Scenario: Delete character in middle of line redraws
    Given the line buffer contains "hello"
    And the cursor is at the end
    When I move cursor left 3 times
    And I delete the character
    Then the line buffer should contain "helo"
    And the line should have been redrawn to output

  Scenario: Move right with CSI escape in middle of line redraws
    Given the line buffer contains "hello"
    And the cursor is at the start
    When I move cursor right 2 times using CSI
    Then the line should have been redrawn to output

  Scenario: Move right with SS3 escape in middle of line redraws
    Given the line buffer contains "hello"
    And the cursor is at the start
    When I move cursor right 2 times using SS3
    Then the line should have been redrawn to output

  Scenario: Home key moves cursor to beginning of line
    Given the line buffer contains "hello"
    And the cursor is at the end
    When I process ANSI SS3 escape "H"
    Then the cursor position should be 0

  Scenario: End key in middle of line redraws
    Given the line buffer contains "hello"
    And the cursor is at the start
    When I move cursor right 2 times using CSI
    And I process ANSI SS3 escape "F"
    Then the line should have been redrawn to output

  Scenario: Down arrow loads history entry when next returns true
    Given the line buffer contains "hello"
    And history is being browsed
    And history next will return true
    When I process ANSI CSI escape "B"
    Then the line should have been redrawn to output

  # History Navigation and Display
  Scenario: Up arrow calls history prev
    When I process ANSI CSI escape "A"
    Then history.prev should have been called
    And process_char should return false

  Scenario: Up arrow loads history entry when prev returns true
    When I process ANSI CSI escape "A"
    And history.prev returns true
    Then line_buf.copy_from should have been called with history.current
    And the prompt should be printed
    And the history entry should be displayed

  Scenario: Up arrow does nothing when prev returns false
    When I process ANSI CSI escape "A"
    And history.prev returns false
    Then line_buf should not be modified
    And no output should be produced

  Scenario: Down arrow calls history next
    When I process ANSI CSI escape "B"
    Then history.next should have been called
    And process_char should return false

  Scenario: Down arrow loads history entry when next returns true
    When I process ANSI CSI escape "B"
    And history.next returns true
    Then line_buf.copy_from should have been called with history.current
    And the prompt should be printed
    And the history entry should be displayed

  Scenario: Down arrow exits browsing and shows line buffer when next returns false
    When I process ANSI CSI escape "B"
    And history.next returns false
    Then line_buf.clear should have been called
    And the prompt should be printed
    And the buffer should be empty

  # History Editing
  Scenario: Insert while browsing copies history entry to buffer
    Given history is being browsed
    When I process the character "a"
    Then line_buf.copy_from should have been called with history.current first
    And then editor.insert should have been called with "a"
    And browsing mode should be exited

  Scenario: Backspace while browsing copies history entry to buffer
    Given history is being browsed
    When I process the character with code 0x08
    Then line_buf.copy_from should have been called with history.current first
    And then editor.backspace should have been called
    And browsing mode should be exited

  Scenario: Delete while browsing copies history entry to buffer
    Given history is being browsed
    When I process ANSI CSI escape "3~"
    Then line_buf.copy_from should have been called with history.current first
    And then editor.delete_at_cursor should have been called
    And browsing mode should be exited

  # History and Line Submission
  Scenario: Enter while browsing copies history entry and signals ready
    Given history is being browsed
    When I process the character with code 0x0D
    Then line_buf.copy_from should have been called with history.current first
    And process_char should return true
    And history.cancel_browsing should have been called
