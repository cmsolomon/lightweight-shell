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

Feature: Shell - Command Dispatch and Execution

  Background:
    Given a shell with empty command table

  # Basic Command Dispatch
  Scenario: Execute a single simple command
    Given a command "echo" that returns 0
    When I submit the line "echo"
    Then the shell should return status OK
    And the command exit code should be 0

  Scenario: Unknown command returns CommandNotFound
    When I submit the line "unknown"
    Then the shell should return status CommandNotFound

  Scenario: Empty command returns EmptyCommand
    When I submit the line ""
    Then the shell should return status EmptyCommand

  # Command with Arguments
  Scenario: Command receives parsed arguments
    Given a command "test" that records arguments
    When I submit the line "test arg1 arg2"
    Then the command should have received 2 arguments
    And the first argument should be "arg1"
    And the second argument should be "arg2"

  # Command Chain - Sequential Operator
  Scenario: Semicolon chains commands sequentially
    Given a command "first" that returns 0
    And a command "second" that returns 0
    When I submit the line "first; second"
    Then both commands should have been executed

  Scenario: Semicolon chains execute even if first fails
    Given a command "fail" that returns -1
    And a command "second" that returns 0
    When I submit the line "fail; second"
    Then both commands should have been executed

  # Command Chain - Conditional Operator
  Scenario: Double ampersand continues if first command succeeds
    Given a command "pass" that returns 0
    And a command "second" that returns 0
    When I submit the line "pass && second"
    Then both commands should have been executed

  Scenario: Double ampersand skips second if first fails
    Given a command "fail" that returns -1
    And a command "second" that returns 0
    When I submit the line "fail && second"
    Then only the first command should have been executed

  # Permission Modes
  Scenario: Command respects permission mode
    Given a command "restricted" available only in "Mode1"
    And the shell is in "Mode0"
    When I submit the line "restricted"
    Then the shell should return status PermissionDenied

  Scenario: Command executes when permission granted
    Given a command "restricted" available only in "Mode0"
    And the shell is in "Mode0"
    When I submit the line "restricted"
    Then the shell should return status OK

  # Help Command (Built-in)
  Scenario: Help lists all available commands
    Given a command "echo"
    And a command "status"
    When I submit the line "help"
    Then the output should contain "echo"
    And the output should contain "status"

  # "status" is otherwise only ever referenced in the help-listing scenario
  # above, never actually dispatched - this proves it also executes
  # correctly, not just that it appears in the listing.
  Scenario: The status command dispatches successfully
    Given a command "status" that returns 0
    When I submit the line "status"
    Then the shell should return status OK

  # HideWhenUnavailable only affects the help listing (is_hidden()), not
  # dispatch (tab_complete()/invoke_slice() check is_available() directly) -
  # this proves a hidden command still executes normally once it IS available,
  # distinguishing this from "Hidden command prints [ERROR: Command not
  # found]" below, where the command is hidden AND unavailable.
  Scenario: A hidden command still dispatches successfully once available
    Given a hidden command "secret" available only in "Mode0"
    And the shell is in "Mode0"
    When I submit the line "secret"
    Then the shell should return status OK

  Scenario: Help with specific command shows help text
    Given a command "echo" with help text "Echo the arguments"
    When I submit the line "help echo"
    Then the output should contain "Echo the arguments"

  Scenario: Help for the help command itself does not report not-found
    When I submit the line "help help"
    Then the output should contain "Show available commands"
    And the output should not contain "Command not found"

  Scenario: Help list skips a hidden command
    Given a command "echo"
    And a hidden command "secret" available only in "Mode1"
    And the shell is in "Mode0"
    When I submit the line "help"
    Then the output should contain "echo"
    And the output should not contain "secret"

  # Exercises the help-for-a-specific-command search loop actually skipping
  # past both a null (unregistered) slot and a registered-but-wrong-name
  # entry before finding its match - not reached when the wanted command
  # happens to be first in the array, as in the "shows help text" scenario.
  Scenario: Help for a specific command finds it later in the command table
    Given a command "echo" with help text "Echo the arguments"
    And a command "success" with help text "Reports success"
    When I submit the line "help success"
    Then the output should contain "Reports success"
    And the output should not contain "Echo the arguments"

  # "glbvs" and "yacxa" are a genuine FNV-1a hash collision under hash_cmd()
  # (both hash to 2713492047), and "glbvs" is registered first. Searching for
  # "yacxa" makes the loop hit "glbvs"'s entry first: matches_hash() succeeds
  # (same hash) but matches_name() fails, exercising the continue at
  # lishHelp.h line ~146 - without that check, this lookup could wrongly
  # return "glbvs"'s help text instead of continuing on to "yacxa".
  Scenario: Help lookup by name is correct even for hash-colliding command names
    Given a command "glbvs" with help text "First colliding command"
    And a command "yacxa" with help text "Second colliding command"
    When I submit the line "help yacxa"
    Then the output should contain "Second colliding command"
    And the output should not contain "First colliding command"

  # invoke_slice() checks matches_hash() before matches_name() (see the fix
  # alongside this scenario), so it is now subject to the same collision
  # concern as execute_help() above. "glbvs" is registered first, so
  # dispatching "yacxa" makes the loop hit glbvs's entry first (hash matches,
  # name doesn't - the continue branch) before finding yacxa's real match.
  Scenario: Dispatch resolves correctly for hash-colliding command names too
    Given a command "glbvs" that returns 0
    And a command "yacxa" that returns 0
    When I submit the line "yacxa"
    Then the shell should return status OK

  # Companion to the scenario above: dispatching "glbvs" (registered first)
  # matches on the very first iteration, never reaching yacxa's entry - this
  # doesn't exercise a new branch (it's the same immediate-match path as any
  # ordinary first-registered command), but it does invoke glbvs's own
  # handler at least once directly, alongside yacxa's above.
  Scenario: Dispatch also succeeds for the first-registered hash-colliding command
    Given a command "glbvs" that returns 0
    And a command "yacxa" that returns 0
    When I submit the line "glbvs"
    Then the shell should return status OK

  # Standardized Status Output
  Scenario: Successful command prints [OK]
    Given a command "success" that returns 0
    When I submit the line "success"
    Then the output should contain "[OK]"

  Scenario: Command with non-zero exit code prints [ERROR: Code N]
    Given a command "fail" that returns 42
    When I submit the line "fail"
    Then the output should contain "[ERROR: Code 042]"

  Scenario: Unknown command prints [ERROR: Command not found]
    When I submit the line "nonexistent"
    Then the output should contain "[ERROR: Command not found]"

  # Note: the malformed line is submitted via a dedicated, parameterless step
  # (rather than passing it as a quoted {string} parameter) because the
  # cwt-cucumber Gherkin parser crashes on a quoted parameter containing an
  # odd/unbalanced number of embedded double-quote characters.
  Scenario: Malformed quote syntax prints Invalid argument format
    Given a command "echo" that returns 0
    When I submit a line with an unclosed quote
    Then the output should contain "Invalid argument format"

  Scenario: Permission denied prints [ERROR: Permission denied]
    Given a command "restricted" available only in "Mode1"
    And the shell is in "Mode0"
    When I submit the line "restricted"
    Then the output should contain "[ERROR: Permission denied]"

  Scenario: Hidden command prints [ERROR: Command not found] not Permission denied
    Given a hidden command "secret" available only in "Mode1"
    And the shell is in "Mode0"
    When I submit the line "secret"
    Then the output should contain "[ERROR: Command not found]"
    And the output should not contain "Permission denied"

  # Note: there is no ShellStatus for an oversized line. LineEditor::insert()
  # silently stops accepting characters once the (fixed, compile-time)
  # LineBuffer capacity is reached; whatever fit is submitted as a normal,
  # truncated command line when Enter is pressed - it is not an error.
  Scenario: Typing beyond the line buffer capacity truncates rather than erroring
    Given a command "test" that records arguments
    When I submit the line "test aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    Then the shell should return status OK
    And the first argument should be shorter than 32 characters

  # Note: Shell::on_submit() prints status exactly once per submitted line, for
  # the LAST command's result in a chain - not once per command. An earlier,
  # incorrect version of this test expected one status line per chained
  # command; that was never real Shell behavior.
  Scenario: Chain of successful commands prints a single [OK] for the whole line
    Given a command "first" that returns 0
    And a command "second" that returns 0
    When I submit the line "first; second"
    Then the output should contain "[OK]"

  Scenario: Chain ending in a failed command prints only that command's error code
    Given a command "first" that returns 0
    And a command "second" that returns 5
    When I submit the line "first; second"
    Then the output should contain "[ERROR: Code 005]"
    And the output should not contain "[OK]"

  # IShell API (write/read/set_mode/mode) - exercised by real command handlers
  # in production (see cmd_login.cpp, cmd_mode.cpp), not by pure dispatch
  Scenario: A command handler can use the shell's write/read/set_mode/mode API
    Given a command "test" that exercises the shell API
    When I submit the line "test"
    Then the shell should return status OK

  # Custom Prompt Callback
  Scenario: A custom prompt callback is used instead of the default prompt
    Given a custom prompt callback is used
    And a command "echo" that returns 0
    When I submit the line "echo"
    Then the output should contain "CUSTOM>"

  # Non-Interactive Dispatch (run_line, as used by e.g. cmd_sudo.cpp to
  # re-dispatch a reconstructed command line without a second status line)
  Scenario: run_line dispatches non-interactively without printing status
    Given a command "success" that returns 0
    When I run the line "success" directly
    Then the shell should return status OK

  Scenario: run_line with an empty line returns EmptyCommand without dispatching
    When I run the line "" directly
    Then the shell should return status EmptyCommand
