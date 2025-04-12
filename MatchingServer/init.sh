#!/bin/bash

# 실패 시 스크립트 중단
set -e

echo "===== 시스템 패키지 업데이트 및 필요한 의존성 설치 ====="
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    pkg-config \
    bison \
    flex \
    libssl-dev \
    zlib1g-dev \
    autoconf \
    automake \
    libtool \
    m4 \
    cmake \
    uuid-dev \
    libpq-dev \
    curl \
    unzip \
    tar \
    python3 \
    python3-pip \
    ninja-build

echo "===== vcpkg 설정 ====="
# 이미 vcpkg가 존재하는 경우 제거
if [ -d "vcpkg" ]; then
    echo "기존 vcpkg 디렉토리 제거 중..."
    rm -rf vcpkg
fi

# vcpkg 클론
echo "vcpkg 저장소 클론 중..."
git clone https://github.com/microsoft/vcpkg.git

# vcpkg 부트스트랩
echo "vcpkg 부트스트랩 실행 중..."
cd vcpkg
./bootstrap-vcpkg.sh
cd ..

# vcpkg로 패키지 설치
echo "vcpkg로 필요한 패키지 설치 중..."
./vcpkg/vcpkg install --triplet=x64-linux

# 환경 변수 설정으로 빌드 시 vcpkg 라이브러리 사용
echo "===== Makefile 실행 ====="
# LD_LIBRARY_PATH에 vcpkg 라이브러리 경로 추가
export LD_LIBRARY_PATH="$PWD/vcpkg_installed/x64-linux/lib:$LD_LIBRARY_PATH"

# 프로젝트 빌드
make

echo "===== 설치 및 빌드 완료 ====="
