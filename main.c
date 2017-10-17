#include <stdio.h>
#include <unistd.h>
#include <termio.h>
#include <stdlib.h>
#include <ctype.h>

struct termios orig_termios;

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);

    //  Ensure we’ll leave the terminal attributes
    //  the way we found them when our program exits
    atexit(disableRawMode);

    struct termios raw = orig_termios;

    // Map bits to disable echo and canonical mode
    // turns off echoing in command line, and read byte by byte

    // See link below for flags disabling explanation
    // http://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html#turn-off-canonical-mode
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    //  The TCSAFLUSH argument specifies when to apply the change:
    //  in this case, it waits for all pending output to be written to the terminal,
    //  and also discards any input that hasn’t been read.

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main() {
    enableRawMode();
    char c;
    while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
        if(iscntrl(c)) {
            printf("%d\r\n", c);
        } else {
            printf("%d ('%c')\r\n", c, c);
        }
    };
    return 0;
}