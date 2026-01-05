/*** slit.c - One Line Editor (v0.3 - Fixed ESC Lag) ***/
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

#define OLE_VERSION "0.3.0"

enum editorKey {
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN
};

typedef struct erow {
    int size;   
    int len;    
    char *chars;
} erow;

struct editorConfig {
    int cx, cy;     
    int numrows;    
    erow *row;      
    char *filename; 
    struct termios orig_termios;
};

struct editorConfig E;

void die(const char *s) {
    write(STDOUT_FILENO, "\r\n", 2);
    perror(s);
    exit(1);
}

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios);
}

void enableRawMode() {
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
    atexit(disableRawMode);

    struct termios raw = E.orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    
    // readのタイムアウト設定 (VMIN=1 だと1バイト来るまで無限に待つ)
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

int is_utf8_continuation(char c) {
    return (c & 0xC0) == 0x80;
}

void editorAppendRow(char *s, size_t len) {
    E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
    int at = E.numrows;
    E.row[at].len = len;
    E.row[at].size = len + 1; 
    E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0';
    E.numrows++;
}

void editorRowInsertChar(erow *row, int at, int c) {
    if (at < 0 || at > row->len) at = row->len;
    row->chars = realloc(row->chars, row->len + 2);
    memmove(&row->chars[at + 1], &row->chars[at], row->len - at + 1);
    row->len++;
    row->chars[at] = c;
    E.cx++;
}

void editorRowDelChar(erow *row, int at) {
    if (at < 0 || at >= row->len) return;
    memmove(&row->chars[at], &row->chars[at + 1], row->len - at);
    row->len--;
}

void editorInsertChar(int c) {
    if (E.cy == E.numrows) {
        editorAppendRow("", 0);
    }
    editorRowInsertChar(&E.row[E.cy], E.cx, c);
}

void editorBackspace() {
    erow *row = &E.row[E.cy];
    if (E.cx == 0 && E.cy == 0) return;

    if (E.cx > 0) {
        int delete_len = 0;
        int pos = E.cx;
        do {
            pos--;
            delete_len++;
        } while (pos > 0 && is_utf8_continuation(row->chars[pos]));

        for(int i=0; i<delete_len; i++) {
            editorRowDelChar(row, pos);
        }
        E.cx = pos;
    }
}

void editorOpen(char *filename) {
    free(E.filename);
    E.filename = strdup(filename);

    FILE *fp = fopen(filename, "r");
    if (!fp) return; 

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;

    while ((linelen = getline(&line, &linecap, fp)) != -1) {
        while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r'))
            linelen--;
        editorAppendRow(line, linelen);
    }
    free(line);
    fclose(fp);
}

void editorSave() {
    if (E.filename == NULL) return;
    FILE *fp = fopen(E.filename, "w");
    if (!fp) return;
    for (int i = 0; i < E.numrows; i++) {
        fprintf(fp, "%s\n", E.row[i].chars);
    }
    fclose(fp);
}

void editorMoveCursor(int key) {
    erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
    switch (key) {
        case ARROW_LEFT:
            if (E.cx > 0) {
                do { E.cx--; } while (E.cx > 0 && is_utf8_continuation(row->chars[E.cx]));
            }
            break;
        case ARROW_RIGHT:
            if (row && E.cx < row->len) {
                do { E.cx++; } while (E.cx < row->len && is_utf8_continuation(row->chars[E.cx]));
            }
            break;
        case ARROW_UP:
            if (E.cy > 0) {
                E.cy--;
                if (E.cx > E.row[E.cy].len) E.cx = E.row[E.cy].len;
            }
            break;
        case ARROW_DOWN:
            if (E.cy < E.numrows - 1) {
                E.cy++;
                if (E.cx > E.row[E.cy].len) E.cx = E.row[E.cy].len;
            }
            break;
    }
}

int editorReadKey() {
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }

    if (c == '\x1b') {
        // --- 修正箇所: ESCキーの即時判定ロジック ---
        int bytes_waiting;
        if (ioctl(STDIN_FILENO, FIONREAD, &bytes_waiting) == 0 && bytes_waiting == 0) {
            return '\x1b';
        }
        // ------------------------------------------

        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A': return ARROW_UP;
                case 'B': return ARROW_DOWN;
                case 'C': return ARROW_RIGHT;
                case 'D': return ARROW_LEFT;
            }
        }
        return '\x1b';
    }
    return c;
}

void editorRefreshLine() {
    if (E.cy >= E.numrows) {
         write(STDOUT_FILENO, "\r\x1b[K[EOF]", 8);
         write(STDOUT_FILENO, "\r", 1);
         return;
    }

    erow *row = &E.row[E.cy];
    char status[32];
    snprintf(status, sizeof(status), "[%d/%d] ", E.cy + 1, E.numrows);
    int status_len = strlen(status);

    char *buf = malloc(status_len + row->len + 32);
    // \x1b[?25l はカーソルを隠す、\x1b[?25h は表示する（チラつき軽減）
    sprintf(buf, "\r\x1b[?25l\x1b[K%s%s", status, row->chars);
    write(STDOUT_FILENO, buf, strlen(buf));
    free(buf);

    if (status_len + E.cx > 0) {
        char move_cursor[32];
        snprintf(move_cursor, sizeof(move_cursor), "\r\x1b[%dC", status_len + E.cx);
        write(STDOUT_FILENO, move_cursor, strlen(move_cursor));
    } else {
        write(STDOUT_FILENO, "\r", 1);
    }
    write(STDOUT_FILENO, "\x1b[?25h", 6); // カーソル再表示
}

void editorProcessKeypress() {
    int c = editorReadKey();
    switch (c) {
        case '\x1b': 
            editorSave();
            write(STDOUT_FILENO, "\r\n", 2);
            exit(0);
            break;
        case 127: 
            editorBackspace();
            break;
        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            editorMoveCursor(c);
            break;
        default:
            if (!iscntrl(c)) editorInsertChar(c);
            break;
    }
}

void initEditor() {
    E.cx = 0;
    E.cy = 0;
    E.numrows = 0;
    E.row = NULL;
    E.filename = NULL;
}

int main(int argc, char *argv[]) {
    enableRawMode();
    initEditor();

    char *filename = NULL;
    int start_line = 0;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '+') {
            start_line = atoi(&argv[i][1]);
            if (start_line > 0) start_line--; 
        } else {
            filename = argv[i];
        }
    }

    if (!filename) {
        write(STDOUT_FILENO, "Usage: slit [+line] filename\r\n", 30);
        exit(1);
    }

    editorOpen(filename);
    if (start_line > 0 && start_line < E.numrows) {
        E.cy = start_line;
    } else if (start_line >= E.numrows) {
        E.cy = E.numrows - 1;
    }

    while (1) {
        editorRefreshLine();
        editorProcessKeypress();
    }
    return 0;
}

