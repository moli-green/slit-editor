# slit

**The "Keyhole" Text Editor for Unix.**

> *Don't open the whole door just to change a lock.*

`slit` is a minimal, inline text editor that allows you to edit a specific line of a file without clearing your terminal screen or losing your command history context.

It adheres strictly to the Unix philosophy: **Do one thing and do it well.**

![Demo](demo.gif)

## Why slit?

Have you ever run `grep` to find an error, seen the line number, opened `vim`, and then realized you forgot the line number because the screen cleared?

* **Context Aware:** `slit` edits in-place. Your grep results, compilation logs, and previous commands stay visible right above the editor.
* **Safety First:** It only displays one line at a time. You can't accidentally delete huge chunks of code you can't see.
* **Lightweight:** Written in pure C. No dependencies, instant startup.

## Installation

### From Source

```bash
git clone https://github.com/ShinyaKoyano/slit-editor.git
cd slit-editor
make
sudo make install
```

## Usage

### Basic Usage (File Editing)

Specify the file you want to edit.

```bash
slit config.json
```

Start editing from a specific line (e.g., line 15):

```bash
slit +15 config.json
```

### The Interactive Pipe (Killer Feature)

`slit` can read from `stdin` and write to `stdout`. This allows you to manually intervene in a shell pipeline.

**Example 1: Filter interactively**
Grab the output of `ls`, remove unwanted lines manually, and pass it to `grep`.

```bash
ls -l | slit | grep "rw"
```

**Example 2: Manual Data Correction**
Inject `slit` into a data processing pipeline to fix glitches by hand.

```bash
cat raw_data.csv | slit | sort > sorted_data.csv
```

**Example 3: Quick Memo**
Start with a template and save to a file.

```bash
echo "TODO:" | slit >> todo.txt
```

### Controls

`slit` provides a notepad-like experience with intuitive keybindings.

* **Arrow Up / Down**: Traverse through the file (peep through the slit).
* **Arrow Left / Right**: Move cursor.
* **Type**: Insert text.
* **Enter**: Insert new line (split line).
* **Backspace**: Delete character (join lines if at start of line).
* **ESC**: Save and Quit (Pass data to stdout in pipe mode).

## Safety & Limitations

* **UTF-8 Support**: `slit` supports UTF-8 characters (e.g., Japanese), protecting multi-byte characters from corruption during cursor movement.
* **Line-based**: It is designed for tweaking configuration files or scripts. For writing a novel or refactoring a whole project, use `vim` or `emacs`.

## License

MIT License

---
Copyright (c) 2026 Shinya Koyano.

