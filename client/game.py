import pygame
from client import Client
from consts import *

class Game:
    def __init__(self):
        pygame.init()
        self.client = Client(SERVER_IP, SERVER_PORT)
        self.screen = pygame.display.set_mode((self.client.SCREEN_WIDTH, self.client.SCREEN_HEIGHT))
        pygame.display.set_caption("Bomberman")
        self.clock = pygame.time.Clock()
        self.running = True
        self.font = pygame.font.SysFont(None, 36)  # Add font for text
        self.player_dead = False  # Track if player is dead

        self.player_x = 0
        self.player_y = 0
        self.player_points = 0  # Track player points

        self.last_move_time = pygame.time.get_ticks()
        self.last_bomb_time = pygame.time.get_ticks()
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
        # Check if player is alive first
        players = self.client.get_players()
        player_alive = False
        
        for id, _, _, alive, points in players:
            if id == self.client.player_id and alive:
                player_alive = True
                break
                
        if not player_alive:
            return
            
        dx, dy = 0, 0
        if keys[pygame.K_LEFT]: dx = -PLAYER_SPEED
        if keys[pygame.K_RIGHT]: dx = PLAYER_SPEED
        if keys[pygame.K_UP]: dy = -PLAYER_SPEED
        if keys[pygame.K_DOWN]: dy = PLAYER_SPEED

        if dx or dy:
            new_x = self.player_x + dx
            new_y = self.player_y + dy
            
            # Check if the new position is within screen bounds
            if 0 <= new_x < self.client.SCREEN_WIDTH and 0 <= new_y < self.client.SCREEN_HEIGHT:
                # Check if the new position is not a wall or bomb
                grid_x = new_x // GRID_SIZE
                grid_y = new_y // GRID_SIZE
                
                if self.client.tiles[grid_y][grid_x]['type'] == 0:  # EMPTY or EXPLOSION is ok
                    self.player_x = new_x
                    self.player_y = new_y
                    self.client.send_move(self.client.player_id, MOVED, self.player_x, self.player_y)
                    self.last_move_time = self.current_time

    def draw_points(self):
        # Draw player points in upper right corner
        points_text = self.font.render(f"Points: {self.player_points}", True, (160, 0, 0))
        self.screen.blit(points_text, (self.client.SCREEN_WIDTH - 150, 10))
    
    def draw_death_screen(self):
        # Create semi-transparent overlay
        overlay = pygame.Surface((self.client.SCREEN_WIDTH, self.client.SCREEN_HEIGHT))
        overlay.set_alpha(180)
        overlay.fill((0, 0, 0))
        self.screen.blit(overlay, (0, 0))
        
        # Draw game over text
        game_over = self.font.render("GAME OVER", True, (255, 0, 0))
        points_text = self.font.render(f"Final Score: {self.player_points}", True, (255, 255, 255))
        restart_text = self.font.render("Press R to restart or Q to quit", True, (255, 255, 255))
        
        self.screen.blit(game_over, (self.client.SCREEN_WIDTH//2 - game_over.get_width()//2, 
                                     self.client.SCREEN_HEIGHT//2 - 60))
        self.screen.blit(points_text, (self.client.SCREEN_WIDTH//2 - points_text.get_width()//2, 
                                      self.client.SCREEN_HEIGHT//2))
        self.screen.blit(restart_text, (self.client.SCREEN_WIDTH//2 - restart_text.get_width()//2, 
                                       self.client.SCREEN_HEIGHT//2 + 60))
        
    def draw_tiles(self):
        for y in range(len(self.client.tiles)):
            for x in range(len(self.client.tiles[0])):
                tile = self.client.tiles[y][x]
                if tile is None:
                    continue

                if tile['type'] == 0:  # EMPTY
                    color = (255, 255, 255)
                elif tile['type'] == 1:  # BREAKABLE_WALL
                    color = (150, 150, 150)
                elif tile['type'] == 2:  # UNBREAKABLE_WALL
                    color = (0, 0, 0)
                elif tile['type'] == 3:  # BOMB
                    color = BOMB_COLOR
                elif tile['type'] == 4:  # EXPLOSION
                    color = EXPLOSION_COLOR

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
                    
                # Handle restart and quit from death screen
                if self.player_dead and event.type == pygame.KEYDOWN:
                    if event.key == pygame.K_r:
                        self.client.send_move(self.client.player_id, RESTART, 0, 0)
                        pass
                    elif event.key == pygame.K_q:
                        self.running = False

            keys = pygame.key.get_pressed()
            self.current_time = pygame.time.get_ticks()
            
            # Check player alive status
            players = self.client.get_players()
            self.player_dead = True  # Assume dead until proven alive
            
            for id, px, py, alive, points in players:
                if id == self.client.player_id:
                    if alive:
                        self.player_dead = False
                        self.player_points = points  # Update points
                        self.player_x = px
                        self.player_y = py
                    else:
                        self.player_dead = True
                        self.player_points = points  # Keep final score
            
            # Only allow movement and bomb placement if alive
            if not self.player_dead:
                if self.current_time - self.last_move_time >= MOVE_DELAY:
                    self.move_player(keys)
                    
                if keys[pygame.K_SPACE] and self.current_time - self.last_bomb_time >= BOMB_DELAY:
                    self.client.send_move(self.client.player_id, PLACED_BOMB, self.player_x, self.player_y)
                    self.last_bomb_time = self.current_time

            # Draw players
            for id, px, py, alive, points in players:
                if alive:
                    pygame.draw.rect(self.screen, COLORS[id % len(COLORS)], (px, py, GRID_SIZE, GRID_SIZE))

            # Draw points counter
            self.draw_points()
            
            # Draw death screen if player is dead
            if self.player_dead:
                self.draw_death_screen()

            pygame.display.flip()
            self.clock.tick(60)
        pygame.quit()

if __name__ == "__main__":
    game = Game()
    game.game_loop()