COMPILER = gcc
SERVER_FLAGS = -Wall -Wextra -pedantic
SERVER_DIR = backend
COMMON_DIR = common
CLIENT_DIR = client
TARGET = server.out

PHONY: run_server run_client

run_server:
	cd $(SERVER_DIR) && \
	$(COMPILER) $(SERVER_FLAGS) -o $(TARGET) server.c state_updater.c ../common/map.c && \
	./$(TARGET) && \
	cd ..

run_client:
	python -m venv venv && \
	source venv/bin/activate && \
	pip install -r requirements.txt && \
	python $(CLIENT_DIR)/game.py

clean:
	rm -rf $(SERVER_DIR)/*.out venv/ $(CLIENT_DIR)/__pycache__
