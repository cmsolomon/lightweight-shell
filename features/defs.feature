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

Feature: Core Utilities - Hashing, Character Conversion, and I/O

  Scenario Outline: Convert uppercase letters to lowercase
    When I convert "<input>" to lowercase
    Then the result should be "<expected>"

    Examples: Uppercase alphabet (all 26 letters)
      | input | expected |
      | A     | a        |
      | B     | b        |
      | C     | c        |
      | D     | d        |
      | E     | e        |
      | F     | f        |
      | G     | g        |
      | H     | h        |
      | I     | i        |
      | J     | j        |
      | K     | k        |
      | L     | l        |
      | M     | m        |
      | N     | n        |
      | O     | o        |
      | P     | p        |
      | Q     | q        |
      | R     | r        |
      | S     | s        |
      | T     | t        |
      | U     | u        |
      | V     | v        |
      | W     | w        |
      | X     | x        |
      | Y     | y        |
      | Z     | z        |

  Scenario Outline: Lowercase letters pass through unchanged
    When I convert "<input>" to lowercase
    Then the result should be "<expected>"

    Examples: Lowercase alphabet (all 26 letters)
      | input | expected |
      | a     | a        |
      | b     | b        |
      | c     | c        |
      | d     | d        |
      | e     | e        |
      | f     | f        |
      | g     | g        |
      | h     | h        |
      | i     | i        |
      | j     | j        |
      | k     | k        |
      | l     | l        |
      | m     | m        |
      | n     | n        |
      | o     | o        |
      | p     | p        |
      | q     | q        |
      | r     | r        |
      | s     | s        |
      | t     | t        |
      | u     | u        |
      | v     | v        |
      | w     | w        |
      | x     | x        |
      | y     | y        |
      | z     | z        |

  Scenario Outline: Digits pass through unchanged
    When I convert "<input>" to lowercase
    Then the result should be "<expected>"

    Examples: Digits 0-9 (all 10 digits)
      | input | expected |
      | 0     | 0        |
      | 1     | 1        |
      | 2     | 2        |
      | 3     | 3        |
      | 4     | 4        |
      | 5     | 5        |
      | 6     | 6        |
      | 7     | 7        |
      | 8     | 8        |
      | 9     | 9        |

  Scenario Outline: ASCII symbols pass through unchanged
    When I convert "<input>" to lowercase
    Then the result should be "<expected>"

    Examples: ASCII symbols (sample)
      | input | expected |
      | !     | !        |
      | $     | $        |
      | %     | %        |
      | &     | &        |
      | '     | '        |
      | (     | (        |
      | )     | )        |
      | *     | *        |
      | +     | +        |
      | ,     | ,        |
      | -     | -        |
      | .     | .        |
      | /     | /        |
      | :     | :        |
      | ;     | ;        |
      | <     | <        |
      | =     | =        |
      | >     | >        |
      | ?     | ?        |
      | @     | @        |
      | [     | [        |
      | \     | \        |
      | ]     | ]        |
      | ^     | ^        |
      | _     | _        |
      | ~     | ~        |

      # Gherkin framework limitation: Cannot test these 6 symbols due to escaping/parsing issues:
      # " (double quote), # (hash), ` (backtick), { (left brace), } (right brace), | (pipe)
      # However, to_lower() implementation guarantees the behavior: only A-Z are modified,
      # all other characters (including untested symbols) pass through unchanged.

  Scenario Outline: Hash command names for fast O(1) lookup
    When I hash the command "<command>"
    Then the hash should be valid and consistent

    Examples: Common command hashes
      | command |
      | echo    |
      | cat     |
      | ls      |
      | grep    |
      | help    |
      | exit    |
      | pwd     |
      | cp      |
      | rm      |
      | mv      |

  Scenario Outline: Hash is case-insensitive
    When I hash the command "<uppercase>"
    And I hash the command "<lowercase>"
    Then both hashes should be equal

    Examples: Case-insensitive hashing
      | uppercase | lowercase |
      | ECHO      | echo      |
      | CAT       | cat       |
      | HELP      | help      |
      | LS        | ls        |
      | GREP      | grep      |

  Scenario Outline: Hash produces correct FNV-1a values
    When I hash the command "<command>"
    Then the hash should match expected value "<expected_hex>"

    Examples: FNV-1a test vectors
      | command | expected_hex |
      | echo    | d49dd484     |
      | help    | 3871a3fa     |
      | ls      | 5631dfe8     |
      | cat     | 06745c07     |
      | grep    | bb9027d1     |

  Scenario: Write empty string to I/O adapter
    When I write an empty string to the output
    Then the output should be empty

  Scenario Outline: Write single character strings to I/O adapter
    When I write "<text>" to the output
    Then the output should contain "<text>"

    Examples: Single characters
      | text |
      | a    |
      | Z    |
      | 0    |
      | !    |
      | @    |

  Scenario Outline: Write multi-character strings to I/O adapter
    When I write "<text>" to the output
    Then the output should contain "<text>"

    Examples: Multi-character strings
      | text           |
      | hello          |
      | Hello World    |
      | test123!@#     |
      | command-name   |
      | UPPERCASE      |

  Scenario: ANSI dim constant is available and non-empty
    When I check the ANSI_DIM constant
    Then it should be a valid ANSI escape sequence

  Scenario: ANSI reset constant is available and non-empty
    When I check the ANSI_RESET constant
    Then it should be a valid ANSI escape sequence

  Scenario: Write ANSI codes to output
    When I write the ANSI_DIM constant to the output
    And I write "dimmed text" to the output
    And I write the ANSI_RESET constant to the output
    Then the output should start with the escape character
    And the output should contain "dimmed text"

