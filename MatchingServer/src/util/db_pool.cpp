#include "db_pool.h"

// 표준 라이브러리 헤더 포함
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <stdexcept>

// 로깅 라이브러리
#include <spdlog/spdlog.h>

// PostgreSQL 클라이언트 라이브러리
#include <pqxx/pqxx>

namespace game_server {

    DbPool::DbPool(const std::string& connectionString, int poolSize)
        : connection_string_(connectionString)
    {
        try {
            // 연결 풀 초기화
            connections_.reserve(poolSize);
            in_use_.reserve(poolSize);

            for (int i = 0; i < poolSize; ++i) {
                // 새 연결 생성 및 풀에 추가
                auto conn = std::make_shared<pqxx::connection>(connectionString);
                connections_.push_back(conn);
                in_use_.push_back(false);

                spdlog::info("데이터베이스 연결 생성 {}/{}", i + 1, poolSize);
            }

            spdlog::info("{}개의 데이터베이스 풀 초기화 성공", poolSize);
        }
        catch (const std::exception& e) {
            spdlog::error("데이터베이스 풀 초기화 중 예외가 발생하였습니다: {}", e.what());
            throw;
        }
    }

    DbPool::~DbPool()
    {
        // 모든 연결 종료
        connections_.clear();
        spdlog::info("데이터베이스 풀 삭제");
    }

    std::shared_ptr<pqxx::connection> DbPool::get_connection()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // 사용 가능한 연결 찾기
        for (size_t i = 0; i < connections_.size(); ++i) {
            if (!in_use_[i]) {
                in_use_[i] = true;

                // 연결이 유효한지 확인
                if (!connections_[i]->is_open()) {
                    spdlog::warn("{}번째 연결이 종료되었습니다, 재연결 진행 중...", i);
                    try {
                        connections_[i] = std::make_shared<pqxx::connection>(connection_string_);
                    }
                    catch (const std::exception& e) {
                        spdlog::error("재연결 중 예외가 발생하였습니다: {}", e.what());
                        in_use_[i] = false;
                        throw;
                    }
                }

                return connections_[i];
            }
        }

        // 사용 가능한 연결이 없으면 새 연결 생성
        spdlog::warn("사용 가능한 연결이 없습니다, 연결을 추가로 생성합니다");
        try {
            auto conn = std::make_shared<pqxx::connection>(connection_string_);
            connections_.push_back(conn);
            in_use_.push_back(true);
            return conn;
        }
        catch (const std::exception& e) {
            spdlog::error("연결 추가 중 예외가 발생하였습니다: {}", e.what());
            throw;
        }
    }

    void DbPool::return_connection(std::shared_ptr<pqxx::connection> conn)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // 반환된 연결 찾아서 사용 가능 상태로 변경
        for (size_t i = 0; i < connections_.size(); ++i) {
            if (connections_[i] == conn) {
                in_use_[i] = false;
                return;
            }
        }

        spdlog::warn("데이터베이스 풀로 반환 가능한 연결을 찾지 못했습니다");
    }

} // namespace game_server