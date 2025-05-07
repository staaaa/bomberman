#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../common/map.h"

Map initialize_map_from_file(const char* filename, int grid_size) {
    Map map;
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Error opening map file");
        exit(EXIT_FAILURE);
    }

    int width = 0, height = 0;
    char buffer[1024];
    char* lines[512];
    int line_count = 0;

    while (fgets(buffer, sizeof(buffer), file)) {
        lines[line_count] = strdup(buffer);
        if (line_count == 0) {
            char* token = strtok(buffer, " \n");
            while (token) {
                width++;
                token = strtok(NULL, " \n");
            }
        }
        line_count++;
    }

    height = line_count;

    map.width = width;
    map.height = height;
    map.tile_arr = malloc(height * sizeof(Tile*));
    for (int y = 0; y < height; y++) {
        map.tile_arr[y] = malloc(width * sizeof(Tile));
    }

    for (int y = 0; y < height; y++) {
        char* token = strtok(lines[y], " \n");
        for (int x = 0; x < width && token; x++) {
            map.tile_arr[y][x].x = x;
            map.tile_arr[y][x].y = y;
            map.tile_arr[y][x].type = atoi(token);
            token = strtok(NULL, " \n");
        }
        free(lines[y]);
    }

    fclose(file);
    return map;
}