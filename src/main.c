#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#define term_capacity 65536
#define nodes_capacity 65536
#define file_capacity 65536
#define rendering_size_y 11
#define renderingbody_size_y 10

struct t_term {
    struct termios old;
    struct termios new;
} term;
struct node {
    struct node* next;
    struct node* prev;
    char ch;
};
enum mode {
    mode_insert,
    mode_normal,
};

static struct global {
    struct node nodes[nodes_capacity];
    struct node* nodes_passive[nodes_capacity];
    struct node* nodes_target;
    uint32_t nodes_passive_size;
    enum mode mode;
    uint32_t input_str4;
    char* file_path;
} g;

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

void node_insert(char ch) {
    struct node* this = g.nodes_passive[--g.nodes_passive_size];
    struct node* next = g.nodes_target;
    struct node* prev = g.nodes_target->prev;
    this->ch = ch;
    this->next = g.nodes_target;
    this->prev = g.nodes_target->prev;
    next->prev = this;
    if (prev != NULL) {
        prev->next = this;
    }
}
void node_delete() {
    if (g.nodes_target->prev == NULL) {
        return;
    }
    struct node* this = g.nodes_target->prev;
    struct node* next = this->next;
    struct node* prev = this->prev;
    g.nodes_passive[g.nodes_passive_size++] = this;
    next->prev = prev;
    if (prev != NULL) {
        prev->next = next;
    }
}

void file_open(const char* path) {
    char buf[file_capacity];
    FILE* fp = fopen(path, "r");
    uint32_t n = fread(buf, sizeof(char), file_capacity, fp);
    for (uint32_t i = 0; i < n; i++) {
        node_insert(buf[i]);
    }
    fclose(fp);
}
void file_save(const char* path) {
    char buf[file_capacity];
    uint32_t i;
    struct node* this = g.nodes_target;
    while (this->prev != NULL) {
        this = this->prev;
    }
    for (i = 0; this->next != NULL; i++) {
        buf[i] = this->ch;
        this = this->next;
    }
    FILE* fp = fopen(path, "w");
    fwrite(buf, sizeof(char), i, fp);
    fclose(fp);
}

void input_normal(uint32_t str) {
    switch (str % 256) {
        case 'h':
            if (g.nodes_target->prev != NULL) {
                g.nodes_target = g.nodes_target->prev;
            }
            break;
        case 'l':
            if (g.nodes_target->next != NULL) {
                g.nodes_target = g.nodes_target->next;
            }
            break;
        case 'j':
            break;
        case 'G':
            while (g.nodes_target->next != NULL) {
                g.nodes_target = g.nodes_target->next;
            }
            break;
        case 'x':
            input_normal('l');
            node_delete(127);
            break;
        case 's':
            file_save(g.file_path);
            break;
        case 'i':
            g.mode = mode_insert;
            break;
        case 'q':
            exit(0);
        default:
            break;
    }
    switch (str % 65536) {
        case 26471:  // gg
            while (g.nodes_target->prev != NULL) {
                g.nodes_target = g.nodes_target->prev;
            }
            break;
        default:
            break;
    }
}
void input_insert(uint32_t str) {
    switch (str % 256) {
        case 27:  // escape
            g.mode = mode_normal;
            break;
        case '\b':
        case 127:  // backspace
            node_delete();
            break;
        default:
            node_insert(str);
            break;
    }
    switch (str % 65536) {
        case 27242:  // jj
            node_delete();
            node_delete();
            g.mode = mode_normal;
            break;
        default:
            break;
    }
}
void input(uint32_t str) {
    if (g.mode == mode_insert) {
        input_insert(str);
    } else if (g.mode == mode_normal) {
        input_normal(str);
    }
}
void input_readterm() {
    char term_buf[term_capacity];
    uint32_t input_size = term_read(term_buf);
    for (uint32_t i = 0; i < input_size; i++) {
        g.input_str4 <<= 8;
        g.input_str4 += term_buf[i];
        input(g.input_str4);
    }
}

void rendering_clear() {
    printf("\e[1;1H");
    for (uint32_t i = 0; i < rendering_size_y; i++) {
        for (uint32_t j = 0; j < 64; j++) {
            putchar(' ');
        }
        putchar('\n');
    }
    printf("\e[1;1H");
}
void rendering_top() {
    if (g.mode == mode_insert) {
        printf("---[mode:insert]---\n");
    } else if (g.mode == mode_normal) {
        printf("---[mode:normal]---\n");
    }
}
void rendering_body() {
    struct node* this = g.nodes_target;
    uint32_t y = renderingbody_size_y - 3;
    while (1) {
        if (this->prev == NULL) {
            y = 0;
            break;
        }
        if (y == 0 && this->prev->ch == '\n') {
            break;
        }
        if (this->ch == '\n' && this != g.nodes_target) {
            y--;
        }
        this = this->prev;
    }
    while (1) {
        if (this == NULL) {
            break;
        }
        if (this->ch == '\n') {
            y++;
        }
        if (y == renderingbody_size_y) {
            break;
        }
        if (this == g.nodes_target) {
            putchar('|');
        }
        putchar(this->ch);
        this = this->next;
    }
}
void rendering() {
    rendering_clear();
    rendering_top();
    rendering_body();
    fflush(stdout);
}

void update() {
    input_readterm();
    rendering();
    usleep(1000);
}
void init() {
    term_init();
    printf("\e[>5h");

    g.nodes_passive_size = nodes_capacity;
    for (uint32_t i = 0; i < nodes_capacity; i++) {
        g.nodes_passive[i] = &g.nodes[i];
    }
    g.nodes_target = &g.nodes[0];
    g.mode = mode_insert;
}

int main(int argc, char* argv[]) {
    init();

    if (argc == 2) {
        g.file_path = argv[1];
        file_open(g.file_path);
    }

    while (1) {
        update();
    }
}