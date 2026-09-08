///
/// Copyright 2026 Chris Solomon
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// @file generate_report.cpp
/// @brief Renders an HTML test report from cwt-cucumber's `--report-json` output.
///
/// @details
/// Usage: `generate_report <input.json> <output.html>`
///
/// The input is a JSON array of "feature" objects, each with an `elements` array of
/// "scenario" objects, each with a `steps` array. This matches the shape cwt-cucumber
/// writes via `--report-json` (a `[{name, elements: [{name, steps: [{keyword, name,
/// result: {status, error_message?}}]}]}]}` document); every field this program reads is
/// looked up with a default (via `json::value()`), so an unexpected schema degrades to
/// blank/zero fields in the report rather than throwing.
///
/// @par Invariants & Safety Guarantees:
/// - **Escaping Invariant:** Every piece of text sourced from the input JSON and placed
///   into the HTML body (feature name, scenario name, step keyword, step name, step
///   error message) is passed through escape_html() before being written. Values this
///   program generates itself (counts, percentages, status literals, generated `id`
///   attributes) are not escaped, since they never contain user-controlled text.
/// - **Total Consistency Invariant:** For the Totals returned by summarize(),
///   `passed_scenarios + failed_scenarios == total_scenarios` and
///   `passed_steps + failed_steps + skipped_steps == total_steps` always hold, since
///   every scenario/step increments exactly one of the corresponding counters.
/// - **All-or-Nothing Output Invariant:** main() only writes to `output_file` after the
///   entire HTML document has been built in memory (`html.str()`), and only if the input
///   file was found and parsed as valid JSON. A malformed input or an unwritable output
///   path leaves no partial/truncated report file behind - the process exits non-zero
///   with a message on stderr instead.
/// - **Ordering Invariant:** Features and their scenarios/steps are rendered in the same
///   order they appear in the input JSON array; feature_idx (used to build each
///   `id="feature-N"` anchor) is assigned by iteration order, not by any field in the JSON.

#include <nlohmann/json.hpp>

#include <cctype>
#include <chrono>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using json = nlohmann::json;

namespace {

/// @brief Escapes the five HTML/XML special characters in @p text.
/// @details Replaces `&`, `<`, `>`, `"`, and `'` with their named entities
/// (`&amp;`, `&lt;`, `&gt;`, `&quot;`, `&#039;`); all other bytes pass through unchanged.
/// Applying this twice to already-escaped text will double-escape it (`&` becomes
/// `&amp;amp;`) - callers must call it exactly once per raw string, never on output
/// that has already been escaped.
/// @param text Raw, unescaped text (e.g. a scenario name straight from the input JSON).
/// @return A copy of @p text safe to place inside HTML element text content.
std::string escape_html(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (const char c : text) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&#039;"; break;
            default: out += c; break;
        }
    }
    return out;
}

/// @brief Formats the current local time for the report's footer.
/// @return The current local time as `YYYY-MM-DD HH:MM:SS`, e.g. "2026-09-08 11:31:04".
std::string current_timestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm{};
    localtime_r(&now_time, &local_tm);
    std::ostringstream oss;
    oss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

/// @brief Aggregate pass/fail counts across every feature in a report.
/// @details See the "Total Consistency Invariant" documented at file scope: the
/// scenario counts always sum to total_scenarios, and the step counts always sum to
/// total_steps.
struct Totals {
    int total_scenarios = 0;   ///< Number of scenarios across all features.
    int passed_scenarios = 0;  ///< Scenarios with zero failed steps.
    int failed_scenarios = 0;  ///< Scenarios with at least one failed step.
    int total_steps = 0;       ///< Number of steps across all scenarios.
    int passed_steps = 0;      ///< Steps whose `result.status` is `"passed"`.
    int failed_steps = 0;      ///< Steps whose `result.status` is `"failed"`.
    int skipped_steps = 0;     ///< Steps whose `result.status` is `"skipped"`.
};

/// @brief Determines whether a scenario passed, i.e. contains no failed step.
/// @details A scenario with zero steps is considered passed (vacuously true), matching
/// the same all-steps-must-fail-to-fail logic used by summarize(). A step whose status
/// is neither `"passed"` nor `"failed"` (e.g. `"skipped"` or `"undefined"`) does not by
/// itself fail the scenario - only an explicit `"failed"` status does.
/// @param scenario A single scenario object (one entry of a feature's `elements` array).
/// @return `false` if any step's `result.status` is `"failed"`, `true` otherwise.
bool scenario_passed(const json& scenario) {
    for (const auto& step : scenario.value("steps", json::array())) {
        if (step["result"].value("status", "") == "failed") {
            return false;
        }
    }
    return true;
}

