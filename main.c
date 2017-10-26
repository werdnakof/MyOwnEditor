/*** includes ***/

#include <stdio.h>
#include <unistd.h>
#include <termio.h>
#include <stdlib.h>
#include <asm/errno.h>
#include <errno.h>
#include <ctype.h>

/*** defines ***/

#define CTRL_KEY(k) ((k) & 0x1f)

/*** data ***/

struct editorConfig {
    int screenrows;
    int screencols;
    struct termios orig_termios;
};

struct editorConfig E;

/*** terminal ***/

void die(const char *s) {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    perror(s);
    exit(1);
}

void disableRawMode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
        die("tcsetattr");
}

void enableRawMode() {
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");

    //  Ensure we’ll leave the terminal attributes
    //  the way we found them when our program exits
    atexit(disableRawMode);

    struct termios raw = E.orig_termios;

    /*
     * See links below for flags disabling explanation
     * https://en.wikibooks.org/wiki/Serial_Programming/termios#Introduction
     * https://blog.nelhage.com/2009/12/a-brief-introduction-to-termios-termios3-and-stty/
     */

    /* Input specific flags (bitmask) */
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

    /* Output specific flags (bitmask) */
    raw.c_oflag &= ~(OPOST);

    /* Character processing flags (bitmask) */
    raw.c_cflag |= (CS8);

    /* Line processing flags (bitmask) */
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    /*
     * VMIN value sets the minimum number of bytes of input needed
     * before read() can return
     *
     * VTIME value sets the maximum amount of time to wait before read()
     */
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    //  The TCSAFLUSH argument specifies when to apply the change:
    //  In this case, it waits for all pending output to be written to the terminal,
    //  and also discards any input that hasn’t been read.

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

char editorReadKey() {
    int nread;
    char c;
    while ((nread = (int) read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }
    return c;
}

int getCursorPosition(int *rows, int * cols) {
    char buf[32];
    unsigned int i = 0;

    if(write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

    while(i < sizeof(buf) - 1) {
        if(read(STDIN_FILENO, &buf[i], 1) != 1) break;
        if(buf[i] == 'R') break;
        i++;
    }

    buf[i] = '\0'; // printf() expects strings to end with a 0 byte

    if(buf[0] != '\x1b' || buf[1] != '[') return -1;
    if(sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

    return 0;
}

int getWindowSize(int *rows, int *cols) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        return getCursorPosition(rows, cols);
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

struct abuf {
    char *b;
    int len;
};

#define ABUF_INIT {NULL, 0};

/*** output ***/

void editorDrawRows() {
    int y;
    for(y = 0; y < E.screenrows; y++) {
        write(STDOUT_FILENO, "~", 1);

        if(y < E.screenrows - 1) {
            write(STDOUT_FILENO, "\r\n", 2);
        }
    }
}

void editorRefreshScreen() {
    write(STDOUT_FILENO, "\x1b[2J", 4); // J means erase in display
    write(STDOUT_FILENO, "\x1b[H", 3); // reposition to row 1 column 1
    editorDrawRows();
    write(STDOUT_FILENO, "\x1b[H", 3);
}

void editorProcessKeypress() {
    char c = editorReadKey();

    switch (c) {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
        default:
            printf("%c \r\n", c);
    }
}

/*** init ***/

void initEditor() {
    if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}

int main() {
    enableRawMode();
    initEditor();

    while (1) {
        editorRefreshScreen();
        editorProcessKeypress();
    }
    return 0;
}