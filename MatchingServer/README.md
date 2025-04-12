# 스매시업(Smashup) 매칭 서버

[![C++](https://img.shields.io/badge/C++-20-blue.svg)](https://isocpp.org/)
[![Boost](https://img.shields.io/badge/Boost-1.8.0-orange.svg)](https://www.boost.org/)
[![PostgreSQL](https://img.shields.io/badge/PostgreSQL-14-blue.svg)](https://www.postgresql.org/)

스매시업 게임의 매칭 서버 코드입니다. 이 서버는 게임 클라이언트와 통신하며 사용자 인증, 방 생성 및 참가, 게임 세션 관리 등의 기능을 제공합니다.

## 주요 기능

- **사용자 관리**: 회원가입, 로그인, 닉네임 변경
- **방 관리**: 방 생성, 참가, 퇴장, 목록 조회
- **게임 세션**: 게임 시작, 종료 관리
- **미러 서버 연동**: 게임 컨테이너와의 통신

## 기술 스택

- **언어**: C++20
- **네트워킹**: Boost.Asio
- **JSON 파싱**: nlohmann/json
- **로깅**: spdlog
- **데이터베이스**: PostgreSQL (libpqxx)
- **의존성 관리**: vcpkg
- **컨테이너화**: Docker

## 시스템 아키텍처

```
클라이언트 <-> 매칭 서버 <-> 미러 서버 <-> 게임 컨테이너
                    |
                    v
                PostgreSQL
```

### 코드 구조

프로젝트는 MVC 패턴을 따릅니다:

- **컨트롤러(Controller)**: 요청 처리 및 응답 생성
- **서비스(Service)**: 비즈니스 로직 처리
- **리포지토리(Repository)**: 데이터베이스 인터랙션
- **코어(Core)**: 서버 및 세션 관리
- **유틸(Util)**: 비밀번호 해싱, 데이터베이스 풀 등

## 설치 및 실행

### 사전 요구사항

- Ubuntu 22.04 이상
- CMake 3.16 이상
- Docker 및 Docker Compose

### 설치 과정

1. 저장소 클론:
   ```bash
   git clone https://github.com/your-username/smashup-matching-server.git
   cd smashup-matching-server
   ```

2. 의존성 설치:
   ```bash
   chmod +x ./init.sh
   ./init.sh
   ```

3. 빌드:
   ```bash
   chmod +x ./build.sh
   ./build.sh
   ```

4. Docker 컨테이너 실행:
   ```bash
   docker-compose up -d
   ```

## 환경 변수

서버 실행에 필요한 환경 변수:

| 변수명 | 설명 | 기본값 |
|--------|------|--------|
| SERVER_PORT | 서버 실행 포트 | 8080 |
| SERVER_VERSION | 서버 버전 | 1.0.0 |
| DB_HOST | 데이터베이스 호스트 | postgres |
| DB_PORT | 데이터베이스 포트 | 5432 |
| DB_USER | 데이터베이스 사용자 | admin |
| DB_PASSWORD | 데이터베이스 비밀번호 | admin |
| DB_NAME | 데이터베이스 이름 | gamedata |

## 데이터베이스 관리

### 스키마

데이터베이스는 다음 테이블로 구성됩니다:
- users: 사용자 정보
- maps: 맵 정보
- rooms: 방 정보
- room_users: 방 참가자 정보
- games: 게임 세션 정보

### 백업 및 복원

백업:
```bash
sudo docker exec postgres-db pg_dump -U admin -d gamedata > backup_$(date +%Y%m%d_%H%M%S).sql
```

복원:
```bash
cat backup_file.sql | sudo docker exec -i postgres-db psql -U admin -d gamedata
```

## API 문서

서버는 JSON 기반의 소켓 통신을 사용합니다.

### 인증 관련 API

#### 회원가입
```json
{
  "action": "register",
  "userName": "사용자이름",
  "password": "비밀번호"
}
```

#### 로그인
```json
{
  "action": "login",
  "userName": "사용자이름",
  "password": "비밀번호"
}
```

#### 닉네임 변경
```json
{
  "action": "updateNickName",
  "userId": 1,
  "nickName": "새닉네임"
}
```

### 방 관련 API

#### 방 생성
```json
{
  "action": "createRoom",
  "roomName": "방이름",
  "maxPlayers": 8
}
```

#### 방 참가
```json
{
  "action": "joinRoom",
  "roomId": 1
}
```

#### 방 퇴장
```json
{
  "action": "exitRoom"
}
```

#### 방 목록 조회
```json
{
  "action": "listRooms"
}
```

### 게임 관련 API

#### 게임 시작
```json
{
  "action": "gameStart",
  "roomId": 1,
  "mapId": 2
}
```

#### 게임 종료
```json
{
  "action": "gameEnd",
  "gameId": 1
}
```

## 로깅 및 모니터링

서버는 spdlog를 사용하여 다양한 로그 레벨로 정보를 출력합니다:

```bash
# 로그 확인
sudo docker logs matching-server
```

## 문제 해결

### 일반적인 문제

1. **서버 연결 오류**
   - 방화벽 설정 확인
   - 환경 변수 설정 확인

2. **데이터베이스 연결 오류**
   - PostgreSQL 컨테이너 실행 확인
   - 데이터베이스 자격 증명 확인

3. **빌드 오류**
   - vcpkg 의존성 재설치
   - C++ 컴파일러 버전 확인 (C++20 지원 필요)

## 보안 고려사항

- 비밀번호는 SHA-256으로 해싱됩니다.
- 프로덕션 환경에서는 더 강력한 해싱 알고리즘으로 변경 권장
- 환경 변수를 통한 자격 증명 관리
- IP 기반 동시 접속 제한
