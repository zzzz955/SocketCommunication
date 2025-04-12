// repository/room_repository.cpp
// 방 리포지토리 구현 파일
// 방 관련 데이터베이스 작업을 처리하는 리포지토리
#include "room_repository.h"
#include "../util/db_pool.h"
#include <pqxx/pqxx>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

namespace game_server {

    using json = nlohmann::json;

    // 리포지토리 구현체
    class RoomRepositoryImpl : public RoomRepository {
    public:
        explicit RoomRepositoryImpl(DbPool* dbPool) : dbPool_(dbPool) {}

        std::vector<json> findAllOpen() override {
            std::vector<json> rooms;
            auto conn = dbPool_->get_connection();
            pqxx::work txn(*conn);
            try {
                // 열린 방 목록 조회 (최근 생성순)
                pqxx::result result = txn.exec_params(
                    "SELECT room_id, room_name, host_id, ip_address, port, "
                    "max_players, status, created_at "
                    "FROM rooms WHERE status = 'WAITING' OR status = 'GAME_IN_PROGRESS' "
                    "ORDER BY created_at DESC");

                txn.commit();
                dbPool_->return_connection(conn);

                // 조회 결과를 Room 객체 리스트로 변환
                for (const auto& row : result) {
                    json room;
                    room["roomId"] = row["room_id"].as<int>();
                    room["roomName"] = row["room_name"].as<std::string>();
                    room["hostId"] = row["host_id"].as<int>();
                    room["ipAddress"] = row["ip_address"].as<std::string>();
                    room["port"] = row["port"].as<int>();
                    room["maxPlayers"] = row["max_players"].as<int>();
                    room["status"] = row["status"].as<std::string>();
                    room["createdAt"] = row["created_at"].as<std::string>();
                    rooms.push_back(room);
                }
                return rooms;
            }
            catch (const std::exception& e) {
                spdlog::error("열린 방 목록 가져오기 오류: {}", e.what());
                txn.abort();
                dbPool_->return_connection(conn);
                return rooms;
            }
        }

        json createRoomWithHost(int hostId, const std::string& roomName, int maxPlayers) {
            auto conn = dbPool_->get_connection();
            pqxx::work txn(*conn);
            int roomId = -1;
            json result = {
                {"roomId", -1}
            };
            try {
                pqxx::result isJoined = txn.exec_params(
                    "SELECT room_id FROM room_users WHERE user_id = $1 LIMIT 1"
                    , hostId);
                if (!isJoined.empty()) {
                    txn.abort();
                    dbPool_->return_connection(conn);
                    return result;
                }

                // 유효한 방 ID 찾기
                pqxx::result idResult = txn.exec(
                    "SELECT room_id FROM rooms WHERE status = 'TERMINATED' ORDER BY room_id LIMIT 1 FOR UPDATE");

                if (idResult.empty()) {
                    txn.abort();
                    dbPool_->return_connection(conn);
                    return result;
                }
                roomId = idResult[0][0].as<int>();

                // 방 재활성화
                pqxx::result roomResult;
                roomResult = txn.exec_params(
                    "UPDATE rooms SET room_name = $1, host_id = $2, max_players = $3, "
                    "status = 'WAITING', created_at = DEFAULT "
                    "WHERE room_id = $4 AND status = 'TERMINATED' "
                    "RETURNING room_id, room_name, ip_address, port, max_players",
                    roomName, hostId, maxPlayers, roomId);

                if (roomResult.empty()) {
                    txn.abort();
                    dbPool_->return_connection(conn);
                    return result;
                }

                // 사용자를 방에 추가
                txn.exec_params(
                    "INSERT INTO room_users(room_id, user_id) VALUES($1, $2)",
                    roomId, hostId);

                txn.commit();
                dbPool_->return_connection(conn);
                result["roomId"] = roomResult[0]["room_id"].as<int>();
                result["roomName"] = roomResult[0]["room_name"].as<std::string>();
                result["ipAddress"] = roomResult[0]["ip_address"].as<std::string>();
                result["port"] = roomResult[0]["port"].as<int>();
                result["maxPlayers"] = roomResult[0]["max_players"].as<int>();
                return result;
            }
            catch (const std::exception& e) {
                spdlog::error("createRoomWithHost 오류: {}", e.what());
                txn.abort();
                dbPool_->return_connection(conn);
                return result;
            }
        }

