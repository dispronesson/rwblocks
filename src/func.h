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
    char brand[32];
    char model[32];
    uint16_t hp;
} car_s;

void interface();
void print_records();
void parse_get(char* str);
void get_exist(uint64_t index);
void modify_fields(off_t offset, car_s* exist_car);
void modify_str(car_s* car, int op);
void modify_hp(car_s* car);
int car_equal(const car_s* a, const car_s* b);
void car_copy(car_s* dest, const car_s* src);
void int_handle(int signo);

#endif //FUNC_H