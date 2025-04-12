// service/auth_service.cpp
// 인증 서비스 구현 파일
// 사용자 등록 및 로그인 비즈니스 로직을 처리
#include "auth_service.h"
#include "../util/password_util.h"
#include "../repository/user_repository.h"
#include <spdlog/spdlog.h>
#include <regex>

namespace game_server {

    using json = nlohmann::json;

    namespace {
        // 사용자 이름 유효성 검증 함수
        bool isValidUserName(const std::string& name) {
            // 빈 이름은 유효하지 않음
            if (name.empty()) {
                return false;
            }

            // 30바이트 이내인지 확인
            if (name.size() > 30) {
                return false;
            }

            // "mirror" 단어가 포함되어 있는지 확인 (대소문자 구분 없이)
            if (name.find("mirror") != std::string::npos) {
                return false;
            }

            // 이메일 형식인지 확인
            bool isEmail = (name.find('@') != std::string::npos) &&
                (name.find('.', name.find('@')) != std::string::npos);

            // 이메일이 아닌 경우 영어, 한글, 숫자, @ 문자만 포함하는지 확인
            if (!isEmail) {
                for (unsigned char c : name) {
                    // ASCII 영어와 숫자 확인
                    if ((c >= 'A' && c <= 'Z') ||
                        (c >= 'a' && c <= 'z') ||
                        (c >= '0' && c <= '9')) {
                        continue;
                    }

                    // 허용되지 않는 문자
                    return false;
                }
            }
            else {
                // 이메일인 경우 추가 검증 (간단한 이메일 형식 검사)
                // 여기서는 표준적인 이메일 문자들(영어, 숫자, 일부 특수문자) 허용
                for (unsigned char c : name) {
                    if ((c >= 'A' && c <= 'Z') ||
                        (c >= 'a' && c <= 'z') ||
                        (c >= '0' && c <= '9') ||
                        (c == '@') || (c == '.') ||
                        (c == '_') || (c == '-') || (c == '+')) {
                        continue;
                    }

                    // 이메일에 한글은 허용하지 않음 (IDN 이메일 제외)
                    return false;
                }
            }

            return true;
        }

        bool isValidNickName(const std::string& str) {
            // 정규식 패턴: 한글(가-힣), 영어(A-Za-z), 숫자(0-9)만 허용
            if (str.size() > 24) return false;

            try {
                std::regex pattern("^[가-힣A-Za-z0-9]+$");
                return std::regex_match(str, pattern);
            }
            catch (const std::regex_error& e) {
                spdlog::error("닉네임 {}에 대한 검증 시 정규식 오류 발생 : {}",str,  e.what());
                return false;
            }
        }
    }

    // 서비스 구현체
    class AuthServiceImpl : public AuthService {
    public:
        explicit AuthServiceImpl(std::shared_ptr<UserRepository> userRepo)
            : userRepo_(userRepo) {
        }

        json registerUser(const json& request) override {
            json response;

            // 사용자명 유효성 검증
            if (!request.contains("userName") || !request.contains("password")) {
                response["status"] = "error";
                response["message"] = "회원가입 요청에 필수 필드가 누락되었습니다.";
                return response;
            }

            if (!isValidUserName(request["userName"])) {
                response["status"] = "error";
                response["message"] = "잘못된 형식의 아이디입니다.";
                return response;
            }

            // 비밀번호 유효성 검증
            if (request["password"].get<std::string>().size() < 6) {
                response["status"] = "error";
                response["message"] = "비밀번호는 최소 6자리 이상이어야 합니다.";
                return response;
            }

            // 사용자명 중복 확인
            const json& userInfo = userRepo_->findByUsername(request["userName"]);
            if (userInfo["userId"] != -1) {
                response["status"] = "error";
                response["message"] = "이미 존재하는 아이디입니다.";
                return response;
            }

            // PasswordUtil을 사용하여 비밀번호 해싱
            std::string hashedPassword = PasswordUtil::hashPassword(request["password"]);

            // 새 사용자 생성
            int userId = userRepo_->create(request["userName"], hashedPassword);
            if (userId < 0) {
                response["status"] = "error";
                response["message"] = "회원가입에 실패하였습니다.";
                return response;
            }

            // 성공 응답 생성
            response["action"] = "register";
            response["status"] = "success";
            response["message"] = "회원가입에 성공하였습니다.";
            response["userId"] = userId;
            response["userName"] = request["userName"];

            spdlog::info("새로운 유저가 계정을 생성하였습니다, 유저 이름 : {} (ID: {})", request["userName"].get<std::string>(), userId);
            return response;
        }