        bool addPlayer(int roomId, int userId) override {
            auto conn = dbPool_->get_connection();
            pqxx::work txn(*conn);
            try {
                // 방이 존재하고 WAITING 상태인지 확인
                pqxx::result roomCheck = txn.exec_params(
                    "SELECT status FROM rooms WHERE room_id = $1",
                    roomId);

                if (roomCheck.empty()) {
                    spdlog::error("방 {}이(가) 존재하지 않습니다", roomId);
                    txn.abort();
                    dbPool_->return_connection(conn);
                    return false;
                }

                std::string status = roomCheck[0][0].as<std::string>();
                if (status != "WAITING") {
                    spdlog::error("방 {}에 참가할 수 없습니다 - 상태가 {}입니다", roomId, status);
                    txn.abort();
                    dbPool_->return_connection(conn);
                    return false;
                }

                // 이미 참가한 사용자인지 확인
                pqxx::result checkResult = txn.exec_params(
                    "SELECT joined_at FROM room_users "
                    "WHERE room_id = $1 AND user_id = $2",
                    roomId, userId);

                if (!checkResult.empty()) {
                    // 이미 참가한 상태
                    spdlog::error("사용자 {}는 이미 방 {}에 있습니다", userId, roomId);
                    txn.abort();
                    dbPool_->return_connection(conn);
                    return false;
                }

                // 최대 인원 확인
                pqxx::result maxPlayersResult = txn.exec_params(
                    "SELECT max_players, "
                    "(SELECT COUNT(*) FROM room_users WHERE room_id = $1) as current_players "
                    "FROM rooms WHERE room_id = $1 FOR UPDATE",
                    roomId);

                int maxPlayers = maxPlayersResult[0]["max_players"].as<int>();
                int currentPlayers = maxPlayersResult[0]["current_players"].as<int>();

                if (currentPlayers >= maxPlayers) {
                    spdlog::error("방 {}이(가) 가득 찼습니다 ({}/{})", roomId, currentPlayers, maxPlayers);
                    txn.abort();
                    dbPool_->return_connection(conn);
                    return false;
                }

                // 새 참가자 추가
                pqxx::result result = txn.exec_params(
                    "INSERT INTO room_users (room_id, user_id, joined_at) "
                    "VALUES ($1, $2, DEFAULT) RETURNING room_id",
                    roomId, userId);

                if (result.empty()) {
                    txn.abort();
                    dbPool_->return_connection(conn);
                    return false;
                }

                txn.commit();
                dbPool_->return_connection(conn);
                spdlog::debug("사용자 {}이(가) 방 {}에 참가했습니다", userId, roomId);
                return true;
            }
            catch (const std::exception& e) {
                spdlog::error("방에 플레이어 추가 오류: {}", e.what());
                txn.abort();
                dbPool_->return_connection(conn);
                return false;
            }
        }

        bool removePlayer(int userId) override {
            auto conn = dbPool_->get_connection();
            pqxx::work txn(*conn);
            try {
                // 사용자가 속한 방 ID 가져오기
                pqxx::result roomResult = txn.exec_params(
                    "SELECT room_id FROM room_users WHERE user_id = $1",
                    userId);

                if (roomResult.empty()) {
                    // 사용자가 어떤 방에도 없음
                    txn.abort();
                    dbPool_->return_connection(conn);
                    spdlog::warn("사용자 {}은(는) 어떤 방에도 없습니다", userId);
                    return false;
                }

                int room_id = roomResult[0][0].as<int>();

                // 참가자 제거
                txn.exec_params(
                    "DELETE FROM room_users WHERE user_id = $1",
                    userId);

                // 동일 트랜잭션 내에서 플레이어 수 확인
                pqxx::result countResult = txn.exec_params(
                    "SELECT COUNT(*) FROM room_users WHERE room_id = $1",
                    room_id);

                int remaining_players = countResult[0][0].as<int>();

                // 방에 남은 플레이어가 없으면 방 상태 TERMINATED로 변경
                if (remaining_players == 0) {
                    txn.exec_params(
                        "UPDATE rooms SET status = 'TERMINATED' WHERE room_id = $1",
                        room_id);

                    txn.exec_params(
                        "UPDATE games SET status = 'COMPLETED', completed_at = CURRENT_TIMESTAMP WHERE status = 'IN_PROGRESS' AND room_id = $1",
                        room_id);

                    spdlog::debug("방 {}이(가) 종료 처리되었습니다 (남은 플레이어 없음), 해당 방의 진행 중 게임들도 완료 처리: {}", room_id, room_id);
                }

                txn.commit();
                dbPool_->return_connection(conn);
                spdlog::debug("사용자 {}이(가) 방 {}을(를) 나갔습니다, 남은 플레이어 {}명",
                    userId, room_id, remaining_players);
                return true;
            }
            catch (const std::exception& e) {
                spdlog::error("방에서 플레이어 제거 오류: {}", e.what());
                txn.abort();
                dbPool_->return_connection(conn);
                return false;
            }
        }

        int getPlayerCount(int roomId) override {
            auto conn = dbPool_->get_connection();
            pqxx::work txn(*conn);
            try {
                // 남은 플레이어 수 확인
                pqxx::result result = txn.exec_params(
                    "SELECT COUNT(*) FROM room_users WHERE room_id = $1",
                    roomId);

                txn.commit();
                dbPool_->return_connection(conn);

                return result.empty() ? 0 : result[0][0].as<int>();
            }
            catch (const std::exception& e) {
                spdlog::error("방 플레이어 수 가져오기 오류: {}", e.what());
                txn.abort();
                dbPool_->return_connection(conn);
                return -1;
            }
        }

        std::vector<int> getPlayersInRoom(int roomId) override {
            std::vector<int> playerIds;
            auto conn = dbPool_->get_connection();
            pqxx::work txn(*conn);

            try {
                // 현재 방에 있는 참가자 ID 목록 조회
                pqxx::result result = txn.exec_params(
                    "SELECT user_id FROM room_users "
                    "WHERE room_id = $1",
                    roomId);

                txn.commit();
                dbPool_->return_connection(conn);

                for (const auto& row : result) {
                    playerIds.push_back(row[0].as<int>());
                }

                spdlog::debug("방 {}에서 {}명의 플레이어를 찾았습니다", roomId, playerIds.size());
            }
            catch (const std::exception& e) {
                spdlog::error("방 플레이어 목록 가져오기 오류: {}", e.what());
                txn.abort();
                dbPool_->return_connection(conn);
            }

            return playerIds;
        }

    private:
        DbPool* dbPool_;
    };

    // 팩토리 메서드 구현
    std::unique_ptr<RoomRepository> RoomRepository::create(DbPool* dbPool) {
        return std::make_unique<RoomRepositoryImpl>(dbPool);
    }

} // namespace game_server