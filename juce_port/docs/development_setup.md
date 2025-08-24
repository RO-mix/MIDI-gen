# Настройка среды разработки для JUCE порта

## Предварительные требования

### 1. Visual Studio 2022
**Установленные компоненты:**
- Рабочая нагрузка "Разработка классических приложений на C++"
- Инструменты сборки C++ для Windows
- Windows 10 SDK (последняя версия)
- MSVC v143 - VS 2022 C++ x64/x86 (последняя версия)

### 2. JUCE Framework
**Установлен в:** `D:\PROG\JUCE`
**Версия:** 7.x (рекомендуется последняя стабильная)

## Шаги настройки

### Шаг 1: Открытие проекта в Projucer
1. Запустите `D:\PROG\JUCE\Projucer.exe`
2. Откройте файл `CreativeMIDIGenerator.jucer`
3. Убедитесь, что пути к JUCE указаны правильно:
   - `File → Global Paths...`
   - `JUCE Path`: `D:\PROG\JUCE`

### Шаг 2: Генерация Visual Studio проекта
1. В Projucer выберите `File → Save Project`
2. Выберите `Save and Open in IDE`
3. Projucer автоматически:
   - Сгенерирует `.sln` файл
   - Откроет проект в Visual Studio
   - Настроит пути и зависимости

### Шаг 3: Сборка проекта в Visual Studio
1. В Visual Studio выберите конфигурацию:
   - **Debug** или **Release**
   - **x64** платформа
2. Соберите проект: `Build → Build Solution`
3. Ожидаемые результаты:
   - `.dll` файл плагина в папке `Builds/VisualStudio2019/x64/Debug`
   - `.pdb` файл для отладки

## Структура сгенерированного проекта

```
CreativeMIDIGenerator/
├── Builds/
│   └── VisualStudio2019/
│       ├── x64/
│       │   ├── Debug/
│       │   │   ├── CreativeMIDIGenerator.dll  # Плагин
│       │   │   └── CreativeMIDIGenerator.pdb  # Отладочные символы
│       │   └── Release/
│       │       └── CreativeMIDIGenerator.dll  # Релизная версия
│       └── CreativeMIDIGenerator.sln          # Solution файл
├── JuceLibraryCode/                           # Сгенерированные JUCE файлы
└── Source/                                    # Исходный код
```

## Тестирование плагина

### Регистрация плагина в DAW
1. Скопируйте `.dll` файл в папку плагинов DAW:
   - **Ableton Live:** `C:\ProgramData\Ableton\Live 11 Suite\Resources\MIDI Remote Scripts`
   - **FL Studio:** `C:\Program Files\Image-Line\FL Studio 20\Plugins\VST`
   - **Reaper:** `C:\Program Files\REAPER\Plugins`
   - **Cubase:** `C:\Program Files\Steinberg\Cubase\Components`

2. Перезапустите DAW
3. Добавьте плагин как MIDI эффект

### Отладка
1. В Visual Studio установите breakpoint в `PluginProcessor::processBlock`
2. Запустите отладку: `Debug → Start Debugging`
3. В DAW создайте новый проект и добавьте плагин
4. Проигрывайте MIDI - отладчик должен остановиться на breakpoint

## Распространенные проблемы

### Проблема 1: "Cannot find JUCE modules"
**Решение:**
- Убедитесь, что путь к JUCE указан правильно в Projucer
- Пересохраните проект в Projucer
- Перегенерируйте Visual Studio проект

### Проблема 2: "Missing dependencies"
**Решение:**
- Установите недостающие компоненты Visual Studio
- Проверьте, что Windows 10 SDK установлен

### Проблема 3: Плагин не отображается в DAW
**Решение:**
- Убедитесь, что плагин скопирован в правильную папку
- Проверьте, что DAW поддерживает VST3
- Перезапустите DAW

### Проблема 4: Ошибки компиляции
**Решение:**
- Проверьте, что все пути указаны правильно
- Убедитесь, что Visual Studio обновлен
- Попробуйте Clean и Rebuild

## Переменные среды

Рекомендуется добавить в системные переменные:
```
JUCE_PATH = D:\PROG\JUCE
```

## Полезные ссылки

- [JUCE Documentation](https://juce.com/learn/documentation)
- [JUCE Forum](https://forum.juce.com/)
- [VST3 SDK Documentation](https://steinbergmedia.github.io/vst3_doc/)
- [Microsoft Visual Studio](https://visualstudio.microsoft.com/)

## Следующие шаги после настройки

1. ✅ Проверить сборку проекта
2. ✅ Протестировать плагин в DAW
3. 🔄 Начать разработку EuclideanGenerator
4. 🔄 Реализовать интеграцию с PluginProcessor
5. 🔄 Создать систему тестирования

---

*Этот документ является частью процесса портирования Creative MIDI Generator на JUCE*