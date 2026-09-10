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

#ifndef LISH_SHELL_H
#define LISH_SHELL_H

#include <stddef.h>
#include <stdint.h>
#include "lishArgs.h"
#include "lishCmdDescriptor.h"
#include "lishCmdPermission.h"
#include "lishDefs.h"
#include "lishHelp.h"
#include "lishIShell.h"
#include "lishInputProcessor.h"
#include "lishResult.h"

namespace lish {

/// @brief Concrete shell: owns interactive editing, history, and command dispatch for a
///        fixed, compile-time-known command table.
///
/// @details
/// Shell is the library's main entry point (typically constructed via make_shell(), see
/// lish.h). It combines an IOAdapter, a LineBuffer/LineEditor/History-backed
/// InputProcessor for interactive editing, and a fixed array of CmdDescriptor pointers
/// for dispatch. Application code interacts with it primarily through the IShell
/// interface it implements - command handlers receive an IShell&, not a Shell&, so they
/// never depend on any of the template parameters below.
///
/// Two independent ways to dispatch a command line:
/// - **Interactive** (service() -> InputProcessor -> on_submit() -> dispatch_chain()):
///   builds the line character-by-character via the line editor, then on Enter dispatches
///   it, prints a status line, and clears the buffer for the next line.
/// - **Non-interactive** (run_line()): dispatches a caller-supplied line directly via
///   dispatch_chain(), without going through the line editor and without printing status.
///   Safe to call reentrantly from within a command handler (see cmd_sudo.cpp in
///   examples/BasicShell) - see IShell::run_line()'s own doc for how the result of a
///   reentrant call relates to the outer one.
///
/// @tparam IOAdapter I/O adapter type (must implement write(char) and
///         read(char&, uint16_t timeout_ms = 0) - see IShell::read() for the required
///         timeout semantics).
/// @tparam LineBufferType LineBuffer specialization backing the interactive line editor.
/// @tparam LineEditorType LineEditor specialization operating on LineBufferType.
/// @tparam HistoryType History specialization backing interactive command recall.
/// @tparam ContextType Application context type, passed to command handlers.
/// @tparam PromptCallbackType Unused type parameter kept for interface symmetry; the
///         actual prompt callback type is fixed to `void (*)(IShell&, ContextType&)`
///         (see PromptCallback below) regardless of what's supplied here.
/// @tparam NumCommands Number of entries in the cmd_ptrs array.
/// @tparam MaxLineLength Maximum interactive line length (LineBufferType's capacity).
///
/// ## Invariants
/// - `current_mode_` must always be a single active mode bit, never a combined mask -
///   see IShell's class-level Invariants (the same requirement applies here, since Shell
///   is what actually stores and checks it).
/// - A command handler's own return code and dispatch-level failures (e.g. a chain parse
///   error) are both surfaced through ShellStatus::Ok with a nonzero CommandResult::code,
///   not a distinct status - see lishResult.h's Invariants for why `code` must always be
///   considered alongside `status`.
/// - run_line() with a nonzero-length line that is entirely whitespace does NOT return
///   ShellStatus::EmptyCommand (unlike a zero-length line) - Args::parse() reduces it to
///   an empty command segment, invoke_slice() is never called, and dispatch_chain()
///   returns its untouched initial value of CommandResult(ShellStatus::Ok, 0). Only the
///   interactive path's is_blank() check (in on_submit()) treats whitespace-only content
///   as equivalent to empty.
template <
  typename IOAdapter,
  typename LineBufferType,
  typename LineEditorType,
  typename HistoryType,
  typename ContextType,
  typename PromptCallbackType,
  size_t NumCommands,
  uint8_t MaxLineLength = 64
>
class Shell : public IShell {
public:
  using PromptCallback = void (*)(IShell& shell, ContextType& ctx);

