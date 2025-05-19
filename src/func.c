#include "func.h"

struct termios oldt, newt;
int fd;

void interface() {
    char buf[32];
    
    while (1) {
        fgets(buf, sizeof(buf), stdin);

        char* pos = strchr(buf, '\n');
        if (pos) *pos = '\0';
        else while (getchar() != '\n');

        if (strcmp(buf, "LST") == 0) print_records();
        else if (strncmp(buf, "GET ", 4) == 0) parse_get(buf + 4);
        else if (strcmp(buf, "EXIT") == 0) return;
        else if (strcmp(buf, "\0") == 0);
        else fprintf(stderr, "error: %s: command not found...\n", buf);
    }
}

void print_records() {
    book_s book[10];

    struct flock fl = {
        .l_type = F_RDLCK,
        .l_whence = SEEK_SET,
        .l_start = 0,
        .l_len = 0
    };

    if (fcntl(fd, F_SETLKW, &fl) == -1) {
        perror("error: fcntl");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (pread(fd, book, sizeof(book), 0) != sizeof(book)) {
        perror("error: read");
        close(fd);
        exit(EXIT_FAILURE);
    }

    fl.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &fl);

    for (int i = 0; i < 10; i++) {
        printf("%d. «%s» (%u) — %s\n", i + 1, book[i].title, book[i].year, book[i].author);
    }
}

void parse_get(char* str) {
    if (*str < '0' || *str > '9') {
        fprintf(stderr, "error: GET: invalid operand\n");
        return;
    }

    errno = 0;
    char* end;
    uint64_t index = strtoull(str, &end, 10);

    if (*end != '\0') {
        fprintf(stderr, "error: GET: not valid number\n");
        return;
    }
    if (errno == ERANGE) {
        fprintf(stderr, "error: GET: overflow\n");
        return;
    }
    if (index > 10) {
        fprintf(stderr, "error: GET: incorrect index\n");
        return;
    }

    get_exist(index);
}

void get_exist(uint64_t index) {
    book_s exist_book;

    struct flock fl = {
        .l_type = F_RDLCK,
        .l_whence = SEEK_SET,
        .l_start = (index - 1) * sizeof(book_s),
        .l_len = sizeof(book_s)
    };

    if (fcntl(fd, F_SETLKW, &fl) == -1) {
        perror("error: fcntl");
        close(fd);
        exit(EXIT_FAILURE);
    }

    size_t r = pread(fd, &exist_book, sizeof(book_s), (index - 1) * sizeof(book_s));
    if (r != sizeof(book_s)) {
        perror("error: pread");
        close(fd);
        exit(EXIT_FAILURE);
    }

    fl.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &fl);

    modify_fields(index, &exist_book);
}

void modify_fields(uint64_t index, book_s* exist_book) {
    book_s book_cpy = {0};
    book_copy(&book_cpy, exist_book);

    while (1) {
        printf("Исходная запись:\n");
        printf("«%s» (%u) — %s\n", exist_book->title, exist_book->year, exist_book->author);

        if (!book_equal(&book_cpy, exist_book)) {
            printf("Правки в записи:\n");
            printf("«%s» (%u) — %s\n", book_cpy.title, book_cpy.year, book_cpy.author);
        }

        printf("1. Изменить название\n");
        printf("2. Изменить автора\n");
        printf("3. Изменить год написания\n");
        printf("4. Сохранить изменения\n");
        printf("0. Назад\n");

        char ch;

        int flag = 1;
        while (flag == 1) {
            tcsetattr(STDIN_FILENO, TCSANOW, &newt);
            ch = getchar();
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

            switch (ch) {
                case '1':
                    modify_str(&book_cpy, 1);
                    flag = 0;
                    break;
                case '2':
                    modify_str(&book_cpy, 0);
                    flag = 0;
                    break;
                case '3':
                    modify_year(&book_cpy);
                    flag = 0;
                    break;
                case '4':
                    flag = 2;
                    break;
                case '0':
                    printf("Отмена...\n");
                    return;
                case '\n':
                    break;
                default:
                    fprintf(stderr, "error: invalid choice\n");
                    break;
            }
        }

        if (flag == 2) {
            if (!book_equal(&book_cpy, exist_book)) {
                struct flock fl = {
                    .l_type = F_WRLCK,
                    .l_whence = SEEK_SET,
                    .l_start = (index - 1) * sizeof(book_s),
                    .l_len = sizeof(book_s)
                };

                if (fcntl(fd, F_SETLKW, &fl) == -1) {
                    perror("error: fcntl");
                    close(fd);
                    exit(EXIT_FAILURE);
                }

                book_s book_new;
                size_t r = pread(fd, &book_new, sizeof(book_s), (index - 1) * sizeof(book_s));
                if (r != sizeof(book_s)) {
                    perror("error: pread");
                    close(fd);
                    exit(EXIT_FAILURE);
                }

                if (book_equal(exist_book, &book_new)) {
                    size_t w = pwrite(fd, &book_cpy, sizeof(book_s), (index - 1) * sizeof(book_s));
                    if (w != sizeof(book_s)) {
                        perror("error: pwrite");
                        close(fd);
                        exit(EXIT_FAILURE);
                    }

                    fl.l_type = F_UNLCK;
                    fcntl(fd, F_SETLK, &fl);

                    printf("Запись успешно обновлена!\n");
                    return;
                }
                else {
                    fl.l_type = F_UNLCK;
                    fcntl(fd, F_SETLK, &fl);
                    printf("Обнаружено изменение исходной записи другим процессом! Перечитываю...\n");
                    book_copy(exist_book, &book_new);
                }
            }
            else {
                printf("Изменений не обнаружено\n");
                return;
            }
        }
    }
}

void modify_str(book_s* book, int op) {
    char buf[128];
    printf("Введите %s (макс. 64 символа): ", op ? "новое название" : "нового автора");
    fgets(buf, sizeof(buf), stdin);

    char* pos = strchr(buf, '\n');
    if (pos) *pos = '\0';
    else while (getchar() != '\n');

    if (op) {
        memset(book->title, 0, sizeof(book->title));
        strcpy(book->title, buf);
    }
    else {
        memset(book->author, 0, sizeof(book->author));
        strcpy(book->author, buf);
    }
}

void modify_year(book_s* book) {
    int32_t year;
    printf("Введите новый год написания: ");

    while (1) {
        if (scanf("%d", &year) != 1) {
            fprintf(stderr, "Неверный ввод. Повторите попытку: ");
            while (getchar() != '\n');
        }
        else if (year <= 0 || year > 2100) {
            fprintf(stderr, "Неверное значение года. Повторите попытку: ");
        }
        else {
            while (getchar() != '\n');
            break;
        }
    }

    book->year = (uint16_t)year;
}

int book_equal(const book_s* a, const book_s* b) {
    return strcmp(a->title, b->title) == 0 &&
           strcmp(a->author, b->author) == 0 &&
           a->year == b->year;
}

void book_copy(book_s* dest, const book_s* src) {
    strcpy(dest->title, src->title);
    strcpy(dest->author, src->author);
    dest->year = src->year;
}

void int_handle(int signo) {
    close(fd);
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    write(STDOUT_FILENO, "\n", 1);
    exit(SIGINT);
}