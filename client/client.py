import socket
import struct
import threading
from consts import GRID_SIZE

class Client:
    def __init__(self, server_ip, server_port):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((server_ip, server_port))
        self.running = True
        self.players = []

        self.player_id = struct.unpack("i", self.recvall(4))[0]
        self.tiles = self.get_tiles_from_server()

        self.SCREEN_HEIGHT = len(self.tiles) * GRID_SIZE
        self.SCREEN_WIDTH = len(self.tiles[0]) * GRID_SIZE

        self.lock = threading.Lock()
        threading.Thread(target=self.receive_game_state, daemon=True).start()

    def recvall(self, n):
        data = b''
        while len(data) < n:
            packet = self.sock.recv(n - len(data))
            if not packet:
                return None
            
            data += packet
        
        return data

    def get_tiles_from_server(self):
        width = struct.unpack("i", self.recvall(4))[0]
        height = struct.unpack("i", self.recvall(4))[0]

        tiles = [[None for _ in range(width)] for _ in range(height)]

        for y in range(height):
            for x in range(width):
                tile_data = self.recvall(12) 
                if tile_data is None:
                    raise ConnectionError("Incomplete tile data received")

                tile_x, tile_y, tile_type = struct.unpack("iii", tile_data)

                if tile_x != x or tile_y != y:
                    raise ValueError(f"Unexpected tile coordinates: expected ({x},{y}), got ({tile_x},{tile_y})")

                tiles[tile_y][tile_x] = {
                    'type': tile_type,
                }

        return tiles

    def send_move(self, player_id, move_type, x, y):
        if self.sock and self.running:
            try:
                data = struct.pack("iiii", player_id, move_type, x, y)
                self.sock.sendall(data)
            except (BrokenPipeError, OSError) as e:
                print(f"Error sending move: {e}")
                self.running = False

    def receive_game_state(self):
        while self.running:
            try:
                data = self.recvall(4)
                if data is None:
                    print("Connection lost or incomplete data")
                    break

                raw = self.recvall(4)
                if raw is None: break
                num_players = struct.unpack("i", raw)[0]

                with self.lock:
                    new_players = []

                    for _ in range(num_players):
                        data = self.recvall(20)  # Changed from 12 to 16 to include alive status
                        if data is None:
                            print("Incomplete player data")
                            break

                        id, px, py, alive, points = struct.unpack("iiiii", data)
                        new_players.append((id, px, py, alive, points))

                    self.players = new_players
                    
                    # Receive updated map
                    for y in range(len(self.tiles)):
                        for x in range(len(self.tiles[0])):
                            raw = self.recvall(12)
                            if raw is None:
                                print("Incomplete tile data received")
                                self.running = False
                                break
                                
                                
                            tile_x, tile_y, tile_type = struct.unpack("iii", raw)
                            self.tiles[tile_y][tile_x]['type'] = tile_type

            except Exception as e:
                print(f"Error receiving game state: {e}")
                break

        self.running = False
        self.sock.close() 

    def get_players(self):
        with self.lock:
            return self.players.copy()
