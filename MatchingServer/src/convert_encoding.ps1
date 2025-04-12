# 현재 디렉토리를 기준으로 상대 경로 사용
$projectPath = "."  # 현재 디렉토리

# 또는 스크립트가 있는 디렉토리를 기준으로 할 경우
# $projectPath = $PSScriptRoot

# 변환할 확장자 (필요에 따라 추가)
$extensions = @("*.cpp", "*.h", "*.hpp")

# 모든 해당 파일 찾기
foreach ($ext in $extensions) {
    $files = Get-ChildItem -Path $projectPath -Filter $ext -Recurse
    
    foreach ($file in $files) {
        # 파일 내용 읽기
        $content = Get-Content -Path $file.FullName -Raw
        
        # UTF-8로 다시 쓰기
        $content | Out-File -FilePath $file.FullName -Encoding utf8
    }
}

Write-Host "인코딩 변환이 완료되었습니다."