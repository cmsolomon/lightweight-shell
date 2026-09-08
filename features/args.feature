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

Feature: Argument Parsing

  Scenario: Command with no arguments
    Given I have a command string "help"
    When I parse the command line
    Then the command should be "help"
    And the argument count should be 0
    And arguments 0 to 8 return null

  Scenario: Command with single argument
    Given I have a command string "echo hello"
    When I parse the command line
    Then the command should be "echo"
    And the argument count should be 1
    And argument 0 should be "hello"
    And arguments 1 to 8 return null

  Scenario: Command with multiple arguments
    Given I have a command string "cat file1 file2 file3"
    When I parse the command line
    Then the command should be "cat"
    And the argument count should be 3
    And argument 0 should be "file1"
    And argument 1 should be "file2"
    And argument 2 should be "file3"
    And arguments 3 to 8 return null

  Scenario: Leading whitespace is skipped
    Given I have a command string "   echo hello"
    When I parse the command line
    Then the command should be "echo"
    And the argument count should be 1
    And argument 0 should be "hello"

  Scenario: Multiple spaces between arguments are skipped
    Given I have a command string "echo    hello    world"
    When I parse the command line
    Then the command should be "echo"
    And the argument count should be 2
    And argument 0 should be "hello"
    And argument 1 should be "world"

  Scenario: Second command slice after semicolon
    Given I have a command string "cat file.txt;grep pattern file.txt"
    When I parse the second command slice
    Then the command should be "grep"
    And the argument count should be 2
    And argument 0 should be "pattern"
    And argument 1 should be "file.txt"
    And arguments 2 to 8 return null

  Scenario: Parse with nullptr buffer
    Given I have a nullptr buffer
    When I parse the command line
    Then the terminator should be EndOfLine
    And the command should be ""
    And the argument count should be 0

  Scenario: Parse with slice_offset beyond buffer length
    Given I have a command string "echo hello"
    When I parse with slice_offset beyond buffer length
    Then the terminator should be EndOfLine
    And the command should be ""
    And the argument count should be 0

  Scenario: Line starting with semicolon (no spaces after)
    Given I have a command string ";echo hello"
    When I parse the command line
    Then the command should be "echo"
    And the argument count should be 1
    And argument 0 should be "hello"

  Scenario: Line starting with semicolon (spaces after)
    Given I have a command string ";  echo hello"
    When I parse the command line
    Then the command should be "echo"
    And the argument count should be 1
    And argument 0 should be "hello"

  Scenario: Line starting with spaces and semicolon (no spaces after)
    Given I have a command string "  ;echo hello"
    When I parse the command line
    Then the command should be "echo"
    And the argument count should be 1
    And argument 0 should be "hello"

  Scenario: Line starting with spaces and semicolon (spaces after)
    Given I have a command string "  ;  echo hello"
    When I parse the command line
    Then the command should be "echo"
    And the argument count should be 1
    And argument 0 should be "hello"

  Scenario: Line starting with && (no spaces after)
    Given I have a command string "&&echo hello"
    When I parse the command line
    Then the command should be "echo"
    And the argument count should be 1
    And argument 0 should be "hello"

  Scenario: Line starting with && (spaces after)
    Given I have a command string "&&  echo hello"
    When I parse the command line
    Then the command should be "echo"
    And the argument count should be 1
    And argument 0 should be "hello"

  Scenario: Line starting with spaces and && (no spaces after)
    Given I have a command string "  &&echo hello"
    When I parse the command line
    Then the command should be "echo"
    And the argument count should be 1
    And argument 0 should be "hello"

  Scenario: Line starting with spaces and && (spaces after)
    Given I have a command string "  &&  echo hello"
    When I parse the command line
    Then the command should be "echo"
    And the argument count should be 1
    And argument 0 should be "hello"

  Scenario: Two commands chained with &&
    Given I have a command string "echo hello && cat file.txt"
    When I parse the command line
    Then the command should be "echo"
    And the argument count should be 1
    And argument 0 should be "hello"
    And the terminator should be LogicalAnd

  Scenario: Second command after &&
    Given I have a command string "echo hello && cat file.txt"
    When I parse the second command slice
    Then the command should be "cat"
    And the argument count should be 1
    And argument 0 should be "file.txt"

  Scenario: Buffer reaches max_length without null terminator
    Given I have a command string that fills the buffer with no null terminator
    When I parse the command line
    Then the terminator should be EndOfLine
    And the command should be "echo"
    And the argument count should be 1
    And argument 0 should be "hello"

  Scenario: Single-quoted argument with spaces preserves content
    Given I have a command string "cmdB 'test data'"
    When I parse the command line
    Then the command should be "cmdB"
    And the argument count should be 1
    And argument 0 should be "test data"

  Scenario: Mixed quoted and unquoted - single quotes
    Given I have a command string "cmdC plain 'has space' another"
    When I parse the command line
    Then the command should be "cmdC"
    And the argument count should be 3
    And argument 0 should be "plain"
    And argument 1 should be "has space"
    And argument 2 should be "another"

  Scenario: Unclosed single quote returns parse error
    Given I have a command string "cmdD 'unclosed"
    When I parse the command line
    Then the terminator should be ParseError

  Scenario: Quote in middle of token returns parse error
    Given I have a command string "cmdE unquoted'part"
    When I parse the command line
    Then the terminator should be ParseError

  Scenario: Command chaining with && after quoted argument
    Given I have a command string "cmdF 'test data' && cmdG arg"
    When I parse the command line
    Then the command should be "cmdF"
    And the argument count should be 1
    And argument 0 should be "test data"
    And the terminator should be LogicalAnd

  Scenario: Second command after && with quoted argument
    Given I have a command string "cmdF 'test data' && cmdG arg"
    When I parse the second command slice
    Then the command should be "cmdG"
    And the argument count should be 1
    And argument 0 should be "arg"

  Scenario: Direct && after closing quote (no space)
    Given I have a command string "cmdH 'test'&& cmdI"
    When I parse the command line
    Then the command should be "cmdH"
    And the argument count should be 1
    And argument 0 should be "test"
    And the terminator should be LogicalAnd
