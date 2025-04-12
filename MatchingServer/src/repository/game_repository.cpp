#include "game_repository.h"
#include "../util/db_pool.h"
#include <pqxx/pqxx>
#include <spdlog/spdlog.h>

namespace game_server {

    using json = nlohmann::json;

    // 리포지토리 구현체
    class GameRepositoryImpl : public GameRepository {
    public:
        explicit GameRepositoryImpl(DbPool* dbPool) : dbPool_(dbPool) {}

        json createGame(const json& request) {
            json response = {
                {"gameId", -1},
                { "users", json::array() }
            };
            auto conn = dbPool_->get_connection();
            pqxx::work txn(*conn);
            try {
                int roomId = request["roomId"];
                int mapId = request["mapId"];

                pqxx::result result = txn.exec_params(
                    "INSERT INTO games (room_id, map_id) "
                    "VALUES ($1, $2) "
                    "RETURNING game_id",
                    roomId, mapId);

                pqxx::result updateRoom = txn.exec_params(
                    "UPDATE rooms "
                    "SET status = 'GAME_IN_PROGRESS' "
                    "WHERE room_id = $1 "
                    "RETURNING status",
                    roomId);

                if (result.empty() || updateRoom.empty()) {
                    spdlog::error("방 번호 : {}에 대한 게임 세션을 생성할 수 없습니다", roomId);
                    txn.abort();
                    dbPool_->return_connection(conn);
                    return response;
                }
                int gameId = result[0][0].as<int>();
                spdlog::info("방 번호 : {}에 대한 게임 세션이 생성되었습니다 게임 ID: {}", roomId, gameId);

                pqxx::result inRoomUsers = txn.exec_params(
                    "SELECT user_id FROM room_users WHERE room_id = $1",
                    roomId);

                for (const auto& res : inRoomUsers) {
                    response["users"].push_back(res[0].as<int>());
                }

                txn.commit();
                dbPool_->return_connection(conn);
                return response;
            }
            catch (const std::exception& e) {
                txn.abort();
                dbPool_->return_connection(conn);
                spdlog::error("createGame 데이터베이스 오류: {}", e.what());
                return response;
            }
        }

        json endGame(int gameId) {
            json response = {
                {"gameId", -1},
                { "users", json::array()}
            };
            auto conn = dbPool_->get_connection();
            pqxx::work txn(*conn);
            try {
                pqxx::result result = txn.exec_params(
                    "UPDATE games "
                    "SET status = 'COMPLETED', completed_at = CURRENT_TIMESTAMP "
                    "WHERE game_id = $1 "
                    "RETURNING room_id",
                    gameId);

                if (result.empty()) {
                    spdlog::error("게임 ID: {}에 해당하는 방 ID를 찾을 수 없습니다", gameId);
                    txn.abort();
                    dbPool_->return_connection(conn);
                    return response;
                }
                int roomId = result[0][0].as<int>();
                spdlog::info("게임 ID: {}의 상태가 성공적으로 완료로 업데이트되었습니다", gameId);

                pqxx::result inRoomUsers = txn.exec_params(
                    "SELECT user_id FROM room_users WHERE room_id = $1",
                    roomId);
                response["roomId"] = roomId;
                for (const auto& res : inRoomUsers) {
                    response["users"].push_back(res[0].as<int>());
                }

                txn.commit();
                dbPool_->return_connection(conn);
                return response;
            }
            catch (const std::exception& e) {
                txn.abort();
                dbPool_->return_connection(conn);
                spdlog::error("endGame 데이터베이스 오류: {}", e.what());
                return response;
            }
        }


    private:
        DbPool* dbPool_;
    };

    // 팩토리 메서드 구현
    std::unique_ptr<GameRepository> GameRepository::create(DbPool* dbPool) {
        return std::make_unique<GameRepositoryImpl>(dbPool);
    }

} // namespace game_server