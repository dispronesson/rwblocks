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
        else if (strncmp(buf, "GET", 3) == 0) parse_get(buf + 3);
        else if (strcmp(buf, "EXIT") == 0) return;
        else if (strcmp(buf, "\0") == 0);
        else fprintf(stderr, "error: %s: command not found...\n", buf);
    }
}

void print_records() {
    car_s car[10];

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

    if (pread(fd, car, sizeof(car), 0) != sizeof(car)) {
        perror("error: read");
        close(fd);
        exit(EXIT_FAILURE);
    }

    fl.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &fl);

    for (int i = 0; i < 10; i++) {
        printf("%d. %s %s, %u hp\n", i + 1, car[i].brand, car[i].model, car[i].hp);
    }
}

void parse_get(char* str) {
    if (*str == '\0') {
        fprintf(stderr, "error: GET: missing operand\n");
        return;
    }

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
    if (index == 0 || index > 10) {
        fprintf(stderr, "error: GET: incorrect index\n");
        return;
    }

    get_exist(index);
}

void get_exist(uint64_t index) {
    car_s exist_car;
    off_t offset = (index - 1) * sizeof(car_s);

    struct flock fl = {
        .l_type = F_RDLCK,
        .l_whence = SEEK_SET,
        .l_start = offset,
        .l_len = sizeof(car_s)
    };

    if (fcntl(fd, F_SETLKW, &fl) == -1) {
        perror("error: fcntl");
        close(fd);
        exit(EXIT_FAILURE);
    }

    size_t r = pread(fd, &exist_car, sizeof(car_s), offset);
    if (r != sizeof(car_s)) {
        perror("error: pread");
        close(fd);
        exit(EXIT_FAILURE);
    }

    fl.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &fl);

    modify_fields(offset, &exist_car);
}

void modify_fields(off_t offset, car_s* exist_car) {
    car_s car_cpy;
    memset(&car_cpy, 0, sizeof(car_s));
    car_copy(&car_cpy, exist_car);

    while (1) {
        printf("Original record:\n");
        printf("\"%s %s, %u hp\"\n", exist_car->brand, exist_car->model, exist_car->hp);

        if (!car_equal(&car_cpy, exist_car)) {
            printf("Edits in record:\n");
            printf("\"%s %s, %u hp\"\n", car_cpy.brand, car_cpy.model, car_cpy.hp);
        }

        printf("1. Change brand\n");
        printf("2. Change model\n");
        printf("3. Change horsepower value\n");
        printf("4. Save changes\n");
        printf("0. Cancel\n");

        char ch;

        int flag = 1;
        while (flag == 1) {
            tcsetattr(STDIN_FILENO, TCSANOW, &newt);
            ch = getchar();
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

            switch (ch) {
                case '1':
                    modify_str(&car_cpy, 1);
                    flag = 0;
                    break;
                case '2':
                    modify_str(&car_cpy, 0);
                    flag = 0;
                    break;
                case '3':
                    modify_hp(&car_cpy);
                    flag = 0;
                    break;
                case '4':
                    flag = 2;
                    break;
                case '0':
                    printf("Cancel...\n");
                    return;
                case '\n':
                    break;
                default:
                    fprintf(stderr, "error: invalid choice\n");
                    break;
            }
        }

        if (flag == 2) {
            if (!car_equal(&car_cpy, exist_car)) {
                struct flock fl = {
                    .l_type = F_WRLCK,
                    .l_whence = SEEK_SET,
                    .l_start = offset,
                    .l_len = sizeof(car_s)
                };

                if (fcntl(fd, F_SETLKW, &fl) == -1) {
                    perror("error: fcntl");
                    close(fd);
                    exit(EXIT_FAILURE);
                }

                car_s car_new;
                size_t r = pread(fd, &car_new, sizeof(car_s), offset);
                if (r != sizeof(car_s)) {
                    perror("error: pread");
                    close(fd);
                    exit(EXIT_FAILURE);
                }

                if (car_equal(exist_car, &car_new)) {
                    size_t w = pwrite(fd, &car_cpy, sizeof(car_s), offset);
                    if (w != sizeof(car_s)) {
                        perror("error: pwrite");
                        close(fd);
                        exit(EXIT_FAILURE);
                    }

                    fl.l_type = F_UNLCK;
                    fcntl(fd, F_SETLK, &fl);

                    printf("Record has been updated successfully!\n");
                    return;
                }
                else {
                    fl.l_type = F_UNLCK;
                    fcntl(fd, F_SETLK, &fl);
                    printf("Changes of original record by other process has been deteced! Read a new record...\n");
                    car_copy(exist_car, &car_new);
                }
            }
            else {
                printf("No changes\n");
                return;
            }
        }
    }
}

void modify_str(car_s* car, int op) {
    char buf[32];
    printf("Enter %s: ", op ? "a new brand" : "a new model");
    fgets(buf, sizeof(buf), stdin);

    char* pos = strchr(buf, '\n');
    if (pos) *pos = '\0';
    else while (getchar() != '\n');

    if (op) strcpy(car->brand, buf);
    else strcpy(car->model, buf);
}

void modify_hp(car_s* car) {
    int32_t hp;
    printf("Enter a new horsepower value: ");

    while (1) {
        if (scanf("%d", &hp) != 1) {
            fprintf(stderr, "error: Invalid input: Try again: ");
        }
        else if (hp <= 0 || hp > 10000) {
            fprintf(stderr, "error: Invalid value: Try again: ");
        }
        else break;
        while (getchar() != '\n');
    }
    while (getchar() != '\n');

    car->hp = (uint16_t)hp;
}

int car_equal(const car_s* a, const car_s* b) {
    return strcmp(a->brand, b->brand) == 0 &&
           strcmp(a->model, b->model) == 0 &&
           a->hp == b->hp;
}

void car_copy(car_s* dest, const car_s* src) {
    strcpy(dest->brand, src->brand);
    strcpy(dest->model, src->model);
    dest->hp = src->hp;
}

void int_handle(int signo) {
    close(fd);
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    write(STDOUT_FILENO, "\n", 1);
    exit(SIGINT);
}