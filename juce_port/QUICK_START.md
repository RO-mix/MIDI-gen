# Быстрый старт для VSCode

## 🎯 Для пользователей VSCode

### Предварительные требования
- ✅ VSCode установлен
- ✅ Расширение "C/C++" установлено
- ✅ Расширение "CMake Tools" установлено
- ✅ JUCE установлен в `D:\PROG\JUCE`
- ✅ CMake установлен и добавлен в PATH

### Шаг 1: Настройка проекта
```powershell
# Перейди в директорию проекта
cd juce_port\build_scripts

# Запусти настройку CMake
.\setup_cmake.ps1
```

### Шаг 2: Открытие в VSCode
1. Открой VSCode
2. `File → Open Folder...`
3. Выбери папку `juce_port\juce_project`
4. VSCode автоматически настроит проект через CMake

### Шаг 3: Сборка проекта
1. В VSCode нажми `Ctrl+Shift+P`
2. Введи "CMake: Build"
3. Выбери "CMake: Build" из списка
4. Дождись завершения сборки

### Шаг 4: Поиск готового плагина
Плагин будет создан в папке:
```
juce_port\build\Release\CreativeMIDIGenerator.dll
```

## 🔧 Альтернативный способ (терминал)

### Ручная сборка через терминал
```bash
# Перейди в директорию проекта
cd juce_port/juce_project

# Создай директорию сборки
mkdir ../build
cd ../build

# Настрой CMake
cmake ../juce_project -DJUCE_PATH=D:/PROG/JUCE

# Собери проект
cmake --build . --config Release
```

## 📋 Проверка результатов

### Успешная сборка
- ✅ В папке `build/Release/` появится `CreativeMIDIGenerator.dll`
- ✅ Рядом будет `.pdb` файл для отладки

### Возможные проблемы

**CMake не найден:**
```powershell
# Скачай и установи CMake с https://cmake.org/
# Добавь в PATH: C:\Program Files\CMake\bin
```

**JUCE не найден:**
- Проверь путь: `D:\PROG\JUCE`
- Убедись, что папка существует

**Ошибки компиляции:**
- Установи недостающие компоненты Visual Studio
- Проверь, что установлен "Build Tools for Visual Studio"

## 🚀 Тестирование плагина

1. **Скопируй DLL** в папку плагинов DAW
2. **Перезапусти DAW**
3. **Найди плагин** "Creative MIDI Generator"
4. **Протестируй** RandomGenerator

## 📞 Поддержка

Если возникли проблемы:
1. Проверь логи в терминале VSCode
2. Убедись, что все пути указаны правильно
3. Попробуй перезапустить VSCode
4. Сообщи об ошибке для исправления

## 🎵 Следующие шаги

После успешной сборки можно:
- ✅ Тестировать RandomGenerator в DAW
- 🔄 Реализовывать EuclideanGenerator
- 🔄 Добавлять новые функции
- 🔄 Создавать пользовательский интерфейс

---

*Удачной разработки! 🎸*