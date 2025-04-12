#include "game_service.h"
#include "../repository/game_repository.h"
#include <spdlog/spdlog.h>
#include <random>
#include <string>
#include <nlohmann/json.hpp>

namespace game_server {

    using json = nlohmann::json;

    // 서비스 구현체
    class GameServiceImpl : public GameService {
    public:
        explicit GameServiceImpl(std::shared_ptr<GameRepository> gameRepo)
            : gameRepo_(gameRepo) {
        }

        json startGame(json& request) {
            json response;
            try {
                // 요청 유효성 검증
                if (!request.contains("roomId") || !request.contains("mapId")) {
                    response["status"] = "error";
                    response["message"] = "게임 시작 요청에 필수 필드가 누락되었습니다";
                    return response;
                }

                // 게임 ID 얻기 실패 시 -1
                json result = gameRepo_->createGame(request);

                if (result["gameId"] == -1) {
                    response["status"] = "error";
                    response["message"] = "새 게임 기록 추가에 실패했습니다";
                    return response;
                }

                // 성공 응답 생성
                response["action"] = "gameStart";
                response["status"] = "success";
                response["message"] = "게임이 성공적으로 생성되었습니다";
                response["gameId"] = result["gameId"];
                response["users"] = result["users"];

                spdlog::info("방 {}가 새 게임 ID: {}를 생성했습니다",
                    request["roomId"].get<int>(), response["gameId"].get<int>());

                return response;
            }
            catch (const std::exception& e) {
                response["status"] = "error";
                response["message"] = std::string("게임 생성 오류: ") + e.what();
                spdlog::error("createGame 오류: {}", e.what());
                return response;
            }
        }

        json endGame(json& request) {
            json response;
            try {
                // 요청 유효성 검증
                if (!request.contains("gameId")) {
                    response["status"] = "error";
                    response["message"] = "게임 종료 요청에 필수 필드가 누락되었습니다";
                    return response;
                }

                json result = gameRepo_->endGame(request["gameId"]);
                if (result["gameId"] == -1) {
                    response["status"] = "error";
                    response["message"] = "게임 종료 업데이트에 실패했습니다";
                    return response;
                }

                // 성공 응답 생성
                response["action"] = "gameEnd";
                response["status"] = "success";
                response["roomId"] = result["roomId"];
                response["message"] = "게임이 성공적으로 종료되었습니다";
                response["users"] = result["users"];

                spdlog::info("방 {}가 게임 ID: {}를 종료했습니다",
                    response["roomId"].get<int>(), request["gameId"].get<int>());
                return response;
            }
            catch (const std::exception& e) {
                response["status"] = "error";
                response["message"] = std::string("게임 종료 오류: ") + e.what();
                spdlog::error("endGame 오류: {}", e.what());
                return response;
            }
        }

    private:
        std::shared_ptr<GameRepository> gameRepo_;
    };

    // 팩토리 메서드 구현
    std::unique_ptr<GameService> GameService::create(std::shared_ptr<GameRepository> gameRepo) {
        return std::make_unique<GameServiceImpl>(gameRepo);
    }

} // namespace game_server