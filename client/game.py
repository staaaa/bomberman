import pygame
from client import Client
from consts import *

class Game:
    def __init__(self):
        pygame.init()
        self.client = Client(SERVER_IP, SERVER_PORT)
        self.screen = pygame.display.set_mode((self.client.SCREEN_WIDTH, self.client.SCREEN_HEIGHT))
        self.clock = pygame.time.Clock()
        self.running = True

        self.player_x = 0
        self.player_y = 0

        self.last_move_time = pygame.time.get_ticks()
        self.current_time = None

    def is_position_empty(self, x, y):
        if 0 <= x < self.client.SCREEN_WIDTH and 0 <= y < self.client.SCREEN_HEIGHT:
            if self.client.tiles[y//GRID_SIZE][x//GRID_SIZE]['type'] != IDLE:
                return False
            players = self.client.get_players()
            for id, px, py in players:
                if x == px and y == py:
                    return False
            return True
        return False

    def move_player(self, keys):
        dx, dy = 0, 0
        if keys[pygame.K_LEFT]: dx = -PLAYER_SPEED
        if keys[pygame.K_RIGHT]: dx = PLAYER_SPEED
        if keys[pygame.K_UP]: dy = -PLAYER_SPEED
        if keys[pygame.K_DOWN]: dy = PLAYER_SPEED

        if dx or dy:
            if self.is_position_empty(self.player_x+dx, self.player_y+dy):
                self.player_x += dx
                self.player_y += dy
                self.client.send_move(self.client.player_id, MOVED, self.player_x, self.player_y)
                self.last_move_time = self.current_time

    def draw_tiles(self):
        for y in range(len(self.client.tiles)):
            for x in range(len(self.client.tiles[0])):
                tile = self.client.tiles[y][x]
                if tile is None:
                    continue

                if tile['type'] == 0:
                    color = (255, 255, 255)
                elif tile['type'] == 1:
                    color = (150, 150, 150)
                elif tile['type'] == 2:
                    color = (0, 0, 0)

                pygame.draw.rect(
                    self.screen,
                    color,
                    (x * GRID_SIZE, y * GRID_SIZE, GRID_SIZE, GRID_SIZE)
                )

    def game_loop(self):
        while self.running:
            self.screen.fill((0,0,0))
            self.draw_tiles()

            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    self.running = False

            keys = pygame.key.get_pressed()
            self.current_time = pygame.time.get_ticks()
            if self.current_time - self.last_move_time >= MOVE_DELAY:
                self.move_player(keys)

            pygame.draw.rect(self.screen, COLORS[self.client.player_id], (self.player_x, self.player_y, GRID_SIZE, GRID_SIZE))

            players = self.client.get_players()
            for id, px, py in players:
                if id != self.client.player_id:
                    pygame.draw.rect(self.screen, COLORS[id], (px, py, GRID_SIZE, GRID_SIZE))

            pygame.display.flip()
            self.clock.tick(60)
        pygame.quit()

if __name__ == "__main__":
    game = Game()
    game.game_loop()