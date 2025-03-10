enum TileType{
  EMPTY,
  BREAKABLE_WALL,
  UNBREAKABLE_WALL,
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
