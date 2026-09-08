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

Feature: Command Results

  Scenario Outline: Result code handling with different statuses
    When I create a result with status "<status>" and code <code>
    Then the result succeeded status should be <shouldSucceed>

    Examples:
      | status              | code | shouldSucceed |
      | Ok                  | 0    | true          |
      | Ok                  | 1    | false         |
      | Ok                  | -1   | false         |
      | CommandNotFound     | 0    | false         |
      | CommandNotFound     | 1    | false         |
      | PermissionDenied    | 0    | false         |
      | PermissionDenied    | -1   | false         |
      | EmptyCommand        | -128 | false         |
