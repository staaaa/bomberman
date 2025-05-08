COMPILER = gcc
SERVER_FLAGS = -Wall -Wextra -pedantic
SERVER_DIR = backend
COMMON_DIR = common
CLIENT_DIR = client
TARGET = server.out
PROTOC = protoc
PROTOC_C = protoc-c

.PHONY: all proto run_server run_client clean

all: proto run_server

proto:
	$(PROTOC) --c_out=$(SERVER_DIR) --proto_path=. game.proto
	$(PROTOC) --python_out=$(CLIENT_DIR) --proto_path=. game.proto

run_server: proto
	cd $(SERVER_DIR) && \
	$(COMPILER) $(SERVER_FLAGS) -o $(TARGET) server.c state_updater.c ../common/map.c game.pb-c.c -lprotobuf-c -pthread && \
	./$(TARGET) && \
	cd ..

run_client: proto
	python -m venv venv && \
	. venv/bin/activate && \
	pip install protobuf && \
	pip install -r requirements.txt && \
	python $(CLIENT_DIR)/game.py

clean:
	rm -rf $(SERVER_DIR)/*.out $(SERVER_DIR)/*.pb-c.* venv/ $(CLIENT_DIR)/__pycache__/ $(CLIENT_DIR)/*.py[cod] $(CLIENT_DIR)/*_pb2.py