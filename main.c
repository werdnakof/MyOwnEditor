#include <stdio.h>
#include <unistd.h>
#include <termio.h>
#include <stdlib.h>
#include <ctype.h>
#include <asm/errno.h>
#include <errno.h>

struct termios orig_termios;

void die(const char *s) {
    perror(s);
    exit(1);
}

void disableRawMode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
        die("tcsetattr");
}

void enableRawMode() {
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) die("tcgetattr");

    //  Ensure we’ll leave the terminal attributes
    //  the way we found them when our program exits
    atexit(disableRawMode);

    struct termios raw = orig_termios;

    /* See links below for flags disabling explanation
     * https://en.wikibooks.org/wiki/Serial_Programming/termios#Introduction
     * https://blog.nelhage.com/2009/12/a-brief-introduction-to-termios-termios3-and-stty/
     * /

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

int main() {
    enableRawMode();

    while (1) {
        char c = '\0';
        if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN) die("read");
        read(STDIN_FILENO, &c, 1);
        if (iscntrl(c)) {
            printf("%d\r\n", c);
        } else {
            printf("%d ('%c')\r\n", c, c);
        }
        if (c == 'q') break;
    }
    return 0;
}