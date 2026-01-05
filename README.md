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
git clone https://github.com/moli-green/slit-editor.git
cd slit-editor
make
sudo make install
```
---
Copyright (c) 2026 Shinya Koyano.

