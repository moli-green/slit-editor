/*** slit.c - The Keyhole Text Editor (v0.5 - Pipe Support) ***/
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h> // open用
#include <errno.h>

#define SLIT_VERSION "0.5.0"

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
    char *filename; // NULLならパイプモード
    struct termios orig_termios;
    int tty_fd;     // ★制御用端末のファイルディスクリプタ
};

struct editorConfig E;

// --- 画面出力用ラッパー ---
// 画面制御コードは必ず /dev/tty に送る
void tty_write(const char *buf, size_t len) {
    if (E.tty_fd != -1) {
        write(E.tty_fd, buf, len);
    }
}

void die(const char *s) {
    tty_write("\r\n", 2);
    perror(s);
    exit(1);
}

void disableRawMode() {
    if (E.tty_fd != -1) {
        tcsetattr(E.tty_fd, TCSAFLUSH, &E.orig_termios);
    }
}

void enableRawMode() {
    // ★ 標準入力ではなく、制御端末(tty_fd)に対してRawモードを設定する
    if (tcgetattr(E.tty_fd, &E.orig_termios) == -1) die("tcgetattr");
    atexit(disableRawMode);

    struct termios raw = E.orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    
    if (tcsetattr(E.tty_fd, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

int is_utf8_continuation(char c) {
    return (c & 0xC0) == 0x80;
}

// --- 行操作 (v0.4と同じ) ---

void editorInsertRow(int at, char *s, size_t len) {
    if (at < 0 || at > E.numrows) return;
    E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
    memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.numrows - at));
    E.row[at].size = len + 1;
    E.row[at].len = len;
    E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0';
    E.numrows++;
}

void editorFreeRow(erow *row) {
    free(row->chars);
}

void editorDelRow(int at) {
    if (at < 0 || at >= E.numrows) return;
    editorFreeRow(&E.row[at]);
    memmove(&E.row[at], &E.row[at + 1], sizeof(erow) * (E.numrows - at - 1));
    E.numrows--;
}

void editorRowInsertChar(erow *row, int at, int c) {
    if (at < 0 || at > row->len) at = row->len;
    row->chars = realloc(row->chars, row->len + 2);
    memmove(&row->chars[at + 1], &row->chars[at], row->len - at + 1);
    row->len++;
    row->chars[at] = c;
    E.cx++;
}

void editorRowAppendString(erow *row, char *s, size_t len) {
    row->chars = realloc(row->chars, row->len + len + 1);
    memcpy(&row->chars[row->len], s, len);
    row->len += len;
    row->chars[row->len] = '\0';
}

void editorRowDelChar(erow *row, int at) {
    if (at < 0 || at >= row->len) return;
    memmove(&row->chars[at], &row->chars[at + 1], row->len - at);
    row->len--;
}

// --- エディタ操作 ---

void editorInsertChar(int c) {
    if (E.cy == E.numrows) {
        editorInsertRow(E.numrows, "", 0);
    }
    editorRowInsertChar(&E.row[E.cy], E.cx, c);
}

void editorInsertNewline() {
    if (E.cx == 0) {
        editorInsertRow(E.cy, "", 0);
    } else {
        erow *row = &E.row[E.cy];
        editorInsertRow(E.cy + 1, &row->chars[E.cx], row->len - E.cx);
        row = &E.row[E.cy];
        row->len = E.cx;
        row->chars[row->len] = '\0';
    }
    E.cy++;
    E.cx = 0;
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
        for(int i=0; i<delete_len; i++) editorRowDelChar(row, pos);
        E.cx = pos;
    } else {
        erow *prev_row = &E.row[E.cy - 1];
        E.cx = prev_row->len;
        editorRowAppendString(prev_row, row->chars, row->len);
        editorDelRow(E.cy);
        E.cy--;
    }
}

// --- 入出力 (Pipe対応) ---