  /// @brief Constructs a Shell instance with command dispatcher and interactive editing.
  ///
  /// @param ctx Application context, later passed by reference to every command handler
  ///            and to prompt_cb. Shell stores a type-erased pointer to it (via void*)
  ///            and casts back to ContextType& when needed - ctx's lifetime must exceed
  ///            the Shell's.
  /// @param cmd_ptrs Array of NumCommands pointers to command descriptors; entries may be
  ///                 nullptr for an unregistered slot. Referenced, not copied - the array
  ///                 (and each non-null CmdDescriptor it points to) must outlive the Shell.
  /// @param prompt_cb Optional custom prompt callback, invoked by print_prompt() instead
  ///                  of writing the default prompt. Pass nullptr (the default) to use
  ///                  PROMPT_DEFAULT.
  /// @param initial_mode Starting permission mode. Must be a single active mode bit (see
  ///                     class-level Invariants).
  Shell(
    ContextType& ctx,
    const CmdDescriptor* const (&cmd_ptrs)[NumCommands],
    const PromptCallback prompt_cb = nullptr,
    const CmdPermission initial_mode = CmdPermission::Mode0
  )
    : ctx_(static_cast<void*>(&ctx)),
      io_(),
      cmds_(cmd_ptrs),
      prompt_cb_(prompt_cb),
      current_mode_(initial_mode),
      last_result_code_(0),
      line_buf_(),
      input_processor_(
        line_buf_,
        io_,
        cmd_ptrs,
        current_mode_,
        *this
      )
  {}

  /// @details Stores mode into current_mode_, which every subsequent dispatch's
  /// permission checks (invoke_slice()) read; see IShell::set_mode() for the single-
  /// mode-bit precondition.
  void set_mode(const CmdPermission mode) override {
    current_mode_ = mode;
  }

  /// @details Returns current_mode_ as last set by set_mode() or the constructor's
  /// initial_mode; see IShell::mode().
  CmdPermission mode() const override {
    return current_mode_;
  }

  /// @details Drains and dispatches all currently-available input via the interactive
  /// line editor (InputProcessor); see IShell::service() for the full contract. Each
  /// completed line runs through on_submit(), which dispatches it, prints its status, and
  /// resets the buffer for the next line - unlike run_line(), this path always produces
  /// visible output.
  void service() override {
    char c;
    while (io_.read(c)) {
      if (input_processor_.process_char(c)) {
        on_submit();
      }
    }
  }

  /// @details Non-interactive dispatch; see IShell::run_line() for the full contract,
  /// including reentrant-call behavior. A zero-length line short-circuits to
  /// ShellStatus::EmptyCommand without calling dispatch_chain() at all - but a
  /// whitespace-only nonzero-length line does NOT get the same treatment; see class-level
  /// Invariants.
  CommandResult run_line(char* const line, const uint8_t length) override {
    if (length == 0) {
      CommandResult result(ShellStatus::EmptyCommand, 0);
      last_result_code_ = result.code;
      return result;
    }

    CommandResult result = dispatch_chain(line, length);
    last_result_code_ = result.code;
    return result;
  }

  /// @details Returns last_result_code_, which both run_line() and on_submit() update
  /// (from their dispatch_chain() result's code) immediately before returning - see
  /// IShell::get_last_result_code() for the general contract, and class-level Invariants
  /// for what a nonzero code can mean even when the underlying status was Ok.
  int8_t get_last_result_code() const override {
    return last_result_code_;
  }

  /// @details Forwards directly to the underlying IOAdapter; see IShell::write().
  void write(const char c) override {
    io_.write(c);
  }

  /// @details Forwards directly to the underlying IOAdapter, timeout_ms included; see
  /// IShell::read() for the required timeout semantics. Distinct from service()'s own
  /// input loop, which calls io_.read(c) with no explicit timeout (IOAdapter's own
  /// single-argument default) rather than going through this override.
  bool read(char& c, const uint16_t timeout_ms = 0) override {
    return io_.read(c, timeout_ms);
  }

