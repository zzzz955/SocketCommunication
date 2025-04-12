// util/password_util.cpp
// 비밀번호 유틸리티 구현 파일
// 비밀번호 해싱 및 검증 기능 제공
#include "password_util.h"
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>
#include <spdlog/spdlog.h>

namespace game_server {

    std::string PasswordUtil::hashPassword(const std::string& password) {
        // 보안 참고: 실제 제품에서는 더 안전한 해싱 알고리즘 사용 필요
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(password.c_str()),
            password.length(), hash);

        // 해시를 16진수 문자열로 변환
        std::stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(hash[i]);
        }

        return ss.str();
    }

    bool PasswordUtil::verifyPassword(const std::string& password, const std::string& hashedPassword) {
        // 입력된 비밀번호의 해시와 저장된 해시 비교
        std::string computedHash = hashPassword(password);
        return computedHash == hashedPassword;
    }

} // namespace game_server