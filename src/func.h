#ifndef FUNC_H
#define FUNC_H

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <termios.h>
#include <signal.h>

typedef struct {
    char title[128];
    char author[128];
    uint16_t year;
} book_s;

void interface();
void print_records();
void parse_get(char* str);
void get_exist(uint64_t index);
void modify_fields(uint64_t index, book_s* exist_book);
void modify_str(book_s* book, int op);
void modify_year(book_s* book);
int book_equal(const book_s* a, const book_s* b);
void book_copy(book_s* dest, const book_s* src);
void int_handle(int signo);

#endif //FUNC_H