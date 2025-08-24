# Скрипт настройки CMake для JUCE проекта
param(
    [string]$BuildDir = "$PSScriptRoot\..\build",
    [string]$JucePath = "D:\PROG\JUCE"
)

Write-Host "=== Настройка CMake для Creative MIDI Generator ===" -ForegroundColor Green
Write-Host "Build Directory: $BuildDir" -ForegroundColor Cyan
Write-Host "JUCE Path: $JucePath" -ForegroundColor Cyan
Write-Host ""

# Проверяем наличие CMake
try {
    $cmakeVersion = & cmake --version
    Write-Host "✅ CMake найден:" -ForegroundColor Green
    Write-Host $cmakeVersion -ForegroundColor Gray
} catch {
    Write-Error "CMake не найден. Установите CMake с https://cmake.org/"
    Write-Host "Или добавьте CMake в PATH" -ForegroundColor Yellow
    exit 1
}

# Проверяем наличие JUCE
if (-not (Test-Path $JucePath)) {
    Write-Error "JUCE не найден по пути: $JucePath"
    exit 1
} else {
    Write-Host "✅ JUCE найден: $JucePath" -ForegroundColor Green
}

# Создаем директорию сборки
if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
    Write-Host "📁 Создана директория сборки: $BuildDir" -ForegroundColor Cyan
} else {
    Write-Host "📁 Директория сборки существует: $BuildDir" -ForegroundColor Cyan
}

# Настраиваем CMake
Write-Host ""
Write-Host "🔧 Настройка CMake..." -ForegroundColor Yellow

$cmakeArgs = @(
    "-S", "$PSScriptRoot\..",
    "-B", $BuildDir,
    "-DJUCE_PATH=$JucePath"
)

try {
    & cmake @cmakeArgs
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ CMake настроен успешно!" -ForegroundColor Green
    } else {
        Write-Error "Ошибка настройки CMake"
        exit 1
    }
} catch {
    Write-Error "Ошибка при запуске CMake: $($_.Exception.Message)"
    exit 1
}

Write-Host ""
Write-Host "=== Следующие шаги ===" -ForegroundColor Green
Write-Host "1. Откройте проект в VSCode: $PSScriptRoot\.." -ForegroundColor Cyan
Write-Host "2. Установите расширение CMake Tools" -ForegroundColor Cyan
Write-Host "3. Соберите проект: Ctrl+Shift+P → 'CMake: Build'" -ForegroundColor Cyan
Write-Host "4. Найдите плагин в: $BuildDir\Release\CreativeMIDIGenerator.dll" -ForegroundColor Cyan

Write-Host ""
Write-Host "Готово! Проект настроен для сборки." -ForegroundColor Green