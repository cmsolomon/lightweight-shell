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

Feature: Command Permissions

  Scenario Outline: Verify mode availability for permissions
    Given permissions are set to "<permissions>"
    When I check permissions
    Then only the following modes should be available: "<available_modes>"

    Examples: Single Mode Permissions
      | permissions | available_modes                                       |
      | Mode0       | Mode0                                                 |
      | Mode1       | Mode1                                                 |
      | Mode2       | Mode2                                                 |
      | Mode3       | Mode3                                                 |
      | Mode4       | Mode4                                                 |
      | Mode5       | Mode5                                                 |
      | Mode6       | Mode6                                                 |
      | AllModes    | Mode0, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6       |

  Scenario: Verify no modes available for None permission
    Given permissions are set to "None"
    When I check permissions
    Then no modes should be available

  Scenario Outline: Verify mode availability for multi-mode permissions
    Given permissions are set to "<permissions>"
    When I check permissions
    Then only the following modes should be available: "<available_modes>"

    Examples: Two Mode Combinations
      | permissions | available_modes                 |
      | Mode0,Mode1 | Mode0, Mode1                    |
      | Mode1,Mode2 | Mode1, Mode2                    |
      | Mode2,Mode3 | Mode2, Mode3                    |
      | Mode3,Mode4 | Mode3, Mode4                    |
      | Mode4,Mode5 | Mode4, Mode5                    |
      | Mode5,Mode6 | Mode5, Mode6                    |

    Examples: Three Mode Combinations
      | permissions       | available_modes       |
      | Mode0,Mode1,Mode2 | Mode0, Mode1, Mode2   |
      | Mode1,Mode2,Mode3 | Mode1, Mode2, Mode3   |
      | Mode2,Mode3,Mode4 | Mode2, Mode3, Mode4   |
      | Mode3,Mode4,Mode5 | Mode3, Mode4, Mode5   |
      | Mode4,Mode5,Mode6 | Mode4, Mode5, Mode6   |

    Examples: Four Mode Combinations
      | permissions             | available_modes           |
      | Mode0,Mode1,Mode2,Mode3 | Mode0, Mode1, Mode2, Mode3 |
      | Mode3,Mode4,Mode5,Mode6 | Mode3, Mode4, Mode5, Mode6 |

  Scenario Outline: Verify real-world permission patterns
    Given permissions are set to "<permissions>"
    When I check permissions
    Then only the following modes should be available: "<available_modes>"

    Examples: Common Patterns
      | permissions                | available_modes                     |
      | AllModes & ~Mode0          | Mode1, Mode2, Mode3, Mode4, Mode5, Mode6 |
      | AllModes & ~Mode1          | Mode0, Mode2, Mode3, Mode4, Mode5, Mode6 |
      | AllModes & ~Mode6          | Mode0, Mode1, Mode2, Mode3, Mode4, Mode5 |
      | AllModes & ~(Mode0,Mode1)  | Mode2, Mode3, Mode4, Mode5, Mode6  |
      | AllModes & ~(Mode0,Mode1,Mode2) | Mode3, Mode4, Mode5, Mode6  |

  Scenario Outline: Verify bitwise AND operations
    Given permission A is set to "<permissionA>"
    And permission B is set to "<permissionB>"
    When I compute A AND B
    Then only the following modes should be available: "<expected_modes>"

    Examples: AND Operations
      | permissionA | permissionB | expected_modes |
      | Mode0       | Mode0       | Mode0          |
      | Mode0       | AllModes    | Mode0          |
      | Mode1,Mode2 | Mode2,Mode3 | Mode2          |

  Scenario: Verify AND of non-overlapping modes results in no modes
    Given permission A is set to "Mode0"
    And permission B is set to "Mode1"
    When I compute A AND B
    Then no modes should be available

  Scenario Outline: Verify bitwise OR operations
    Given permission A is set to "<permissionA>"
    And permission B is set to "<permissionB>"
    When I compute A OR B
    Then only the following modes should be available: "<expected_modes>"

    Examples: OR Operations
      | permissionA | permissionB | expected_modes                 |
      | Mode0       | Mode0       | Mode0                          |
      | Mode0       | Mode1       | Mode0, Mode1                   |
      | Mode1,Mode2 | Mode2,Mode3 | Mode1, Mode2, Mode3            |
      | Mode0,Mode1 | Mode5,Mode6 | Mode0, Mode1, Mode5, Mode6     |

  Scenario Outline: Verify bitwise NOT operations
    Given permission is set to "<permission>"
    When I compute NOT
    Then the result should have all bits flipped in the uint8 representation

    Examples: NOT Operations
      | permission |
      | Mode0      |
      | Mode1      |
      | AllModes   |
      | None       |

  Scenario Outline: Verify HideWhenUnavailable visibility
    Given permission is set to "<permission>" with HideWhenUnavailable flag
    When I check in mode "<mode>"
    Then visibility should be "<visibility>"

    Examples: Visibility Tests
      | permission | mode  | visibility |
      | Mode0      | Mode0 | visible    |
      | Mode0      | Mode1 | hidden     |
      | Mode1      | Mode1 | visible    |
      | Mode1      | Mode2 | hidden     |
      | AllModes   | Mode0 | visible    |
      | AllModes   | Mode6 | visible    |
      | None       | Mode0 | hidden     |
      | None       | Mode6 | hidden     |