  /// @details Writes prompt_cb (if one was supplied at construction) or PROMPT_DEFAULT
  /// otherwise. Called by on_submit() after every dispatched line, and by InputProcessor
  /// during line redraws (tab completion, history browsing, exiting history navigation).
  void print_prompt() override {
    if (prompt_cb_) {
      prompt_cb_(*this, *static_cast<ContextType*>(ctx_));
    } else {
      write_flash_string(PROMPT_DEFAULT, io_);
    }
  }

private:
  /// @brief Parses and executes a full line as a chain of `;`/`&&`-separated commands.
  ///
  /// @details
  /// Command segments are parsed and executed incrementally, one at a time: each segment
  /// is parsed, then (if non-empty) immediately run via invoke_slice(), before the next
  /// segment is parsed. This is not a two-phase "parse the whole line, then run everything"
  /// design — commands earlier in the chain have already executed by the time a later
  /// segment is parsed.
  ///
  /// If a segment fails to parse (Args::Terminator::ParseError, e.g. an unclosed quote),
  /// the chain halts immediately at that point: the errored segment does not run, and
  /// nothing after it runs either. Segments that already executed earlier in the chain are
  /// unaffected. This is not "abort the whole line" so much as a natural consequence of the
  /// parser being unable to determine token boundaries from the point of failure onward —
  /// most acutely with an unclosed quote, where every character to end-of-line is ambiguous
  /// (it may have been intended to lie inside the quotes). There is no reliable way to guess
  /// where a "next command" would begin, so no attempt is made to resynchronize and continue.
  ///
  /// @return The last-executed segment's result, or CommandResult(ShellStatus::Ok, -1) for
  ///         a parse error (see lishResult.h's Invariants - a dispatch-level failure like
  ///         this is deliberately surfaced as a nonzero code, not a distinct status). If
  ///         the line's only segment is empty (e.g. whitespace-only), returns the untouched
  ///         initial value CommandResult(ShellStatus::Ok, 0) - see class-level Invariants.
  CommandResult dispatch_chain(char* const buf, const uint8_t length);

  /// @brief Looks up and executes a single, already-parsed command segment.
  ///
  /// @details
  /// Special-cases the built-in "help" command first (absent from cmds_ - see
  /// HELP_COMMAND_STR in lishDefs.h and lishHelp.h), then searches cmds_ by hash then
  /// name (see CmdDescriptor::matches_hash()/matches_name() - the hash is a fast
  /// pre-filter only, never sufficient alone; see lishHelp.h's Invariants for why). A
  /// hidden command that doesn't match is skipped exactly like a non-matching one -
  /// hidden and truly-absent commands are indistinguishable to the caller, both reporting
  /// ShellStatus::CommandNotFound.
  ///
  /// @param view The already-parsed command segment (see Args); command() must be
  ///             non-empty - dispatch_chain() only calls this when it is.
  ///
  /// @return ShellStatus::Ok with the invoked handler's own return code (help's or a
  ///         registered command's) on success; ShellStatus::PermissionDenied if the
  ///         command was found but is unavailable in the current mode; otherwise
  ///         ShellStatus::CommandNotFound. code is always 0 for the latter two.
  CommandResult invoke_slice(Args& view);

  /// @brief Prints a single status line summarizing a dispatch result.
  ///
  /// @details
  /// Writes "[OK]" for a fully successful result (see CommandResult::succeeded()),
  /// "[ERROR: Code N]" (N is result.code's plain decimal representation, no
  /// zero-padding, '-' prefixed if negative - e.g. "5", "-5", "42") for
  /// ShellStatus::Ok with a nonzero code, or "[ERROR: <message>]" for
  /// CommandNotFound/PermissionDenied. Called exactly once per submitted line by
  /// on_submit() - a chain of `;`/`&&`-separated commands prints one status line for the
  /// whole chain's final result, not once per command.
  ///
  /// @param result The result to summarize; only ever a value dispatch_chain() can
  ///               produce (see its own @return) - never ShellStatus::EmptyCommand, which
  ///               is exclusively produced by run_line(), a path that never calls this.
  void print_status(const CommandResult result);

  /// @brief Handles a completed interactive line: dispatch, status, reset, reprompt.
  ///
  /// @details
  /// Called by service() when InputProcessor::process_char() signals a line is ready
  /// (Enter was pressed). A blank line (LineBuffer::is_blank() - empty or whitespace-only)
  /// skips dispatch and status printing entirely; any other line is dispatched via
  /// dispatch_chain() and its result printed via print_status(). Either way, the line
  /// buffer is cleared and the prompt reprinted before returning, ready for the next line.
  void on_submit();

  void* ctx_;
  IOAdapter io_;
  const CmdDescriptor* const (&cmds_)[NumCommands];
  const PromptCallback prompt_cb_;
  CmdPermission current_mode_;
  int8_t last_result_code_;

  LineBufferType line_buf_;
  InputProcessor<IOAdapter, LineEditorType, HistoryType, void (*)(), NumCommands, MaxLineLength> input_processor_;
};

}  // namespace lish

#include "lishShell.ipp"

#endif  // LISH_SHELL_H