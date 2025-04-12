#!/bin/bash

# 색상 코드
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

echo "===== 빌드 시작 ====="

# 빌드 디렉토리 정리
if [ -d "build" ]; then
    echo "기존 빌드 디렉토리 정리 중..."
    make clean
fi

# vcpkg 의존성 변경 확인
if [ -f "vcpkg.json" ]; then
    VCPKG_HASH=$(md5sum vcpkg.json | awk '{print $1}')
    VCPKG_HASH_FILE=".vcpkg_hash"
    
    if [ -f "$VCPKG_HASH_FILE" ] && [ "$(cat $VCPKG_HASH_FILE)" = "$VCPKG_HASH" ]; then
        echo "vcpkg 의존성 변경 없음, 설치 건너뜀..."
    else
        echo "vcpkg 의존성 변경 감지, 설치 중..."
        ./vcpkg/vcpkg install --triplet=x64-linux
        echo "$VCPKG_HASH" > "$VCPKG_HASH_FILE"
    fi
fi

# 빌드 실행
echo "프로젝트 빌드 중..."
make -j$(nproc)

# 빌드 결과 확인
if [ $? -eq 0 ]; then
    echo -e "${GREEN}빌드 성공!${NC}"
    echo "실행 파일 위치: ./build/bin/MatchingServer"
else
    echo -e "${RED}빌드 실패${NC}"
    exit 1
fi

echo "===== 빌드 완료 ====="
echo "매칭 서버 도커 컨테이너 실행 중..."
sudo docker-compose down && sudo docker-compose up -d --build

echo "미러 서버 컨테이너 실행 중..."
cd /home/ubuntu/smashup-server/
git pull
sudo docker-compose down && sudo docker-compose up -d --build

echo "도커 컨테이너 실행 완료"
cd
cd ./BackEnd_Doc/MatchingServer