/// @brief Walks every feature/scenario/step in @p features and tallies pass/fail counts.
/// @param features The top-level JSON array parsed from cwt-cucumber's `--report-json` output.
/// @return A Totals with every counter populated; see its @ref Totals "documented invariant".
Totals summarize(const json& features) {
    Totals totals;
    for (const auto& feature : features) {
        for (const auto& scenario : feature.value("elements", json::array())) {
            totals.total_scenarios++;
            bool passed = true;

            for (const auto& step : scenario.value("steps", json::array())) {
                totals.total_steps++;
                const std::string status = step["result"].value("status", "");
                if (status == "passed") {
                    totals.passed_steps++;
                } else if (status == "failed") {
                    totals.failed_steps++;
                    passed = false;
                } else if (status == "skipped") {
                    totals.skipped_steps++;
                }
            }

            if (passed) {
                totals.passed_scenarios++;
            } else {
                totals.failed_scenarios++;
            }
        }
    }
    return totals;
}

/// @brief Appends the report's `<style>` block to @p html.
/// @details The header, summary-card numbers, and pass-rate figure are all tinted with
/// one accent color chosen from @p all_passed (green if every step passed, red
/// otherwise) - everything else (status pills, step borders) uses its own fixed
/// per-status color regardless of the overall result.
/// @param[in,out] html The in-progress document; the style block is appended, nothing
/// already written is modified.
/// @param all_passed Whether every step in the run passed (see Totals::failed_steps).
void write_styles(std::ostringstream& html, const bool all_passed) {
    const std::string accent = all_passed ? "#4caf50" : "#f44336";
    html << R"(  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
      background: #f5f5f5;
      color: #333;
      line-height: 1.6;
    }
    .container { max-width: 1200px; margin: 0 auto; padding: 20px; }
    .header {
      background: )" << accent << R"(;
      color: white;
      padding: 30px;
      border-radius: 8px;
      margin-bottom: 30px;
    }
    .header h1 { font-size: 2em; margin-bottom: 10px; }
    .summary {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
      gap: 20px;
      margin-top: 20px;
    }
    .summary-card {
      background: rgba(255, 255, 255, 0.95);
      padding: 20px;
      border-radius: 8px;
      text-align: center;
    }
    .summary-card .number {
      font-size: 2.5em;
      font-weight: bold;
      color: )" << accent << R"(;
      margin-bottom: 10px;
    }
    .summary-card .label {
      font-size: 0.9em;
      color: #666;
      text-transform: uppercase;
      letter-spacing: 0.5px;
    }
    .pass-rate {
      background: white;
      padding: 20px;
      border-radius: 8px;
      margin-bottom: 30px;
      text-align: center;
    }
    .pass-rate .number {
      font-size: 3em;
      font-weight: bold;
      color: )" << accent << R"(;
    }
    .features-section {
      background: white;
      border-radius: 8px;
      overflow: hidden;
      box-shadow: 0 2px 4px rgba(0,0,0,0.1);
    }
    .feature {
      border-bottom: 1px solid #eee;
      padding: 0;
    }
    .feature:last-child { border-bottom: none; }
    .feature-header {
      background: #f9f9f9;
      padding: 20px;
      cursor: pointer;
      user-select: none;
      display: flex;
      justify-content: space-between;
      align-items: center;
    }
    .feature-header:hover { background: #f0f0f0; }
    .feature-name { font-size: 1.1em; font-weight: 600; }
    .feature-stats {
      font-size: 0.85em;
      color: #666;
      margin-left: 20px;
    }
    .feature-content {
      display: none;
      padding: 20px;
      background: white;
    }
    .feature-content.open { display: block; }
    .scenario {
      margin-bottom: 30px;
      padding-bottom: 20px;
      border-bottom: 1px solid #eee;
    }
    .scenario:last-child {
      border-bottom: none;
      margin-bottom: 0;
      padding-bottom: 0;
    }
    .scenario-header {
      display: flex;
      align-items: center;
      gap: 10px;
      margin-bottom: 10px;
    }
    .scenario-name {
      font-weight: 600;
      font-size: 1em;
    }
    .scenario-status {
      display: inline-block;
      padding: 4px 12px;
      border-radius: 4px;
      font-size: 0.85em;
      font-weight: 600;
      text-transform: uppercase;
    }
    .status-passed { background: #c8e6c9; color: #2e7d32; }
    .status-failed { background: #ffcdd2; color: #c62828; }
    .status-skipped { background: #f0f0f0; color: #666; }
    .step {
      margin: 10px 0 10px 20px;
      padding: 10px;
      border-left: 3px solid #ddd;
      font-size: 0.95em;
      font-family: 'Monaco', 'Menlo', 'Ubuntu Mono', monospace;
    }
    .step.passed {
      border-left-color: #4caf50;
      background: #f1f8e9;
    }
    .step.failed {
      border-left-color: #f44336;
      background: #ffebee;
    }
    .step.skipped {
      border-left-color: #999;
      background: #f5f5f5;
      opacity: 0.7;
    }
    .step-keyword { font-weight: 600; color: #1976d2; }
    .step-text { color: #333; }
    .step-result {
      font-size: 0.85em;
      margin-top: 8px;
      padding: 8px;
      background: rgba(0,0,0,0.05);
      border-radius: 4px;
      color: #666;
    }
    .step-error {
      color: #c62828;
      font-weight: 600;
      margin-top: 5px;
    }
    .footer {
      text-align: center;
      padding: 20px;
      color: #999;
      font-size: 0.9em;
    }
  </style>
)";
}

/// @brief Appends one collapsible `<div class="feature">` section to @p html.
/// @details Renders the feature's header (name, "passed/total scenarios" count, and a
/// failure count if any) followed by every scenario and step nested inside it. All
/// user-supplied text (feature name, scenario names, step keywords/names/error messages)
/// is passed through escape_html() before being written; @p feature_idx is only used to
/// build a unique `id="feature-N"` anchor for the header's collapse/expand toggle and
/// carries no meaning from the input JSON itself.
/// @param[in,out] html The in-progress document; this feature's markup is appended.
/// @param feature One entry of the top-level features array, expected to have a `name`
/// string and an `elements` array of scenarios (both read with a safe default via
/// `json::value()` if missing).
/// @param feature_idx Zero-based position of @p feature in the iteration order used by
/// main() - not read from the JSON, so it is stable across runs only as long as the
/// input array's order doesn't change.
void write_feature(std::ostringstream& html, const json& feature, const int feature_idx) {
    const std::string feature_id = "feature-" + std::to_string(feature_idx);
    const auto elements = feature.value("elements", json::array());

    int passed_count = 0;
    for (const auto& scenario : elements) {
        if (scenario_passed(scenario)) {
            passed_count++;
        }
    }
    const int failed_count = static_cast<int>(elements.size()) - passed_count;

    html << "\n      <div class=\"feature\">\n"
         << "        <div class=\"feature-header\" onclick=\"document.getElementById('"
         << feature_id << "').classList.toggle('open')\">\n"
         << "          <div class=\"feature-name\">\U0001F4C4 "
         << escape_html(feature.value("name", "Feature")) << "</div>\n"
         << "          <div class=\"feature-stats\">\n"
         << "            " << passed_count << "/" << elements.size() << " scenarios\n";
    if (failed_count > 0) {
        html << "            <span style=\"color: #f44336;\"> (" << failed_count
             << " failed)</span>\n";
    }
    html << "          </div>\n"
         << "        </div>\n"
         << "        <div class=\"feature-content\" id=\"" << feature_id << "\">";

    for (const auto& scenario : elements) {
        const bool passed = scenario_passed(scenario);
        const std::string status_class = passed ? "passed" : "failed";
        const std::string status_text = passed ? "PASSED" : "FAILED";

        html << "\n          <div class=\"scenario\">\n"
             << "            <div class=\"scenario-header\">\n"
             << "              <span class=\"scenario-name\">"
             << escape_html(scenario.value("name", "")) << "</span>\n"
             << "              <span class=\"scenario-status status-" << status_class
             << "\">" << status_text << "</span>\n"
             << "            </div>";

        for (const auto& step : scenario.value("steps", json::array())) {
            const std::string status = step["result"].value("status", "");
            const std::string step_class = status == "passed" || status == "failed" ? status : "skipped";
            std::string status_text_cap = status;
            if (!status_text_cap.empty()) {
                status_text_cap[0] = static_cast<char>(std::toupper(status_text_cap[0]));
            }

            html << "\n            <div class=\"step " << step_class << "\">\n"
                 << "              <span class=\"step-keyword\">" << escape_html(step.value("keyword", ""))
                 << "</span>\n"
                 << "              <span class=\"step-text\">" << escape_html(step.value("name", ""))
                 << "</span>\n"
                 << "              <div class=\"step-result\">[" << status_text_cap << "]</div>";

            if (step["result"].contains("error_message")) {
                html << "<div class=\"step-error\">"
                     << escape_html(step["result"].value("error_message", "")) << "</div>";
            }

            html << "</div>";
        }

        html << "</div>";
    }

    html << "</div></div>";
}

}  // namespace

/// @brief Entry point: reads a cwt-cucumber JSON report and writes an HTML report.
/// @details
/// Exits non-zero (with a message on stderr, no output file written - see the
/// "All-or-Nothing Output Invariant" documented at file scope) when:
/// - `argc != 3` (wrong number of arguments).
/// - `argv[1]` cannot be opened for reading.
/// - `argv[1]`'s contents are not valid JSON.
/// - `argv[2]` cannot be opened for writing (e.g. its parent directory doesn't exist).
///
/// On success, prints a one-line confirmation to stdout and returns 0.
/// @param argc Argument count; must be exactly 3 (program name, input path, output path).
/// @param argv `argv[1]` is the input JSON path, `argv[2]` is the output HTML path.
/// @return 0 on success, 1 on any of the failure conditions listed above.
int main(const int argc, const char* const argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: generate_report <input.json> <output.html>\n";
        return 1;
    }

    const std::string input_file = argv[1];
    const std::string output_file = argv[2];

    std::ifstream input(input_file);
    if (!input) {
        std::cerr << "Error: Input file not found: " << input_file << "\n";
        return 1;
    }

    json features;
    try {
        input >> features;
    } catch (const json::parse_error& e) {
        std::cerr << "Error generating report: " << e.what() << "\n";
        return 1;
    }

    const Totals totals = summarize(features);
    const int pass_rate = totals.total_steps > 0
        ? static_cast<int>(std::lround(100.0 * totals.passed_steps / totals.total_steps))
        : 0;
    const bool all_passed = totals.failed_steps == 0;

    std::ostringstream html;
    html << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n"
         << "  <meta charset=\"UTF-8\">\n"
         << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
         << "  <title>lish Test Report</title>\n";
    write_styles(html, all_passed);
    html << "</head>\n<body>\n  <div class=\"container\">\n"
         << "    <div class=\"header\">\n"
         << "      <h1>\U0001F9EA lish Test Report</h1>\n"
         << "      <p>Build and test results</p>\n\n"
         << "      <div class=\"summary\">\n"
         << "        <div class=\"summary-card\"><div class=\"number\">" << totals.total_scenarios
         << "</div><div class=\"label\">Scenarios</div></div>\n"
         << "        <div class=\"summary-card\"><div class=\"number\">" << totals.passed_scenarios
         << "</div><div class=\"label\">Passed</div></div>\n"
         << "        <div class=\"summary-card\"><div class=\"number\">" << totals.failed_scenarios
         << "</div><div class=\"label\">Failed</div></div>\n"
         << "        <div class=\"summary-card\"><div class=\"number\">" << totals.total_steps
         << "</div><div class=\"label\">Total Steps</div></div>\n"
         << "      </div>\n"
         << "    </div>\n\n"
         << "    <div class=\"pass-rate\">\n"
         << "      <div class=\"number\">" << pass_rate << "%</div>\n"
         << "      <div class=\"label\">Pass Rate (" << totals.passed_steps << "/" << totals.total_steps
         << " steps)</div>\n"
         << "    </div>\n\n"
         << "    <div class=\"features-section\">";

    int feature_idx = 0;
    for (const auto& feature : features) {
        write_feature(html, feature, feature_idx++);
    }

    html << "\n    </div>\n\n"
         << "    <div class=\"footer\">\n"
         << "      <p>Generated on " << current_timestamp() << "</p>\n"
         << "    </div>\n  </div>\n\n"
         << "  <script>\n"
         << "    // Auto-open failed feature sections\n"
         << "    document.querySelectorAll('.feature-header').forEach(header => {\n"
         << "      const content = header.nextElementSibling;\n"
         << "      if (content.textContent.includes('FAILED')) {\n"
         << "        content.classList.add('open');\n"
         << "      }\n"
         << "    });\n"
         << "  </script>\n</body>\n</html>";

    std::ofstream output(output_file);
    if (!output) {
        std::cerr << "Error: could not open output file: " << output_file << "\n";
        return 1;
    }
    output << html.str();

    std::cout << "✅ HTML report generated: " << output_file << "\n";
    return 0;
}
