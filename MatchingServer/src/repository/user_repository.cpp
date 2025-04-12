// repository/user_repository.cpp
// 사용자 리포지토리 구현 파일
// 사용자 관련 데이터베이스 작업을 처리하는 리포지토리
#include "user_repository.h"
#include "../util/db_pool.h"
#include <pqxx/pqxx>
#include <spdlog/spdlog.h>

namespace game_server {

    using json = nlohmann::json;

    // 리포지토리 구현체
    class UserRepositoryImpl : public UserRepository {
    public:
        explicit UserRepositoryImpl(DbPool* dbPool) : dbPool_(dbPool) {}

        //std::optional<json> findById(int userId) override {}

        json findByUsername(const std::string& userName) {
            auto conn = dbPool_->get_connection();
            pqxx::work txn(*conn);
            try {
                pqxx::result result = txn.exec_params(
                    "SELECT user_id, user_name, password_hash, nick_name, created_at, last_login FROM users WHERE LOWER(user_name) = LOWER($1)",
                    userName);

                if (result.empty()) {
                    // 사용자를 찾지 못함 - std::nullopt 반환
                    txn.abort();
                    dbPool_->return_connection(conn);
                    return { {"userId", -1} };
                }

                // 결과를 JSON으로 변환
                nlohmann::json user;
                user["userId"] = result[0]["user_id"].as<int>();
                user["userName"] = result[0]["user_name"].as<std::string>();
                user["passwordHash"] = result[0]["password_hash"].as<std::string>();
                user["nickName"] = result[0]["nick_name"].as<std::string>();
                user["createdAt"] = result[0]["created_at"].as<std::string>();
                user["lastLogin"] = result[0]["last_login"].as<std::string>();

                txn.commit();
                dbPool_->return_connection(conn);
                return user;
            }
            catch (const std::exception& e) {
                txn.abort();
                dbPool_->return_connection(conn);
                spdlog::error("findByUsername 데이터베이스 오류: {}", e.what());
                return { {"userId", -1} };
            }
        }

        int create(const std::string& userName, const std::string& hashedPassword) override {
            auto conn = dbPool_->get_connection();
            pqxx::work txn(*conn);
            try {
                // 새 사용자 생성
                pqxx::result result = txn.exec_params(
                    "INSERT INTO users (user_name, password_hash) "
                    "VALUES ($1, $2) RETURNING user_id",
                    userName, hashedPassword);

                txn.commit();
                dbPool_->return_connection(conn);

                if (result.empty()) {
                    txn.abort();
                    dbPool_->return_connection(conn);
                    return -1;
                }

                return result[0][0].as<int>();
            }
            catch (const std::exception& e) {
                txn.abort();
                spdlog::error("사용자 생성 오류: {}", e.what());
                dbPool_->return_connection(conn);
                return -1;
            }
        }

        bool updateLastLogin(int userId) override {
            auto conn = dbPool_->get_connection();
            pqxx::work txn(*conn);
            try {
                // 마지막 로그인 시간 업데이트
                pqxx::result result = txn.exec_params(
                    "UPDATE users SET last_login = CURRENT_TIMESTAMP "
                    "WHERE user_id = $1 RETURNING user_id",
                    userId);

                txn.commit();
                dbPool_->return_connection(conn);

                return !result.empty();
            }
            catch (const std::exception& e) {
                txn.abort();
                spdlog::error("마지막 로그인 업데이트 오류: {}", e.what());
                dbPool_->return_connection(conn);
                return false;
            }
        }

        bool updateUserNickName(int userId, const std::string& nickName) override {
            auto conn = dbPool_->get_connection();
            pqxx::work txn(*conn);
            try {
                // 닉네임 업데이트
                pqxx::result result = txn.exec_params(
                    "UPDATE users SET nick_name = $2 "
                    "WHERE user_id = $1 "
                    "RETURNING user_id",
                    userId, nickName);

                txn.commit();
                dbPool_->return_connection(conn);

                return !result.empty();
            }
            catch (const std::exception& e) {
                txn.abort();
                spdlog::error("닉네임 업데이트 오류: {}", e.what());
                dbPool_->return_connection(conn);
                return false;
            }
        }

    private:
        DbPool* dbPool_;
    };

    // 팩토리 메서드 구현
    std::unique_ptr<UserRepository> UserRepository::create(DbPool* dbPool) {
        return std::make_unique<UserRepositoryImpl>(dbPool);
    }

} // namespace game_server