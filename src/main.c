#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define term_capacity 65536

#ifdef __unix__
#include <termios.h>
struct t_term {
    struct termios old;
    struct termios new;
} term;
void term_exit() {
    tcsetattr(STDIN_FILENO, TCSANOW, &term.old);
}
size_t term_read(char* dst) {
    return read(STDIN_FILENO, dst, term_capacity);
}
void term_init() {
    tcgetattr(STDIN_FILENO, &term.old);
    term.new = term.old;
    term.new.c_lflag &= ~(ICANON | ECHO);
    term.new.c_cc[VMIN] = 0;
    term.new.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &term.new);
}
#endif
#ifdef _WIN32
#include <conio.h>
void term_exit(){};
size_t term_read(char* dst) {
    size_t i;
    for (i = 0; kbhit() && i < term_capacity; i++) {
        dst[i] = getch();
    }
    return i;
};
void term_init(){};
#endif

void init() {
    term_init();
}

int main() {
    init();
}