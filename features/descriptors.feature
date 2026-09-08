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

Feature: Command Descriptors

  Scenario Outline: Descriptor stores all fields correctly
    When I create a command descriptor with name "<name>" and permission "<permission>"
    Then the descriptor name should be "<name>"
    And the descriptor hash should match the command name
    And the descriptor permission should be "<permission>"

    Examples: Various commands and permissions
      | name | permission |
      | help | Mode0      |
      | echo | Mode1      |
      | exit | Mode2      |
      | pwd  | AllModes   |

  Scenario: Descriptor with help text stores the help
    When I create a command descriptor with name "ls" and permission "Mode0" and help "List files"
    Then the descriptor name should be "ls"
    And the descriptor help should be "List files"
    And the descriptor permission should be "Mode0"

  Scenario: Descriptor without help text has null help
    When I create a command descriptor with name "clear" and permission "Mode0" without help
    Then the descriptor name should be "clear"
    And the descriptor help should be null

  Scenario: Multiple descriptors have independent hashes
    When I create descriptors for "cat", "grep", and "sed"
    Then all three descriptors should have different hashes
    And each descriptor hash should match its command name

  Scenario Outline: Descriptor hash matching
    When I create a command descriptor with name "<name>" and permission "Mode0"
    Then the descriptor should match hash for "<input>"
    And the descriptor should not match hash for "<non_match>"

    Examples: Hash matching
      | name | input | non_match |
      | help | help  | echo      |
      | echo | echo  | help      |
      | mode | mode  | cat       |

  Scenario Outline: Descriptor name matching (case-insensitive)
    When I create a command descriptor with name "<name>" and permission "Mode0"
    Then the descriptor should match name "<input>"

    Examples: Case-insensitive name matching
      | name | input |
      | help | help  |
      | help | HELP  |
      | help | Help  |
      | echo | echo  |
      | echo | ECHO  |
      | mode | MODE  |

  Scenario: Descriptor name matching fails for non-matching names
    When I create a command descriptor with name "help" and permission "Mode0"
    Then the descriptor should not match name "echo"
    And the descriptor should not match name "hel"

  Scenario Outline: Descriptor prefix matching (case-insensitive)
    When I create a command descriptor with name "<name>" and permission "Mode0"
    Then the descriptor should match prefix "<prefix>" with length <length>

    Examples: Prefix matching
      | name    | prefix | length |
      | help    | h      | 1      |
      | help    | he     | 2      |
      | help    | hel    | 3      |
      | help    | help   | 4      |
      | history | hist   | 4      |
      | history | HIST   | 4      |

  Scenario: Descriptor prefix matching fails for non-matching prefixes
    When I create a command descriptor with name "help" and permission "Mode0"
    Then the descriptor should not match prefix "x" with length 1
    And the descriptor should not match prefix "hx" with length 2

  Scenario Outline: Descriptor visibility based on permissions
    When I create a command descriptor with name "admin" and permission "<permission>"
    Then the descriptor should be hidden in mode "<hidden_mode>"
    And the descriptor should be available in mode "<available_mode>"

    Examples: Permission modes and visibility
      | permission | hidden_mode | available_mode |
      | Mode0      | Mode1       | Mode0          |
      | Mode1      | Mode0       | Mode1          |
      | Mode2      | Mode0       | Mode2          |
      | Mode6      | Mode0       | Mode6          |

  Scenario: Descriptor execute calls the command function
    When I create a command descriptor with name "test" and permission "Mode0"
    Then executing the descriptor should call the command function
