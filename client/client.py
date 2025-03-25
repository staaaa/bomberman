import pygame
import socket
import struct
import threading

SERVER_IP = "127.0.0.1"
SERVER_PORT = 50000
SCREEN_WIDTH, SCREEN_HEIGHT = 800, 600
PLAYER_SPEED = 5

IDLE, PLACED_BOMB, MOVED = 0, 1, 2

class Client:
    def __init__(self, server_ip, server_port):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((server_ip, server_port))
        self.running = True
        self.players = []

        self.player_id = struct.unpack("i", self.sock.recv(4))[0]  

        threading.Thread(target=self.receive_game_state, daemon=True).start()

    def send_move(self, player_id, move_type, x, y):
        if self.sock:
            try:
                data = struct.pack("iiii", player_id, move_type, x, y)
                self.sock.sendall(data)
            except (BrokenPipeError, OSError) as e:
                print(f"Error sending move: {e}")
                self.running = False

    def receive_game_state(self):
        while self.running:
            try:
                data = self.sock.recv(4)
                if not data or len(data) < 4:
                    print("Connection lost or incomplete data")
                    break

                num_players = struct.unpack("i", data)[0]
                self.players = []

                for _ in range(num_players):
                    data = self.sock.recv(8)
                    if not data or len(data) < 8:
                        print("Incomplete player data")
                        break

                    px, py = struct.unpack("ii", data)
                    self.players.append((px, py))

            except Exception as e:
                print(f"Error receiving game state: {e}")
                break

        self.running = False
        self.sock.close()

pygame.init()
screen = pygame.display.set_mode((SCREEN_WIDTH, SCREEN_HEIGHT))
clock = pygame.time.Clock()
client = Client(SERVER_IP, SERVER_PORT)

running = True
player_x, player_y = 100, 100

while running:
    screen.fill((0, 0, 0))
    
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False

    keys = pygame.key.get_pressed()
    dx, dy = 0, 0

    if keys[pygame.K_LEFT]: dx = -PLAYER_SPEED
    if keys[pygame.K_RIGHT]: dx = PLAYER_SPEED
    if keys[pygame.K_UP]: dy = -PLAYER_SPEED
    if keys[pygame.K_DOWN]: dy = PLAYER_SPEED

    if dx or dy:
        player_x += dx
        player_y += dy
        client.send_move(client.player_id, MOVED, player_x, player_y)

    for px, py in client.players:
        pygame.draw.rect(screen, (0, 255, 0), (px, py, 20, 20))  

    pygame.display.flip()
    clock.tick(60)

pygame.quit()
