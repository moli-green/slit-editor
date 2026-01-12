# AGENTS.md - Context & Directive for AI Agents

This document defines the architectural philosophy, constraints, and future direction for `slit`.
Any AI agent (including Jules) working on this repository MUST read and adhere to these guidelines.

## 1. Project Identity & Philosophy

**`slit` is the "Keyhole" Text Editor for Unix.**

* **Metaphor:** Don't open the whole door just to change a lock.
* **Core Value:** `slit` edits a specific part of a file (or stream) *in-place*, preserving the surrounding terminal context (grep results, logs, previous commands).
* **Anti-Goals:**
    * DO NOT try to become Vim, Emacs, or Nano.
    * DO NOT implement full-screen scrolling or screen clearing.
    * DO NOT implement syntax highlighting or plugins (keep it minimal).
    * DO NOT embed AI/LLM logic inside the binary (slit is the *interface* for AI, not the AI itself).

## 2. Technical Constraints

* **Language:** C (C99 standard).
* **Dependencies:** strictly `libc` only. No `ncurses`, no external libraries.
* **Build:** Single `Makefile`. The output binary must be standalone and portable.
* **Memory Management:** Use dynamic allocation (`realloc`) for rows. Handle large lines gracefully, but assume the tool is for config files/scripts, not gigabyte-sized logs.

## 3. Architecture & The "Interactive Pipe"

The most critical architectural feature of `slit` is its ability to handle pipes interactively.

### The I/O Triad
Unlike standard editors, `slit` separates Data I/O from UI I/O.

1.  **STDIN (`0`):** Data Source.
    * If piped (`echo "foo" | slit`): Read content from here.
    * If TTY (`slit file`): Read keyboard input (Raw mode) *unless* `/dev/tty` is used.
2.  **STDOUT (`1`):** Data Sink.
    * If piped (`slit | sort`): Write final content here on exit.
    * If TTY: Used for UI rendering *unless* piped.
3.  **Control TTY (`/dev/tty`):** User Interface.
    * **CRITICAL:** When running in a pipe, `slit` MUST open `/dev/tty` to render the UI and capture keyboard events, bypassing stdin/stdout.

### Implementation Rule
* **NEVER** use `printf()` for UI rendering, as it targets `stdout`.
* ALWAYS use the wrapper function `tty_write()` (or equivalent) that targets the Control TTY for drawing ANSI escape sequences.

## 4. Current State (v0.6) & Known Behaviors

* **File Mode:** `slit filename` (Edits file in-place).
* **Pipe Mode:** `ls | slit` (Reads stdin, edits in memory, writes to stdout on exit).
* **TTY Detection:** If `filename` is NULL and `stdin` is a TTY, `slit` initializes an empty buffer (acts like a scratchpad).
* **Crash Safety:** Logic is in place to handle non-existent files by creating a dummy row to prevent segfaults during rendering.

## 5. Roadmap & Future Tasks

### Immediate Maintenance
* **Refactoring:** `slit.c` is becoming monolithic. Keep it readable. Functions are currently mixed (logic vs rendering).
* **Stability:** Ensure edge cases (window resizing `SIGWINCH`, extremely long lines, binary files) do not crash the shell.

### Allowed Feature Expansions
* **Jump to Line:** Implementing a way to jump to a specific line number internally (e.g., `Ctrl+G` -> input number).
* **Search:** Simple string search (low priority, keep UI minimal).
* **Undo/Redo:** Simple stack-based undo.

### Forbidden Expansions
* **Scripting Language:** Do not embed Lua/Python.
* **Networking:** Do not make it talk to the internet.

## 6. Communication Style

* The user (Shinya Koyano) and the Architect (Gemini) handle the high-level design.
* Your role (Jules) is to implement robust, error-free C code and maintain the repository health.
* When in doubt, prioritize **Simplicity** and **Speed** over features.

