#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "cpu.h"

int main(int argc, char *argv[]) {


    FILE *file = fopen("rom.gb", "rb");

    if (file == NULL) {
        perror("fopen");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    unsigned long size = ftell(file);
    rewind(file);

    uint8_t *data = malloc(size);
    if (data == NULL) {
        fclose(file);
        return 1;
    }

    size_t bytes_read = fread(data, 1, size, file);

    if (bytes_read != (size_t)size) {
        fprintf(stderr, "Failed to read entire file\n");
    }

    fclose(file);

    // printf("File size: %ld bytes\n", size);

    CPU cpu;
    reset(&cpu);

    for(int i = 0; i < 100; i++) {
        tick(&cpu, data);
    }

    free(data);

    return 0;
}