#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define term_capacity 65536
#define nodes_capacity 65536

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
    atexit(term_exit);
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
void term_exit() {};
size_t term_read(char* dst) {
    size_t i;
    for (i = 0; kbhit() && i < term_capacity; i++) {
        dst[i] = getch();
    }
    return i;
};
void term_init() {};
#endif

struct node {
    struct node* next;
    struct node* prev;
    char ch;
};

static struct node nodes[nodes_capacity];
static struct node* nodes_passive[nodes_capacity];
static struct node* nodes_target;
uint32_t nodes_passive_size = nodes_capacity;

void node_insert(char ch) {
    struct node* this = nodes_passive[--nodes_passive_size];
    struct node* next = nodes_target;
    struct node* prev = nodes_target->prev;
    this->ch = ch;
    this->next = nodes_target;
    this->prev = nodes_target->prev;
    next->prev = this;
    if (prev != NULL)
        prev->next = this;
}
void node_delete() {
    if (nodes_target->prev == NULL)
        return;
    struct node* this = nodes_target->prev;
    struct node* next = this->next;
    struct node* prev = this->prev;
    nodes_passive[nodes_passive_size++] = this;
    next->prev = prev;
    if (prev != NULL)
        prev->next = next;
}

void init() {
    term_init();
    printf("\e[>5h");

    for (uint32_t i = 0; i < nodes_capacity; i++)
        nodes_passive[i] = &nodes[i];
    nodes_target = &nodes[0];
}

void update_rendering_clear() {
    printf("\e[1;1H");
    for (uint32_t i = 0; i < 10; i++) {
        for (uint32_t j = 0; j < 64; j++)
            putchar(' ');
        putchar('\n');
    }
    printf("\e[1;1H");
}
void update_rendering() {
    update_rendering_clear();
    struct node* this = nodes_target;
    while (this->prev != NULL)
        this = this->prev;
    while (this != NULL) {
        if (this == nodes_target)
            putchar('|');
        putchar(this->ch);
        this = this->next;
    }
    fflush(stdout);
}
void update_input() {
    char input[term_capacity];
    uint32_t input_size = term_read(input);
    for (uint32_t i = 0; i < input_size; i++) {
        char ch = input[i];
        switch (ch) {
            case 27:
                ch = input[++i];
                if (ch == '[') {
                    ch = input[++i];
                    if (ch == 'D')
                        if (nodes_target->prev != NULL)
                            nodes_target = nodes_target->prev;
                    if (ch == 'C')
                        if (nodes_target->next != NULL)
                            nodes_target = nodes_target->next;
                }
                break;
            case '\b':
            case 127:
                node_delete();
                break;
            default:
                node_insert(ch);
                break;
        }
    }
}
void update() {
    update_input();
    update_rendering();
    usleep(1000);
}

int main() {
    init();
    while (1) {
        update();
    }
}