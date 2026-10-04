#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "cpu.h"
#include "bus.h"

int main(int argc, char *argv[]) {

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <rom.gb>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "rb");

    if (file == NULL) {
        perror("fopen");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    if (size < 0) {
        perror("ftell");
        fclose(file);
        return 1;
    }

    uint8_t *data = malloc((size_t)size);
    if (data == NULL) {
        fclose(file);
        return 1;
    }

    size_t bytes_read = fread(data, 1, (size_t)size, file);

    if (bytes_read != (size_t)size) {
        fprintf(stderr, "Failed to read entire file\n");
        free(data);
        fclose(file);
        return 1;
    }

    fclose(file);

    CPU cpu;
    Bus bus;

    bus.memory = data;

    reset(&cpu);

    for (int i=0;i<10000000;i++) {
        tick(&cpu, &bus);
        print_registers(&cpu);
    }

    free(data);

    return 0;
}