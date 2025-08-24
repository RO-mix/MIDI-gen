# Скрипт для запуска headless тестов Creative MIDI Generator

param(
    [string]$JucePath = "D:\PROG\JUCE",
    [string]$ProjectPath = "$PSScriptRoot\..\juce_project",
    [string]$BuildPath = "$PSScriptRoot\..\build",
    [switch]$ForceRebuild = $false
)

Write-Host "🎵 Creative MIDI Generator - Headless Test Runner" -ForegroundColor Green
Write-Host "======================================================" -ForegroundColor Green

# Проверяем наличие CMake
try {
    $cmakeVersion = & cmake --version 2>&1
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ CMake найден" -ForegroundColor Green
    } else {
        throw "CMake error"
    }
} catch {
    Write-Error "CMake не найден. Установите CMake с https://cmake.org/"
    Write-Host "Или добавьте в PATH: C:\Program Files\CMake\bin" -ForegroundColor Yellow
    exit 1
}

# Проверяем JUCE
if (-not (Test-Path $JucePath)) {
    Write-Error "JUCE не найден по пути: $JucePath"
    exit 1
} else {
    Write-Host "✅ JUCE найден: $JucePath" -ForegroundColor Green
}

# Создаем директорию сборки
if (-not (Test-Path $BuildPath)) {
    New-Item -ItemType Directory -Path $BuildPath | Out-Null
    Write-Host "📁 Создана директория сборки: $BuildPath" -ForegroundColor Cyan
} elseif ($ForceRebuild) {
    Remove-Item -Path "$BuildPath\*" -Recurse -Force
    Write-Host "🧹 Очищена директория сборки" -ForegroundColor Cyan
}

# Переходим в директорию проекта
Push-Location $ProjectPath

try {
    # Настраиваем CMake если нужно
    if (-not (Test-Path "$BuildPath\CMakeCache.txt") -or $ForceRebuild) {
        Write-Host "🔧 Настройка CMake..." -ForegroundColor Yellow
        $cmakeArgs = @(
            "-S", $ProjectPath,
            "-B", $BuildPath,
            "-DJUCE_PATH=$JucePath",
            "-DCMAKE_BUILD_TYPE=Release"
        )

        $cmakeResult = & cmake @cmakeArgs 2>&1
        if ($LASTEXITCODE -ne 0) {
            Write-Error "Ошибка настройки CMake:"
            Write-Error $cmakeResult
            exit 1
        }
        Write-Host "✅ CMake настроен успешно" -ForegroundColor Green
    }

    # Собираем standalone приложение
    Write-Host "🏗️  Сборка standalone приложения..." -ForegroundColor Yellow
    $buildResult = & cmake --build $BuildPath --target CreativeMIDIGenerator_Standalone --config Release 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Ошибка сборки:"
        Write-Error $buildResult
        exit 1
    }
    Write-Host "✅ Standalone приложение собрано" -ForegroundColor Green

    # Ищем исполняемый файл
    $standaloneExe = Get-ChildItem -Path $BuildPath -Recurse -Filter "*Standalone*.exe" | Select-Object -First 1

    if (-not $standaloneExe) {
        Write-Error "Standalone приложение не найдено в $BuildPath"
        exit 1
    }

    Write-Host "📍 Найден файл: $($standaloneExe.FullName)" -ForegroundColor Cyan

    # Запускаем тесты
    Write-Host "🧪 Запуск headless тестов..." -ForegroundColor Yellow
    Write-Host "=====================================" -ForegroundColor Cyan

    # Создаем лог файл
    $logFile = "$PSScriptRoot\..\headless_test_$(Get-Date -Format 'yyyyMMdd_HHmmss').log"

    # Запускаем приложение с таймаутом
    $startTime = Get-Date
    try {
        $testResult = & $standaloneExe.FullName 2>&1
        $exitCode = $LASTEXITCODE
    } catch {
        Write-Error "Ошибка при запуске тестов: $($_.Exception.Message)"
        exit 1
    }

    $endTime = Get-Date
    $duration = $endTime - $startTime

    # Выводим результаты
    Write-Host $testResult
    Write-Host ""
    Write-Host "📊 Результаты тестирования:" -ForegroundColor Cyan
    Write-Host "   Время выполнения: $($duration.TotalSeconds) сек" -ForegroundColor White
    Write-Host "   Код выхода: $exitCode" -ForegroundColor White
    Write-Host "   Лог сохранен: $logFile" -ForegroundColor White

    # Анализируем результаты
    $passedTests = ($testResult | Select-String -Pattern "PASSED").Count
    $totalTests = ($testResult | Select-String -Pattern "(PASSED|FAILED)").Count

    if ($totalTests -gt 0) {
        $successRate = [math]::Round(($passedTests / $totalTests) * 100, 1)
        Write-Host "   Тестов пройдено: $passedTests/$totalTests ($successRate%)" -ForegroundColor White
    }

    if ($exitCode -eq 0) {
        Write-Host "🎉 Все тесты пройдены успешно!" -ForegroundColor Green
    } else {
        Write-Host "⚠️  Некоторые тесты не пройдены" -ForegroundColor Yellow
    }

    # Сохраняем результаты в лог
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $logContent = @"
=== HEADLESS TEST LOG ===
Timestamp: $timestamp
Duration: $($duration.TotalSeconds) seconds
Exit Code: $exitCode
Tests Passed: $passedTests/$totalTests

=== TEST OUTPUT ===
$testResult

=== SYSTEM INFO ===
CMake: $((cmake --version) -split "`n" | Select-Object -First 1)
JUCE Path: $JucePath
Build Path: $BuildPath
Executable: $($standaloneExe.FullName)
"@

    $logContent | Out-File -FilePath $logFile -Encoding UTF8

    Write-Host ""
    Write-Host "📄 Подробный лог сохранен: $logFile" -ForegroundColor Cyan

    # Выходим с кодом тестов
    exit $exitCode

} finally {
    Pop-Location
}