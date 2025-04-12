CXX = ccache g++
# vcpkg 헤더 파일 경로 추가
CXXFLAGS = -std=c++20 -Wall -Wextra -finput-charset=UTF-8 -I./ -I./src -I./src/core -I./src/controller -I./src/service -I./src/repository -I./src/util -I./vcpkg_installed/x64-linux/include
# vcpkg 라이브러리 경로 추가
LDFLAGS = -L./vcpkg_installed/x64-linux/lib -lpqxx -lpq -lboost_system -lssl -lcrypto -pthread -lfmt -lpgcommon -lpgport
SRC_DIR = ./src
BUILD_DIR = ./build
BIN_DIR = $(BUILD_DIR)/bin
SOURCES = $(SRC_DIR)/main.cpp \
          $(SRC_DIR)/core/server.cpp \
          $(SRC_DIR)/core/session.cpp \
          $(SRC_DIR)/controller/auth_controller.cpp \
          $(SRC_DIR)/controller/room_controller.cpp \
          $(SRC_DIR)/controller/game_controller.cpp \
          $(SRC_DIR)/service/auth_service.cpp \
          $(SRC_DIR)/service/room_service.cpp \
          $(SRC_DIR)/service/game_service.cpp \
          $(SRC_DIR)/repository/user_repository.cpp \
          $(SRC_DIR)/repository/room_repository.cpp \
          $(SRC_DIR)/repository/game_repository.cpp \
          $(SRC_DIR)/util/db_pool.cpp \
          $(SRC_DIR)/util/password_util.cpp
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
TARGET = $(BIN_DIR)/MatchingServer
# 디렉토리 자동 생성
$(shell mkdir -p $(BIN_DIR))
$(shell mkdir -p $(dir $(OBJECTS)))
all: $(TARGET)
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
clean:
	rm -rf $(BUILD_DIR)
.PHONY: all clean