        json loginUser(const json& request) override {
            json response;

            // 사용자명 유효성 검증
            if (!request.contains("userName") || !request.contains("password")) {
                response["status"] = "error";
                response["message"] = "로그인 요청에 필수 필드가 누락되었습니다.";
                return response;
            }

            // 사용자 찾기
            const json& userInfo = userRepo_->findByUsername(request["userName"]);
            if (userInfo["userId"] == -1) {
                response["status"] = "error";
                response["message"] = "존재하지 않는 사용자입니다.";
                return response;
            }

            // PasswordUtil을 사용하여 비밀번호 검증
            if (!PasswordUtil::verifyPassword(request["password"], userInfo["passwordHash"])) {
                response["status"] = "error";
                response["message"] = "비밀번호가 일치하지 않습니다.";
                return response;
            }

            // 로그인 시간 업데이트
            userRepo_->updateLastLogin(userInfo["userId"]);

            // 성공 응답 생성
            response["action"] = "login";
            response["status"] = "success";
            response["message"] = "로그인에 성공하였습니다.";
            response["userId"] = userInfo["userId"];
            response["userName"] = userInfo["userName"];
            response["nickName"] = userInfo["nickName"];
            response["createdAt"] = userInfo["createdAt"];
            response["lastLogin"] = userInfo["lastLogin"];
            return response;
        }

        json registerCheckAndLogin(const nlohmann::json& request) {
            json response;

            // 사용자명 유효성 검증
            if (!request.contains("userName") || !request.contains("password")) {
                response["status"] = "error";
                response["message"] = "회원가입 여부 확인 및 로그인 요청에 필수 필드가 누락되었습니다.";
                return response;
            }

            // 사용자 찾기
            json userInfo = userRepo_->findByUsername(request["userName"]);
            int userId = -1;
            if (userInfo["userId"] == -1) {
                // PasswordUtil을 사용하여 비밀번호 해싱
                std::string hashedPassword = PasswordUtil::hashPassword(request["password"]);

                // 새 사용자 생성
                userId = userRepo_->create(request["userName"], hashedPassword);
                if (userId < 0) {
                    response["status"] = "error";
                    response["message"] = "사용재 생성에 실패하였습니다.";
                    spdlog::error("새로운 사용자를 생성하는 도중 에러가 발생하였습니다.");
                    return response;
                }
            }

            userInfo = userRepo_->findByUsername(request["userName"]);
            
            // 로그인 시간 업데이트
            userRepo_->updateLastLogin(userId);

            // 성공 응답 생성
            response["action"] = "login";
            response["status"] = "success";
            response["message"] = "로그인에 성공하였습니다.";
            response["userId"] = userInfo["userId"];
            response["userName"] = userInfo["userName"];
            response["nickName"] = userInfo["nickName"];
            response["createdAt"] = userInfo["createdAt"];
            response["lastLogin"] = userInfo["lastLogin"];
            return response;
        }

        json updateNickName(const nlohmann::json& request) {
            json response;

            // 사용자명 유효성 검증
            if (!request.contains("userId") || !request.contains("nickName")) {
                response["status"] = "error";
                response["message"] = "닉네임 변경 요청에 필수 필드가 누락되었습니다.";
                return response;
            }

            if (!isValidNickName(request["nickName"])) {
                response["status"] = "error";
                response["message"] = "잘못된 형식의 닉네임입니다.";
                return response;
            }

            // 사용자 찾기
            if (!userRepo_->updateUserNickName(request["userId"], request["nickName"])) {
                response["status"] = "error";
                response["message"] = "닉네임 변경에 실패하였습니다.";
                return response;
            }

            // 성공 응답 생성
            response["action"] = "updateNickName";
            response["status"] = "success";
            response["message"] = "닉네임을 성공적으로 변경하였습니다.";
            response["nickName"] = request["nickName"];
            spdlog::info("유저 ID : {}가 닉네임을 {}로 변경하였습니다.", request["userId"].get<int>(), request["nickName"].get<std::string>());
            return response;
        }

    private:
        std::shared_ptr<UserRepository> userRepo_;
    };

    // 팩토리 메서드 구현
    std::unique_ptr<AuthService> AuthService::create(std::shared_ptr<UserRepository> userRepo) {
        return std::make_unique<AuthServiceImpl>(userRepo);
    }

} // namespace game_server