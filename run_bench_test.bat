@echo off
chcp 65001 >nul
title Motor Driver Bench Test
echo.
echo ╔════════════════════════════════════════════════════════════╗
echo ║  Motor Driver Bench Test — One-Click Runner               ║
echo ╚════════════════════════════════════════════════════════════╝
echo.
echo ⚠  시리얼 모니터가 열려있으면 먼저 닫아주세요!
echo     (PlatformIO Serial Monitor 등)
echo.
pause

set PROJECT_DIR=%~dp0

echo.
echo 🧪 벤치 테스트 실행 중... (약 3~5분 소요)
echo    데이터 수집 + 그래프 생성 + HTML 리포트
echo.
python "%PROJECT_DIR%motor_bench_analyzer.py" COM6 Ba
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ⚠ 오류 발생. 포트 확인 후 수동 실행:
    echo   python motor_bench_analyzer.py COM6 Ba
    echo.
)
pause
