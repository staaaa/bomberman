import socket
import struct
import threading
from consts import GRID_SIZE
import game_pb2  # Wygenerowany plik protobuf dla Pythona

class Client:
    def __init__(self, server_ip, server_port):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((server_ip, server_port))
        self.running = True
        self.players = []

        # Pobieranie początkowego stanu gry przez protobuf
        initial_data = self.recv_protobuf_message()
        if not initial_data:
            raise ConnectionError("Failed to receive initial game state")
        
        # Deserializujemy dane
        init_state = game_pb2.InitialState()
        init_state.ParseFromString(initial_data)
        
        self.player_id = init_state.player_id
        
        # Pobierz mapę
        self.tiles = self.convert_pb_map_to_tiles(init_state.map)
        
        self.SCREEN_HEIGHT = len(self.tiles) * GRID_SIZE
        self.SCREEN_WIDTH = len(self.tiles[0]) * GRID_SIZE

        self.lock = threading.Lock()
        threading.Thread(target=self.receive_game_state, daemon=True).start()

    def recv_protobuf_message(self):
        """Odbiera wiadomość protobuf z serwera"""
        # Odbierz długość wiadomości (4 bajty)
        length_bytes = self.recvall(4)
        if not length_bytes:
            return None
        
        message_length = struct.unpack("!I", length_bytes)[0]  # '!' oznacza network byte order
        
        # Odbierz wiadomość
        message_data = self.recvall(message_length)
        return message_data

    def send_protobuf_message(self, message):
        """Wysyła wiadomość protobuf do serwera"""
        if not self.sock or not self.running:
            return False
        
        # Serializacja wiadomości
        serialized_message = message.SerializeToString()
        
        # Wysyłanie długości wiadomości
        length_prefix = struct.pack("!I", len(serialized_message))
        
        try:
            self.sock.sendall(length_prefix)
            self.sock.sendall(serialized_message)
            return True
        except Exception as e:
            print(f"Error sending protobuf message: {e}")
            self.running = False
            return False

    def recvall(self, n):
        """Odbiera dokładnie n bajtów danych"""
        data = b''
        while len(data) < n:
            packet = self.sock.recv(n - len(data))
            if not packet:
                return None
            
            data += packet
        
        return data

    def convert_pb_map_to_tiles(self, pb_map):
        """Konwertuje mapę protobuf na format tiles używany w grze"""
        width = pb_map.width
        height = pb_map.height
        
        # Inicjalizuj pustą tablicę kafelków
        tiles = [[None for _ in range(width)] for _ in range(height)]
        
        # Wypełnij dane kafelków
        for tile in pb_map.tiles:
            tiles[tile.y][tile.x] = {
                'type': tile.type,
            }
            
        return tiles

    def send_move(self, player_id, move_type, x, y):
        """Wysyła ruch gracza do serwera za pomocą protobuf"""
        if self.sock and self.running:
            try:
                # Tworzymy wiadomość protobuf PlayerMove
                move = game_pb2.PlayerMove()
                move.player_id = player_id
                move.move_type = move_type
                move.pos_x = x
                move.pos_y = y
                
                # Wysyłamy wiadomość
                self.send_protobuf_message(move)
            except Exception as e:
                print(f"Error sending move: {e}")
                self.running = False

    def receive_game_state(self):
        """Odbiera stan gry z serwera za pomocą protobuf"""
        while self.running:
            try:
                # Odbierz wiadomość protobuf
                message_data = self.recv_protobuf_message()
                if not message_data:
                    print("Connection lost or incomplete data")
                    break
                
                # Deserializuj dane stanu gry
                game_state = game_pb2.GameState()
                game_state.ParseFromString(message_data)
                
                with self.lock:
                    # Aktualizuj graczy
                    new_players = []
                    for player in game_state.players:
                        new_players.append((player.id, player.pos_x, player.pos_y, player.alive, player.points))
                    
                    self.players = new_players
                    
                    # Aktualizuj mapę
                    for tile in game_state.map.tiles:
                        self.tiles[tile.y][tile.x]['type'] = tile.type
                
            except Exception as e:
                print(f"Error receiving game state: {e}")
                break

        self.running = False
        self.sock.close() 

    def get_players(self):
        """Zwraca kopię listy graczy"""
        with self.lock:
            return self.players.copy()