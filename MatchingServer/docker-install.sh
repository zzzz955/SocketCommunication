#!/bin/bash

# 실패 시 스크립트 중단
set -e

echo "===== Docker 설치 시작 ====="

# 이전 버전 제거 (이미 설치된 경우)
sudo apt-get remove -y docker docker-engine docker.io containerd runc || true

# 필요한 패키지 설치
sudo apt-get update
sudo apt-get install -y \
    apt-transport-https \
    ca-certificates \
    curl \
    gnupg \
    lsb-release

# Docker의 공식 GPG 키 추가
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo gpg --dearmor -o /usr/share/keyrings/docker-archive-keyring.gpg

# Docker 저장소 설정
echo \
  "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/docker-archive-keyring.gpg] https://download.docker.com/linux/ubuntu \
  $(lsb_release -cs) stable" | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null

# Docker 엔진 설치
sudo apt-get update
sudo apt-get install -y docker-ce docker-ce-cli containerd.io

# 현재 사용자를 docker 그룹에 추가 (sudo 없이 Docker 사용 가능)
sudo usermod -aG docker $USER

# Docker Compose 설치
DOCKER_COMPOSE_VERSION=$(curl -s https://api.github.com/repos/docker/compose/releases/latest | grep 'tag_name' | cut -d\" -f4)
sudo curl -L "https://github.com/docker/compose/releases/download/${DOCKER_COMPOSE_VERSION}/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose
sudo chmod +x /usr/local/bin/docker-compose

# Docker 시작 및 부팅 시 자동 시작 설정
sudo systemctl start docker
sudo systemctl enable docker

echo "===== Docker 설치 완료 ====="
echo "Docker 버전:"
docker --version
echo "Docker Compose 버전:"
docker-compose --version
echo "Docker 그룹에 사용자 추가됨. 변경사항을 적용하려면 재로그인하거나 다음 명령을 실행하세요:"
echo "newgrp docker"