// --- 修正版 editorOpen (v0.6) ---
void editorOpen(char *filename) {
    FILE *fp;
    
    // ★修正ポイント: ファイル名がなく、かつ標準入力が端末(TTY)の場合
    // パイプラインの先頭 (slit | cat) または単独起動 (slit) なので、
    // 入力を待たずに「空のドキュメント」として開始する。
    if (!filename && isatty(STDIN_FILENO)) {
        editorInsertRow(0, "", 0);
        return;
    }

    if (filename) {
        // ファイルモード
        free(E.filename);
        E.filename = strdup(filename);
        fp = fopen(filename, "r");
    } else {
        // パイプモード: 標準入力から読む
        // (isattyチェックを抜けたということは、ここはパイプからの入力)
        E.filename = NULL;
        fp = stdin;
    }

    if (!fp) {
        if (filename) editorInsertRow(0, "", 0); 
        return; 
    }

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;

    while ((linelen = getline(&line, &linecap, fp)) != -1) {
        while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r'))
            linelen--;
        editorInsertRow(E.numrows, line, linelen);
    }
    free(line);
    
    if (filename) fclose(fp);
    
    if (E.numrows == 0) editorInsertRow(0, "", 0);
}

void editorSave() {
    FILE *fp;
    
    if (E.filename) {
        // ファイルへ保存
        fp = fopen(E.filename, "w");
    } else {
        // パイプモード: 標準出力へ書き出す
        fp = stdout;
    }

    if (!fp) return;
    
    for (int i = 0; i < E.numrows; i++) {
        fprintf(fp, "%s\n", E.row[i].chars);
    }
    
    if (E.filename) fclose(fp);
    // stdoutの場合は閉じない（呼び出し元が閉じる）
}

// --- UI制御 ---

void editorMoveCursor(int key) {
    erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
    switch (key) {
        case ARROW_LEFT:
            if (E.cx > 0) {
                do { E.cx--; } while (E.cx > 0 && is_utf8_continuation(row->chars[E.cx]));
            } else if (E.cy > 0) {
                E.cy--;
                E.cx = E.row[E.cy].len;
            }
            break;
        case ARROW_RIGHT:
            if (row && E.cx < row->len) {
                do { E.cx++; } while (E.cx < row->len && is_utf8_continuation(row->chars[E.cx]));
            } else if (row && E.cx == row->len && E.cy < E.numrows - 1) {
                E.cy++;
                E.cx = 0;
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
    // ★キー入力は制御端末(tty_fd)から読む
    while ((nread = read(E.tty_fd, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }

    if (c == '\x1b') {
        int bytes_waiting;
        if (ioctl(E.tty_fd, FIONREAD, &bytes_waiting) == 0 && bytes_waiting == 0) {
            return '\x1b';
        }

        char seq[3];
        if (read(E.tty_fd, &seq[0], 1) != 1) return '\x1b';
        if (read(E.tty_fd, &seq[1], 1) != 1) return '\x1b';

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
         tty_write("\r\x1b[K[EOF]", 8);
         tty_write("\r", 1);
         return;
    }

    erow *row = &E.row[E.cy];
    char status[32];
    snprintf(status, sizeof(status), "[%d/%d] ", E.cy + 1, E.numrows);
    int status_len = strlen(status);

    char *buf = malloc(status_len + row->len + 32);
    // ★画面制御コード
    sprintf(buf, "\r\x1b[?25l\x1b[K%s%s", status, row->chars);
    tty_write(buf, strlen(buf));
    free(buf);

    if (status_len + E.cx > 0) {
        char move_cursor[32];
        snprintf(move_cursor, sizeof(move_cursor), "\r\x1b[%dC", status_len + E.cx);
        tty_write(move_cursor, strlen(move_cursor));
    } else {
        tty_write("\r", 1);
    }
    tty_write("\x1b[?25h", 6);
}

void editorProcessKeypress() {
    int c = editorReadKey();
    switch (c) {
        case '\x1b': 
            editorSave();
            tty_write("\r\n", 2);
            exit(0);
            break;
        case 127: 
            editorBackspace();
            break;
        case 13: 
            editorInsertNewline();
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
    E.tty_fd = -1;
}

int main(int argc, char *argv[]) {
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

    // ★ここでファイルかパイプかを判断し、/dev/tty を開く
    
    // データ読み込み
    editorOpen(filename); 

    // UI制御用の端末を開く
    // /dev/tty はカレントプロセスの制御端末を指す特殊ファイル
    E.tty_fd = open("/dev/tty", O_RDWR);
    if (E.tty_fd == -1) {
        // TTYが開けない（cron等）場合はエラー
        // ただしパイプとしてデータ加工だけするなら続行もアリだが、
        // インタラクティブツールなのでdieする
        fprintf(stderr, "slit: Cannot open /dev/tty. Interactive editing not possible.\n");
        exit(1);
    }

    enableRawMode();
    
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

