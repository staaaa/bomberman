#ifndef MAP_H
#define MAP_H


enum TileType{
  EMPTY,
  BREAKABLE_WALL,
  UNBREAKABLE_WALL,
  BOMB,
  EXPLOSION
};

typedef struct Tile{
  int x;
  int y;
  enum TileType type;
}Tile;

typedef struct Map{
  int width;
  int height;
  Tile** tile_arr;
}Map;

Map initialize_map_from_file(const char* filename, int grid_size);

#endif