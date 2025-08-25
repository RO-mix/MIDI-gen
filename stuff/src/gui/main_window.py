import dearpygui.dearpygui as dpg
import queue
import atexit
import time
import json
import os
import base64
import tempfile
import logging
import sys
import colorsys
from . import embedded_font

from src.engine.sequencer import Sequencer
from src.midi.midi_handler import MidiPlayer
from src.generators.random_generator import RandomGenerator
from src.generators.euclidean_generator import EuclideanGenerator
from src.generators.dual_euclidean_generator import DualEuclideanGenerator
from src.generators.random_generator_v2 import RandomGeneratorV2
from src.theory.scales import NOTE_NAMES, SCALES, get_notes_in_scale
from src.looper.looper_module import LooperModule

class MainWindow:
    def __init__(self):
        self.logger = logging.getLogger(__name__)
        self.logger.info("Initializing MainWindow...")
        
        try:
            # Core Components & State
            self.logger.debug("Creating queues...")
            self.midi_queue = queue.Queue()
            self.gui_queue = queue.Queue()
            self.command_queue = queue.Queue() # Queue for GUI -> Sequencer commands

            self.logger.debug("Initializing MIDI player...")
            self.midi_player = MidiPlayer(self.midi_queue)

            self.logger.debug("Initializing Looper module...")
            # The looper no longer gets the midi_queue directly.
            self.looper = LooperModule(gui_queue=self.gui_queue)

            self.logger.debug("Initializing sequencer...")
            self.sequencer = Sequencer(
                self.midi_queue,
                gui_queue=self.gui_queue,
                command_queue=self.command_queue,
                looper=self.looper
            )
            self.looper.set_sequencer(self.sequencer)
            
            self.root_note = 0
            self.scale_name = "Major"
            self.available_generators = {
                "Случайный": RandomGenerator,
                "Евклидов": EuclideanGenerator,
                "Двойной Евклидов": DualEuclideanGenerator,
                "Случайный v2.2": RandomGeneratorV2
            }
            self.generator_tag_map = {
                "Случайный": "random",
                "Евклидов": "euclidean",
                "Двойной Евклидов": "dual_euclidean",
                "Случайный v2.2": "random_v2"
            }
            self.current_generator_name = "Случайный"
            self.generator = RandomGenerator()
            self.sequencer.set_generator(self.generator)

            # --- Looper State ---
            self.looper_is_in_split_mode = False
            self.sequencer_was_playing = False
            self.looper_playback_progress = 0.0 # This will be deprecated
            self.last_sequencer_beat = 0.0 # This will be deprecated
            # --- Interpolation State ---
            self.last_sequencer_beat_info = {'beat': 0.0, 'time': time.perf_counter()}
            self.last_looper_progress_info = {'progress': 0.0, 'time': time.perf_counter()}
            # --- Live Timeline State ---
            self.live_notes = [] # A list of note dicts for live view
            self.live_notes_min_pitch = 60 # Default view range
            self.live_notes_max_pitch = 72 # Default view range
            self.timeline_transient_items = [] # For tracking temporary drawings
            self.looper_capture_duration_map = {
                "1/8 такта": 0.5, "1/4 такта": 1.0, "1/2 такта": 2.0,
                "1 такт": 4.0, "2 такта": 8.0, "4 такта": 16.0
            }
            self.looper_recapture_period_map = {
                "Выкл.": 0, "Каждые 2 лупа": 2, "Каждые 3 лупа": 3,
                "Каждые 4 лупа": 4, "Каждые 6 лупов": 6, "Каждые 8 лупов": 8
            }
            self.looper_quantize_map = {
                "Выкл.": 0.0, "1/4": 1.0, "1/8": 0.5, "1/16": 0.25,
                "1/32": 0.125, "1/64": 0.0625
            }
            self.looper_record_length_map = {
                "1 такт": 4.0, "2 такта": 8.0, "4 такта": 16.0, "8 тактов": 32.0,
                "16 тактов": 64.0, "32 такта": 128.0, "64 такта": 256.0
            }
            self.action_quantize_map = {
                "Моментально": "instant",
                "След. 1/2": "half_beat",
                "След. бит": "beat",
                "След. такт": "bar"
            }
            
            # Blinker state
            self.blinker_on_color = (255, 255, 0, 255)
            self.blinker_off_color = (100, 100, 100, 255)
            self.blinker_last_on_time = 0
            self.blinker_duration = 0.1
            self.timeline_height = 200

            self.rate_options = {
                "4 такта": 16.0, "2 такта": 8.0, "1 такт": 4.0,
                "1/2 ноты": 2.0, "1/4 ноты": 1.0, "1/8 ноты": 0.5,
                "1/16 ноты": 0.25, "1/32 ноты": 0.125, "1/64 ноты": 0.0625
            }

            # --- V2 Generator Specific Options ---
            self.duration_options = {
                "4 такта": 16.0,
                "2 такта": 8.0,
                "Целая нота (4/4)": 4.0,
                "Половинная с точкой (3/4)": 3.0,
                "Половинная нота (2/4)": 2.0,
                "Четвертная нота (1/4)": 1.0
            }
            self.acceleration_options = {
                # For 4-bar cycle (16 beats)
                16.0: {
                    "4 такта": 16.0, "2 такта": 8.0, "1 такт": 4.0,
                    "1/2 ноты": 2.0, "1/2 триоль": 4/3, "1/4 ноты": 1.0, "1/4 триоль": 2/3,
                    "1/8 ноты": 0.5, "1/8 триоль": 1/3, "1/16 ноты": 0.25, "1/16 триоль": 1/6, "1/32 ноты": 0.125
                },
                # For 2-bar cycle (8 beats)
                8.0: {
                    "2 такта": 8.0, "1 такт": 4.0,
                    "1/2 ноты": 2.0, "1/2 триоль": 4/3, "1/4 ноты": 1.0, "1/4 триоль": 2/3,
                    "1/8 ноты": 0.5, "1/8 триоль": 1/3, "1/16 ноты": 0.25
                },
                # For 1-bar cycle (4 beats)
                4.0: {
                    "1 такт": 4.0,
                    "1/2 ноты": 2.0, "1/2 триоль": 4/3, "1/4 ноты": 1.0, "1/4 триоль": 2/3,
                    "1/8 ноты": 0.5, "1/8 триоль": 1/3, "1/16 ноты": 0.25, "1/16 триоль": 1/6, "1/32 ноты": 0.125
                },
                # For dotted half cycle (3 beats)
                3.0: {
                    "Половинная с точкой": 3.0, "1/2 ноты": 2.0, "1/4 ноты": 1.0, "1/4 триоль": 2/3,
                    "1/8 ноты": 0.5, "1/8 триоль": 1/3, "1/16 ноты": 0.25
                },
                # For half note cycle (2 beats)
                2.0: {
                    "1/2 ноты": 2.0, "1/4 ноты": 1.0, "1/4 триоль": 2/3,
                    "1/8 ноты": 0.5, "1/8 триоль": 1/3, "1/16 ноты": 0.25
                },
                # For quarter note cycle (1 beat)
                1.0: {
                    "1/4 ноты": 1.0, "1/4 триоль": 2/3, "1/8 ноты": 0.5, "1/8 триоль": 1/3,
                    "1/16 ноты": 0.25, "1/16 триоль": 1/6, "1/32 ноты": 0.125
                }
            }

            # Preset Management
            self.preset_dir = "presets"
            self.preset_files = self._get_preset_filenames()

            self._update_generator_scale_notes()
            atexit.register(self.cleanup)
            
            self.logger.info("Starting MIDI player thread...")
            self.midi_player.start()

            self.logger.info("Starting sequencer thread...")
            self.sequencer.start()

            self.logger.info("MainWindow initialized successfully")
            
        except Exception as e:
            self.logger.error(f"Error during MainWindow initialization: {e}", exc_info=True)
            raise

    def _get_preset_filenames(self):
        """Scans the preset directory and returns a list of preset names."""
        try:
            self.logger.debug(f"Scanning preset directory: {self.preset_dir}")
            if not os.path.exists(self.preset_dir):
                self.logger.info(f"Creating preset directory: {self.preset_dir}")
                os.makedirs(self.preset_dir)
            preset_files = [f for f in os.listdir(self.preset_dir) if f.endswith('.json') and os.path.isfile(os.path.join(self.preset_dir, f))]
            preset_names = sorted([os.path.splitext(f)[0] for f in preset_files])
            self.logger.debug(f"Found presets: {preset_names}")
            return preset_names
        except Exception as e:
            self.logger.error(f"CRITICAL: Could not read preset directory '{self.preset_dir}': {e}", exc_info=True)
            return ["Error: Check permissions"]

    def setup_ui(self):
        dpg.create_context()

        # Явная настройка UTF-8 для Dear PyGui
        try:
            dpg.configure_app(manual_callback_management=False, docking=False)
        except Exception as e:
            self.logger.error(f"Ошибка создания контекста: {e}")

        # Загружаем шрифт ПЕРЕД созданием элементов интерфейса
        self._load_and_bind_font()
        
        self._create_file_dialogs()

        # Создаем и привязываем тему для заголовков
        with dpg.theme(tag="header_theme"):
            with dpg.theme_component(dpg.mvCollapsingHeader):
                # Темно-оранжевый цвет
                dpg.add_theme_color(dpg.mvThemeCol_Header, (204, 85, 0, 255))
                dpg.add_theme_color(dpg.mvThemeCol_HeaderHovered, (214, 105, 20, 255))
                dpg.add_theme_color(dpg.mvThemeCol_HeaderActive, (224, 125, 40, 255))

        with dpg.theme(tag="pattern_step_theme"):
            with dpg.theme_component(dpg.mvSliderFloat):
                dpg.add_theme_color(dpg.mvThemeCol_FrameBg, (204, 85, 0, 150))
                dpg.add_theme_color(dpg.mvThemeCol_FrameBgHovered, (204, 85, 0, 200))
                dpg.add_theme_color(dpg.mvThemeCol_FrameBgActive, (204, 85, 0, 255))
                dpg.add_theme_color(dpg.mvThemeCol_SliderGrab, (255, 255, 255, 255))
                dpg.add_theme_color(dpg.mvThemeCol_SliderGrabActive, (255, 255, 255, 255))
                dpg.add_theme_style(dpg.mvStyleVar_GrabRounding, 0)
                dpg.add_theme_style(dpg.mvStyleVar_FrameRounding, 0)


        with dpg.theme(tag="section_text_theme"):
            with dpg.theme_component(dpg.mvText):
                dpg.add_theme_color(dpg.mvThemeCol_Text, (204, 85, 0, 255), category=dpg.mvThemeCat_Core)

        # Устанавливаем глобальный шрифт до создания элементов
        dpg.set_global_font_scale(1.0)

        with dpg.window(label="Управление", tag="main_window", autosize=True):
            # --- Toolbar ---
            with dpg.group(tag="toolbar_group"):
                # --- Row 1: Playback and MIDI Controls ---
                with dpg.group(horizontal=True):
                    with dpg.drawlist(width=20, height=20):
                        dpg.draw_circle(center=(10, 10), radius=8, color=self.blinker_off_color,
                                        fill=self.blinker_off_color, tag="blinker_circle")
                    dpg.add_button(label="Старт", callback=self.toggle_playback, tag="play_button", width=85)
                    dpg.add_slider_int(label="BPM", tag="ui_bpm", default_value=120, min_value=20,
                                     max_value=300, callback=self.on_bpm_changed, width=150)

                    port_names = self.midi_player.get_available_ports()
                    if not port_names:
                        port_names = ["Нет доступных портов"]
                    midi_port_combo = dpg.add_combo(items=port_names, label="", tag="midi_port_combo",
                                callback=self.on_port_selected, width=130)
                    with dpg.tooltip(midi_port_combo):
                        dpg.add_text("Выбор MIDI-порта для вывода.")
                    midi_channel_slider = dpg.add_slider_int(label="", default_value=1, min_value=1, max_value=16,
                                     callback=lambda s, a: self.sequencer.set_midi_channel(a - 1),
                                     width=100, tag="midi_channel_slider")
                    with dpg.tooltip(midi_channel_slider):
                        dpg.add_text("Выбор MIDI-канала для вывода (1-16).")

                # --- Row 2: Musical Context and Presets ---
                with dpg.group(horizontal=True):
                    dpg.add_combo(NOTE_NAMES, label="Тон", tag="ui_root_note", default_value=NOTE_NAMES[self.root_note],
                                callback=self.on_musical_context_changed, width=60)

                    dpg.add_combo(list(SCALES.keys()), label="Лад", tag="ui_scale_name", default_value=self.scale_name,
                                callback=self.on_musical_context_changed, width=130)

                    dpg.add_combo(items=self.preset_files, label="Пресет", tag="preset_combo",
                                callback=self._on_preset_selected, width=130)
                    dpg.add_button(label="+", callback=lambda: dpg.show_item("save_preset_modal"), width=30)
            
            # Добавляем разделитель
            dpg.add_separator()

            # Остальные элементы управления
            self.create_generator_controls()

            dpg.add_separator()

            self.create_looper_controls()

        # --- Modals ---
        with dpg.window(label="Сохранить новый пресет", modal=True, show=False, tag="save_preset_modal", width=400, height=150, no_resize=True):
            dpg.add_input_text(label="Имя", tag="new_preset_name_input")
            dpg.add_separator()
            with dpg.group(horizontal=True):
                dpg.add_button(label="Сохранить", callback=self._save_new_preset, width=75)
                dpg.add_button(label="Отмена", callback=lambda: dpg.hide_item("save_preset_modal"), width=75)


        dpg.bind_item_theme("main_window", "header_theme")

        # --- Theme for main window padding ---
        with dpg.theme(tag="main_window_padding_theme"):
            with dpg.theme_component(dpg.mvWindowAppItem):
                dpg.add_theme_style(dpg.mvStyleVar_WindowPadding, 10, 10, category=dpg.mvThemeCat_Core)
        dpg.bind_item_theme("main_window", "main_window_padding_theme")

        # Устанавливаем локаль для корректного отображения кириллицы в заголовке
        import locale
        try:
            locale.setlocale(locale.LC_CTYPE, 'Russian_Russia.1251')
        except Exception as e:
            self.logger.warning(f"Не удалось установить локаль: {e}")
            try:
                locale.setlocale(locale.LC_CTYPE, 'ru_RU.UTF-8')
            except Exception as e2:
                self.logger.warning(f"Не удалось установить локаль UTF-8: {e2}")

        # Пробуем разные варианты заголовка для лучшей совместимости
        window_title = "Creative MIDI Generator"
        try:
            # Проверяем, можно ли использовать кириллицу в заголовке
            test_title = "Тест"
            dpg.create_viewport(title=test_title, resizable=True)
            dpg.destroy_viewport()  # Удаляем тестовое окно

            # Если тест прошел успешно, используем русский заголовок
            window_title = "Креативный MIDI Генератор"
        except Exception as e:
            self.logger.warning(f"Кириллица в заголовке не поддерживается: {e}")
            window_title = "Creative MIDI Generator"

        dpg.create_viewport(title=window_title, resizable=True, width=680)

        # Применяем шрифт к заголовку окна, если он доступен
        if hasattr(self, 'main_font') and dpg.does_item_exist(self.main_font):
            try:
                dpg.set_viewport_font(self.main_font)
            except Exception as e:
                self.logger.warning(f"Ошибка применения шрифта к заголовку окна: {e}")

        dpg.set_primary_window("main_window", True)
        dpg.setup_dearpygui()
        dpg.show_viewport()

        # Применяем шрифт ко всем элементам ПОСЛЕ их создания
        if hasattr(self, 'main_font') and dpg.does_item_exist(self.main_font):
            all_items = dpg.get_all_items()
            applied_count = 0
            for item in all_items:
                try:
                    dpg.bind_item_font(item, self.main_font)
                    applied_count += 1
                except:
                    pass

        # Инициализируем MIDI порты после показа окна
        self._refresh_midi_ports()

    def _create_file_dialogs(self):
        # This is now handled by a modal, so the file dialogs are not needed.
        pass

    def _update_preset_combo_ui(self):
        """Refreshes the preset list and updates the UI combo box."""
        self.preset_files = self._get_preset_filenames()
        if dpg.does_item_exist("preset_combo"):
            dpg.configure_item("preset_combo", items=self.preset_files)

    def _on_preset_selected(self, sender, preset_name):
        if not preset_name: return
        filepath = os.path.join(self.preset_dir, f"{preset_name}.json")
        self.load_preset(filepath)

    def _save_new_preset(self, sender=None, app_data=None):
        """Сохраняет новый пресет."""
        preset_name = dpg.get_value("new_preset_name_input")

        if not preset_name or preset_name.isspace():
            print("Ошибка: Имя пресета не может быть пустым.")
            return

        safe_preset_name = "".join(c for c in preset_name if c.isalnum() or c in " -_").strip()
        if not safe_preset_name:
            print("Ошибка: Недопустимое имя пресета.")
            return

        filepath = os.path.join(self.preset_dir, f"{safe_preset_name}.json")

        try:
            self.save_preset(filepath)
            print(f"Пресет сохранен: {safe_preset_name}")
        except Exception as e:
            print(f"Ошибка сохранения пресета: {e}")
            return

        dpg.set_value("new_preset_name_input", "")
        dpg.hide_item("save_preset_modal")
        self._update_preset_combo_ui()
        dpg.set_value("preset_combo", safe_preset_name)

    def create_preset_controls(self):
        with dpg.collapsing_header(label="Пресеты"):
            with dpg.group(horizontal=True):
                combo = dpg.add_combo(items=self.preset_files, label="Пресет", tag="preset_combo", callback=self._on_preset_selected, width=200)
                button = dpg.add_button(label="+", callback=lambda: dpg.show_item("save_preset_modal"), width=30)

                # Применяем шрифт к элементам, если он доступен
                if hasattr(self, 'main_font') and dpg.does_item_exist(self.main_font):
                    try:
                        dpg.bind_item_font(combo, self.main_font)
                        dpg.bind_item_font(button, self.main_font)
                    except Exception as e:
                        self.logger.warning(f"Ошибка применения шрифта к элементам пресетов: {e}")

        # Using a modal window for saving presets
        with dpg.window(label="Сохранить новый пресет", modal=True, show=False, tag="save_preset_modal", width=400, height=150, no_resize=True):
            input_text = dpg.add_input_text(label="Имя", tag="new_preset_name_input")
            separator = dpg.add_separator()
            with dpg.group(horizontal=True):
                save_button = dpg.add_button(label="Сохранить", callback=self._save_new_preset, width=75)
                cancel_button = dpg.add_button(label="Отмена", callback=lambda: dpg.hide_item("save_preset_modal"), width=75)

                # Применяем шрифт к элементам модального окна, если он доступен
                if hasattr(self, 'main_font') and dpg.does_item_exist(self.main_font):
                    try:
                        dpg.bind_item_font(input_text, self.main_font)
                        dpg.bind_item_font(save_button, self.main_font)
                        dpg.bind_item_font(cancel_button, self.main_font)
                    except Exception as e:
                        self.logger.warning(f"Ошибка применения шрифта к элементам модального окна: {e}")

    #_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-
    # Preset Callbacks
    #_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-
    def _refresh_preset_list(self):
        """
        Reloads the preset file list and updates the preset combo box.
        """
        self.preset_files = self._get_preset_filenames()
        dpg.configure_item("preset_combo", items=self.preset_files)

    def _save_preset_callback(self, sender, app_data):
        """
        Callback for the 'Save As' file dialog. Currently not used by any button.
        """
        if 'file_path_name' in app_data:
            self.save_preset(app_data['file_path_name'])
            self._refresh_preset_list()
            preset_name = os.path.splitext(os.path.basename(app_data['file_path_name']))[0]
            if preset_name in self.preset_files:
                 dpg.set_value("preset_combo", preset_name)

    def _load_preset_callback(self, sender, app_data):
        """
        Callback for the 'Load From' file dialog. Currently not used by any button.
        """
        if 'file_path_name' in app_data:
            self.load_preset(app_data['file_path_name'])
            self._refresh_preset_list()
            preset_name = os.path.splitext(os.path.basename(app_data['file_path_name']))[0]
            if preset_name in self.preset_files:
                 dpg.set_value("preset_combo", preset_name)

    # Удаляем дублирующийся метод _save_new_preset_v2
    # Используем только один универсальный метод _save_new_preset

    def _on_preset_selected(self, sender, preset_name):
        """
        Loads the selected preset from the dropdown.
        """
        if not preset_name: return
        filepath = os.path.join(self.preset_dir, f"{preset_name}.json")
        self.load_preset(filepath)

    def create_midi_controls(self):
        # MIDI контролы теперь в тулбаре, этот метод оставлен для совместимости
        pass

    def create_sequencer_controls(self):
        with dpg.collapsing_header(label="СЕКВЕНСОР"):
            with dpg.group(horizontal=True):
                with dpg.drawlist(width=25, height=20):
                    dpg.draw_circle(center=(10, 10), radius=8, color=self.blinker_off_color, fill=self.blinker_off_color, tag="blinker_circle_seq")
                play_button = dpg.add_button(label="Старт", callback=self.toggle_playback, tag="play_button")
            bpm_slider = dpg.add_slider_int(label="BPM", tag="ui_bpm", default_value=120, min_value=20, max_value=300, callback=lambda s, a: self.sequencer.set_bpm(a))
            midi_channel_slider = dpg.add_slider_int(label="MIDI Канал", default_value=1, min_value=1, max_value=16, callback=lambda s, a: self.sequencer.set_midi_channel(a - 1))

            # Применяем шрифт к элементам, если он доступен
            if hasattr(self, 'main_font') and dpg.does_item_exist(self.main_font):
                try:
                    dpg.bind_item_font(play_button, self.main_font)
                    dpg.bind_item_font(bpm_slider, self.main_font)
                    dpg.bind_item_font(midi_channel_slider, self.main_font)
                except Exception as e:
                    self.logger.warning(f"Ошибка применения шрифта к элементам контроллеров секвенсора: {e}")

    def create_generator_controls(self):
        dpg.add_text("ГЕНЕРАТОР", tag="generator_header_text")
        dpg.bind_item_theme("generator_header_text", "section_text_theme")
        with dpg.group():
            generator_combo = dpg.add_combo(list(self.available_generators.keys()), label="", tag="ui_selected_generator", default_value=self.current_generator_name, callback=self.on_generator_selected, width=300)

            # Применяем шрифт к элементам, если он доступен
            if hasattr(self, 'main_font') and dpg.does_item_exist(self.main_font):
                try:
                    dpg.bind_item_font(generator_combo, self.main_font)
                except Exception as e:
                    self.logger.warning(f"Ошибка применения шрифта к элементам контроллеров генератора: {e}")

            self._create_random_generator_ui()
            self._create_euclidean_generator_ui()
            self._create_dual_euclidean_generator_ui()
            self._create_random_generator_v2_ui()

    def _add_duration_bias_slider(self, callback, width=None, label="Длительность"):
        kwargs = {
            "label": label,
            "default_value": 0.5,
            "min_value": 0.0,
            "max_value": 1.0,
            "callback": callback,
            "tag": dpg.generate_uuid()
        }
        if width is not None:
            kwargs["width"] = width

        slider = dpg.add_slider_float(**kwargs)
        with dpg.tooltip(dpg.last_item()):
            tooltip_text = dpg.add_text("Характер длительности: управляет вероятностью появления длинных или коротких нот.\n0.0 = только длинные, 1.0 = только короткие.")

            # Применяем шрифт к элементам, если он доступен
            if hasattr(self, 'main_font') and dpg.does_item_exist(self.main_font):
                try:
                    dpg.bind_item_font(slider, self.main_font)
                    dpg.bind_item_font(tooltip_text, self.main_font)
                except Exception as e:
                    self.logger.warning(f"Ошибка применения шрифта к элементам слайдера длительности: {e}")


    def _create_random_generator_ui(self):
        with dpg.group(tag="Случайный_controls", show=True):
            min_note_slider = dpg.add_slider_int(label="Мин. нота", tag="ui_random_min_note", default_value=60, min_value=0, max_value=127, callback=lambda s, a: self._update_and_refresh_scale('min_note', a))
            with dpg.tooltip(min_note_slider):
                dpg.add_text("Нижняя граница диапазона случайных нот.")
            max_note_slider = dpg.add_slider_int(label="Макс. нота", tag="ui_random_max_note", default_value=72, min_value=0, max_value=127, callback=lambda s, a: self._update_and_refresh_scale('max_note', a))
            with dpg.tooltip(max_note_slider):
                dpg.add_text("Верхняя граница диапазона случайных нот.")
            max_velocity_slider = dpg.add_slider_int(label="Макс. велосити", tag="ui_random_max_velocity", default_value=127, min_value=1, max_value=127, callback=lambda s, a: self.generator.update_params(max_velocity=a))
            with dpg.tooltip(max_velocity_slider):
                dpg.add_text("Максимальная сила нажатия (громкость).")

            velocity_bias_slider = dpg.add_slider_float(label="Характер велосити", tag="ui_random_velocity_bias", default_value=0.5, min_value=0.0, max_value=1.0, callback=lambda s, a: self.generator.update_params(velocity_bias=a))
            with dpg.tooltip(velocity_bias_slider):
                dpg.add_text("Влево = тише, вправо = громче.")

            note_probability_slider = dpg.add_slider_float(label="Вероятность ноты", tag="ui_random_note_probability", default_value=1.0, min_value=0.0, max_value=1.0, callback=lambda s, a: self.generator.update_params(note_probability=a))
            with dpg.tooltip(note_probability_slider):
                dpg.add_text("Вероятность появления ноты на каждом шаге.")

            self._add_duration_bias_slider(lambda s, a: self.generator.update_params(duration_bias=a))

            rate_combo = dpg.add_combo(list(self.rate_options.keys()), label="Частота", tag="ui_random_rate", default_value="1/4 ноты", callback=self.on_rate_changed, width=130)
            with dpg.tooltip(rate_combo):
                dpg.add_text("Как часто генератор пытается создать ноту.")
            cc74_checkbox = dpg.add_checkbox(label="Добавить CC74", tag="ui_random_add_cc74", default_value=False, callback=lambda s, a: self.generator.update_params(add_cc74=a))
            with dpg.tooltip(cc74_checkbox):
                dpg.add_text("Добавляет сообщение CC74 для управления фильтром.")

            # Применяем шрифт к элементам, если он доступен
            if hasattr(self, 'main_font') and dpg.does_item_exist(self.main_font):
                try:
                    dpg.bind_item_font(min_note_slider, self.main_font)
                    dpg.bind_item_font(max_note_slider, self.main_font)
                    dpg.bind_item_font(max_velocity_slider, self.main_font)
                    dpg.bind_item_font(velocity_bias_slider, self.main_font)
                    dpg.bind_item_font(cc74_checkbox, self.main_font)
                except Exception as e:
                    self.logger.warning(f"Ошибка применения шрифта к элементам случайного генератора: {e}")

    def _create_euclidean_generator_ui(self):
        with dpg.group(tag="Евклидов_controls", show=False):
            steps_slider = dpg.add_slider_int(label="Шаги", tag="ui_euclidean_steps", default_value=16, min_value=1, max_value=64, callback=lambda s, a: self.generator.update_params(steps=a))
            with dpg.tooltip(steps_slider):
                dpg.add_text("Общее количество шагов в паттерне.")
            pulses_slider = dpg.add_slider_int(label="Пульсы", tag="ui_euclidean_pulses", default_value=4, min_value=0, max_value=64, callback=lambda s, a: self.generator.update_params(pulses=a))
            with dpg.tooltip(pulses_slider):
                dpg.add_text("Количество нот, равномерно распределенных по шагам.")
            note_slider = dpg.add_slider_int(label="Нота", tag="ui_euclidean_note", default_value=60, min_value=0, max_value=127, callback=lambda s, a: self.on_single_note_changed(s, a, 'note'))
            with dpg.tooltip(note_slider):
                dpg.add_text("Нота, которая будет проигрываться (в статичном режиме).")
            velocity_slider = dpg.add_slider_int(label="Велосити", tag="ui_euclidean_velocity", default_value=100, min_value=1, max_value=127, callback=lambda s, a: self.generator.update_params(velocity=a))
            with dpg.tooltip(velocity_slider):
                dpg.add_text("Сила нажатия для каждой ноты.")

            deviation_slider = dpg.add_slider_int(label="Диапазон отклонения", tag="ui_euclidean_deviation_range", default_value=0, min_value=0, max_value=36, callback=lambda s, a: self.generator.update_params(deviation_range=a))
            with dpg.tooltip(deviation_slider):
                dpg.add_text("Случайное смещение ноты вверх/вниз по ладу (в полутонах).")

            bipolar_checkbox = dpg.add_checkbox(label="Биполярное отклонение", tag="ui_euclidean_deviation_is_bipolar", default_value=False, callback=lambda s, a: self.generator.update_params(deviation_is_bipolar=a))
            with dpg.tooltip(bipolar_checkbox):
                dpg.add_text("Если вкл, отклонение работает в обе стороны (e.g. -5 до +5).\nЕсли выкл, то только вверх (e.g. 0 до +5).")

            rate_combo = dpg.add_combo(list(self.rate_options.keys()), label="Темп шага", tag="ui_euclidean_rate", default_value="1/16 ноты", callback=self.on_rate_changed, width=130)
            with dpg.tooltip(rate_combo):
                dpg.add_text("Длительность каждого шага в секвенсоре.")

            self._add_duration_bias_slider(lambda s, a: self.generator.update_params(duration_bias=a))

            note_probability_slider = dpg.add_slider_float(label="Вероятность ноты", tag="ui_euclidean_note_probability", default_value=1.0, min_value=0.0, max_value=1.0, callback=lambda s, a: self.generator.update_params(note_probability=a))
            with dpg.tooltip(note_probability_slider):
                dpg.add_text("Общая вероятность того, что нота будет сыграна на активном шаге.")

            # Применяем шрифт к элементам, если он доступен
            if hasattr(self, 'main_font') and dpg.does_item_exist(self.main_font):
                try:
                    dpg.bind_item_font(steps_slider, self.main_font)
                    dpg.bind_item_font(pulses_slider, self.main_font)
                    dpg.bind_item_font(note_mode_combo, self.main_font)
                    dpg.bind_item_font(note_slider, self.main_font)
                    dpg.bind_item_font(velocity_slider, self.main_font)
                except Exception as e:
                    self.logger.warning(f"Ошибка применения шрифта к элементам евклидова генератора: {e}")

    def _create_dual_euclidean_generator_ui(self):
        with dpg.group(tag="Двойной_Евклидов_controls", show=False):
            with dpg.group(horizontal=True):
                # --- Column A ---
                with dpg.group(width=235):
                    text_a = dpg.add_text("МАШИНА A")
                    dpg.bind_item_theme(text_a, "section_text_theme")
                    steps_a_slider = dpg.add_slider_int(label="Шаги A", tag="ui_dual_euclidean_steps_a", default_value=16, min_value=1, max_value=64, callback=lambda s, a: self.generator.update_params(steps_a=a), width=-1)
                    with dpg.tooltip(steps_a_slider):
                        dpg.add_text("Общее количество шагов для машины А.")
                    pulses_a_slider = dpg.add_slider_int(label="Пульсы A", tag="ui_dual_euclidean_pulses_a", default_value=4, min_value=0, max_value=64, callback=lambda s, a: self.generator.update_params(pulses_a=a), width=-1)
                    with dpg.tooltip(pulses_a_slider):
                        dpg.add_text("Количество нот для машины А.")
                    note_a_slider = dpg.add_slider_int(label="Нота A", tag="ui_dual_euclidean_note_a", default_value=60, min_value=0, max_value=127, callback=lambda s, a: self.on_single_note_changed(s, a, 'note_a'), width=-1)
                    with dpg.tooltip(note_a_slider):
                        dpg.add_text("Статичная нота для машины А.")
                    velocity_a_slider = dpg.add_slider_int(label="Велосити A", tag="ui_dual_euclidean_velocity_a", default_value=100, min_value=1, max_value=127, callback=lambda s, a: self.generator.update_params(velocity_a=a), width=-1)
                    with dpg.tooltip(velocity_a_slider):
                        dpg.add_text("Сила нажатия для машины А.")
                    self._add_duration_bias_slider(lambda s, a: self.generator.update_params(duration_bias_a=a), width=-1)

                    deviation_a_slider = dpg.add_slider_int(label="Отклонение A", tag="ui_dual_euclidean_deviation_range_a", default_value=0, min_value=0, max_value=36, callback=lambda s, a: self.generator.update_params(deviation_range_a=a), width=-1)
                    with dpg.tooltip(deviation_a_slider):
                        dpg.add_text("Случайное смещение ноты для машины A.")
                    bipolar_a_checkbox = dpg.add_checkbox(label="Биполярное A", tag="ui_dual_euclidean_deviation_is_bipolar_a", default_value=False, callback=lambda s, a: self.generator.update_params(deviation_is_bipolar_a=a))
                    with dpg.tooltip(bipolar_a_checkbox):
                        dpg.add_text("Отклонение для машины А в обе стороны.")

                # --- Column B ---
                with dpg.group(width=235):
                    text_b = dpg.add_text("МАШИНА B")
                    dpg.bind_item_theme(text_b, "section_text_theme")
                    steps_b_slider = dpg.add_slider_int(label="", tag="ui_dual_euclidean_steps_b", default_value=15, min_value=1, max_value=64, callback=lambda s, a: self.generator.update_params(steps_b=a), width=-1)
                    with dpg.tooltip(steps_b_slider):
                        dpg.add_text("Шаги B: Общее количество шагов для машины B.")
                    pulses_b_slider = dpg.add_slider_int(label="", tag="ui_dual_euclidean_pulses_b", default_value=4, min_value=0, max_value=64, callback=lambda s, a: self.generator.update_params(pulses_b=a), width=-1)
                    with dpg.tooltip(pulses_b_slider):
                        dpg.add_text("Пульсы B: Количество нот для машины B.")
                    note_b_slider = dpg.add_slider_int(label="", tag="ui_dual_euclidean_note_b", default_value=67, min_value=0, max_value=127, callback=lambda s, a: self.on_single_note_changed(s, a, 'note_b'), width=-1)
                    with dpg.tooltip(note_b_slider):
                        dpg.add_text("Нота B: Статичная нота для машины B.")
                    velocity_b_slider = dpg.add_slider_int(label="", tag="ui_dual_euclidean_velocity_b", default_value=100, min_value=1, max_value=127, callback=lambda s, a: self.generator.update_params(velocity_b=a), width=-1)
                    with dpg.tooltip(velocity_b_slider):
                        dpg.add_text("Велосити B: Сила нажатия для машины B.")
                    self._add_duration_bias_slider(lambda s, a: self.generator.update_params(duration_bias_b=a), width=-1, label="")

                    deviation_b_slider = dpg.add_slider_int(label="", tag="ui_dual_euclidean_deviation_range_b", default_value=0, min_value=0, max_value=36, callback=lambda s, a: self.generator.update_params(deviation_range_b=a), width=-1)
                    with dpg.tooltip(deviation_b_slider):
                        dpg.add_text("Отклонение B: Случайное смещение ноты для машины B.")
                    bipolar_b_checkbox = dpg.add_checkbox(label="", tag="ui_dual_euclidean_deviation_is_bipolar_b", default_value=False, callback=lambda s, a: self.generator.update_params(deviation_is_bipolar_b=a))
                    with dpg.tooltip(bipolar_b_checkbox):
                        dpg.add_text("Биполярное B: Отклонение для машины B в обе стороны.")

            dpg.add_separator()

            # --- Common Controls ---
            note_probability_slider = dpg.add_slider_float(label="Вероятность ноты", tag="ui_dual_euclidean_note_probability", default_value=1.0, min_value=0.0, max_value=1.0, callback=lambda s, a: self.generator.update_params(note_probability=a))
            with dpg.tooltip(note_probability_slider):
                dpg.add_text("Общая вероятность генерации нот на каждом шаге для обеих машин.")

            rate_combo = dpg.add_combo(list(self.rate_options.keys()), label="Темп шага", tag="ui_dual_euclidean_rate", default_value="1/16 ноты", callback=self.on_rate_changed, width=130)
            with dpg.tooltip(rate_combo):
                dpg.add_text("Общая длительность каждого шага для обеих машин.")

            # Применяем шрифт к элементам, если он доступен
            if hasattr(self, 'main_font') and dpg.does_item_exist(self.main_font):
                try:
                    dpg.bind_item_font(text_a, self.main_font)
                    dpg.bind_item_font(steps_a_slider, self.main_font)
                    dpg.bind_item_font(pulses_a_slider, self.main_font)
                    dpg.bind_item_font(note_mode_a_combo, self.main_font)
                    dpg.bind_item_font(note_a_slider, self.main_font)
                    dpg.bind_item_font(velocity_a_slider, self.main_font)
                    dpg.bind_item_font(text_b, self.main_font)
                    dpg.bind_item_font(steps_b_slider, self.main_font)
                    dpg.bind_item_font(pulses_b_slider, self.main_font)
                    dpg.bind_item_font(note_mode_b_combo, self.main_font)
                    dpg.bind_item_font(note_b_slider, self.main_font)
                    dpg.bind_item_font(velocity_b_slider, self.main_font)
                except Exception as e:
                    self.logger.warning(f"Ошибка применения шрифта к элементам двойного евклидова генератора: {e}")

    def _create_random_generator_v2_ui(self):
        # Create a safe tag by replacing spaces and dots
        with dpg.group(tag="Случайный_v2_2_controls", show=False):
            header1 = dpg.add_text("ВЫБОР НОТ")
            dpg.bind_item_theme(header1, "section_text_theme")
            min_note_slider = dpg.add_slider_int(
                label="Мин. нота", tag="ui_random_v2_min_note",
                default_value=48, min_value=0, max_value=127,
                callback=lambda s, a: self._update_and_refresh_scale('min_note', a))
            max_note_slider = dpg.add_slider_int(
                label="Макс. нота", tag="ui_random_v2_max_note",
                default_value=72, min_value=0, max_value=127,
                callback=lambda s, a: self._update_and_refresh_scale('max_note', a))

            dpg.add_separator()
            header2 = dpg.add_text("ДВИЖОК AMBIENT BURST")
            dpg.bind_item_theme(header2, "section_text_theme")

            burst_prob_slider = dpg.add_slider_float(
                label="Вероятность 'взрыва'", tag="ui_random_v2_burst_probability",
                default_value=0.5, min_value=0.0, max_value=1.0,
                callback=lambda s, a: self.generator.update_params(burst_probability=a))

            note_prob_slider = dpg.add_slider_float(
                label="Общая вероятность ноты", tag="ui_random_v2_note_probability",
                default_value=1.0, min_value=0.0, max_value=1.0,
                callback=lambda s, a: self.generator.update_params(note_probability=a))
            with dpg.tooltip(note_prob_slider):
                dpg.add_text("Глобальный шанс генерации любой ноты, влияет и на 'взрывы', и на одиночные ноты.")

            with dpg.group(horizontal=True):
                base_duration_combo = dpg.add_combo(
                    list(self.duration_options.keys()), label="Длительность",
                    tag="ui_random_v2_base_duration", default_value="Целая нота (4/4)",
                    callback=self.on_base_duration_changed, width=130)
                with dpg.tooltip(base_duration_combo):
                    dpg.add_text("Задает основную длительность цикла, в рамках которого происходят события.")

                acceleration_combo = dpg.add_combo(
                    [], label="Ускорение",
                    tag="ui_random_v2_acceleration_strength",
                    show=True, width=130) # Initially no items, will be populated by callback
                with dpg.tooltip(acceleration_combo):
                    dpg.add_text("Определяет скорость нот во время 'взрыва'.")

            dpg.add_separator()
            header3 = dpg.add_text("ПАТТЕРН 'ВЗРЫВА' (ВЕРОЯТНОСТЬ ДЛЯ КАЖДОГО ИЗ 8 ШАГОВ)")
            dpg.bind_item_theme(header3, "section_text_theme")
            with dpg.group(horizontal=True, tag="ui_random_v2_burst_pattern"):
                # Use a static default pattern for UI creation to avoid dependency on the active generator.
                default_pattern = [1.0, 0.5, 0.8, 0.0, 0.9, 0.4, 0.0, 0.6]
                for i in range(8):
                    # Need a function to capture the index `i`
                    def create_callback(index):
                        return lambda s, a: self.on_burst_pattern_changed(index, a)

                    slider = dpg.add_slider_float(
                        label=f"{i+1}", default_value=default_pattern[i],
                        min_value=0.0, max_value=1.0, width=40, height=40,
                        callback=create_callback(i),
                        tag=f"ui_random_v2_burst_pattern_{i}")
                    dpg.bind_item_theme(slider, "pattern_step_theme")

    def on_musical_context_changed(self, sender, value):
        # This is the direct callback from DearPyGui
        sender_tag = dpg.get_item_alias(sender)
        self._update_musical_context(sender_tag, value)

    def _update_musical_context(self, control_tag, value):
        # This method contains the logic and is easily testable
        if control_tag == "ui_root_note":
            self.root_note = NOTE_NAMES.index(value)
        elif control_tag == "ui_scale_name":
            self.scale_name = value
        self._update_generator_scale_notes()

    def on_bpm_changed(self, sender, bpm):
        """Called when the BPM slider is changed."""
        self.sequencer.set_bpm(bpm)
        self.looper.set_bpm(bpm)

    def on_generator_selected(self, sender, generator_name):
        if generator_name == self.current_generator_name: return
        self.current_generator_name = generator_name
        for name in self.available_generators:
            # Create a safe tag by replacing spaces and dots.
            safe_name = name.replace(' ', '_').replace('.', '_')
            tag_name = f"{safe_name}_controls"
            dpg.configure_item(tag_name, show=(name == generator_name))
        self.generator = self.available_generators[generator_name]()
        self.sequencer.set_generator(self.generator)
        self._update_generator_scale_notes()
        # If switching to V2, we need to initialize its acceleration options
        if generator_name == "Случайный v2.2":
            self.on_base_duration_changed(None, dpg.get_value("ui_random_v2_base_duration"))

    def on_base_duration_changed(self, sender, duration_name):
        """Called when the base_duration combo changes for RandomGeneratorV2."""
        base_duration_val = self.duration_options[duration_name]
        self.generator.update_params(base_duration=base_duration_val)

        # Get the corresponding acceleration options
        accel_options = self.acceleration_options[base_duration_val]
        accel_names = list(accel_options.keys())

        # Update the UI
        dpg.configure_item("ui_random_v2_acceleration_strength", items=accel_names, default_value=accel_names[0])

        # Also update the generator's internal state to match the first new option
        first_accel_val = accel_options[accel_names[0]]
        self.generator.update_params(acceleration_strength=first_accel_val)

        # Create a new callback for the acceleration combo
        def on_accel_changed(s, accel_name):
            accel_val = accel_options[accel_name]
            self.generator.update_params(acceleration_strength=accel_val)

        dpg.set_item_callback("ui_random_v2_acceleration_strength", on_accel_changed)

    def on_burst_pattern_changed(self, index, value):
        """Called when a slider for the burst pattern is changed."""
        if self.current_generator_name == "Случайный v2.2":
            self.generator.burst_pattern[index] = value
            # No need to call update_params, we are modifying the list directly.

    def _update_and_refresh_scale(self, param, value):
        self.generator.update_params(**{param: value})
        self._update_generator_scale_notes()

    def on_single_note_changed(self, sender, value, param_name='note'):
        # The snapping logic was confusing for the user.
        # Now we just pass the raw value directly to the generator.
        self.generator.update_params(**{param_name: value})

    def _update_generator_scale_notes(self):
        if self.current_generator_name == "Случайный v2.2":
            # The new generator has a different scale setting method
            self.generator.set_scale(self.root_note, self.scale_name)
        else:
            # Original behavior for other generators
            notes = get_notes_in_scale(self.root_note, SCALES[self.scale_name])
            self.generator.set_scale_notes(notes)

        # Post-update logic for specific generators
        if self.current_generator_name == "Евклидов":
            self.on_single_note_changed("ui_euclidean_note", self.generator.note, 'note')
        elif self.current_generator_name == "Двойной Евклидов":
            self.on_single_note_changed("ui_dual_euclidean_note_a", self.generator.note_a, 'note_a')
            self.on_single_note_changed("ui_dual_euclidean_note_b", self.generator.note_b, 'note_b')

    def on_rate_changed(self, sender, rate_name):
        """Called when the rate combo for any generator changes."""
        rate_value = self.rate_options[rate_name]
        self.generator.update_params(rate=rate_value)

    def _gather_current_state(self):
        gen_settings = {k: v for k, v in self.generator.__dict__.items() if not k.startswith('_') and 'scale_notes' not in k}

        # Exclude dynamically generated patterns from saving, but include the user-configured burst_pattern
        if self.current_generator_name != "Случайный v2.2":
             gen_settings = {k: v for k, v in gen_settings.items() if 'pattern' not in k}

        looper_notes = []
        if self.looper.loop and self.looper.loop.instruments:
            looper_notes = [{'pitch': n.pitch, 'velocity': n.velocity, 'start': n.start, 'end': n.end} for n in self.looper.loop.instruments[0].notes]

        looper_state = {
            "intensity": self.looper.intensity,
            "auto_recapture_period": self.looper.auto_recapture_period,
            "notes": looper_notes
        }

        return {"version": 2,
                "app_state": {"bpm": self.sequencer.bpm, "root_note": self.root_note, "scale_name": self.scale_name},
                "selected_generator": self.current_generator_name,
                "generator_settings": gen_settings,
                "looper_state": looper_state
               }

    def save_preset(self, fp):
        try:
            with open(fp, 'w') as f: json.dump(self._gather_current_state(), f, indent=4)
        except IOError as e: print(f"Error saving preset: {e}")

    def load_preset(self, fp):
        try:
            with open(fp, 'r') as f: state = json.load(f)
        except (IOError, json.JSONDecodeError) as e:
            print(f"Error loading preset: {e}"); return

        # Apply app state first
        app_state = state.get("app_state", {})
        self.sequencer.set_bpm(app_state.get("bpm", 120))
        self.root_note = app_state.get("root_note", 0)
        self.scale_name = app_state.get("scale_name", "Major")

        # This will create the new generator and call _update_generator_scale_notes
        self.on_generator_selected(None, state.get("selected_generator", "Случайный"))

        # Now apply the specific generator settings
        self.generator.update_params(**state.get("generator_settings", {}))

        # Finally, update the UI to reflect the entire loaded state
        self._update_all_ui_from_state(state)

        # Re-snap notes after params have been loaded
        self._update_generator_scale_notes()

        # Load looper state if it exists
        if "looper_state" in state:
            looper_state = state["looper_state"]
            self.looper.intensity = looper_state.get("intensity", {'bass': 0, 'mid': 0, 'high': 0})
            self.looper.set_auto_recapture_period(looper_state.get("auto_recapture_period", 0))

            loaded_notes = looper_state.get("notes", [])
            if loaded_notes:
                self.looper.load_loop(
                    notes=loaded_notes,
                    bpm=self.sequencer.bpm
                )
            self._looper_update_timeline()


    def _update_all_ui_from_state(self, state):
        # Update global controls
        dpg.set_value("ui_bpm", state["app_state"]["bpm"])
        dpg.set_value("ui_root_note", NOTE_NAMES[state["app_state"]["root_note"]])
        dpg.set_value("ui_scale_name", state["app_state"]["scale_name"])
        dpg.set_value("ui_selected_generator", state["selected_generator"])

        # Update generator controls based on the now-active generator
        gen_settings = state.get("generator_settings", {})
        gen_name = state.get("selected_generator", "Случайный")
        gen_tag_prefix = self.generator_tag_map.get(gen_name, "random")

        # Special handling for V2 generator UI restoration
        if gen_name == "Случайный v2.2":
            # Handle combo boxes by finding the key from the value
            base_duration_val = gen_settings.get("base_duration", 4.0)
            for k, v in self.duration_options.items():
                if v == base_duration_val:
                    dpg.set_value("ui_random_v2_base_duration", k)
                    # Manually trigger the callback to update acceleration options
                    self.on_base_duration_changed(None, k)
                    break

            accel_strength_val = gen_settings.get("acceleration_strength", 1/4)
            # The accel options are now populated by the callback above
            current_accel_options = self.acceleration_options[base_duration_val]
            for k, v in current_accel_options.items():
                if v == accel_strength_val:
                    dpg.set_value("ui_random_v2_acceleration_strength", k)
                    break

            # Handle the burst pattern sliders individually
            burst_pattern = gen_settings.get("burst_pattern", [1.0, 0.5, 0.8, 0.0, 0.9, 0.4, 0.0, 0.6])
            for i, p_val in enumerate(burst_pattern):
                dpg.set_value(f"ui_random_v2_burst_pattern_{i}", p_val)

        # Generic loop for all other parameters and generators
        for param, value in gen_settings.items():
            # Skip params we handled manually for V2
            if gen_name == "Случайный v2.2" and param in ["base_duration", "acceleration_strength", "burst_pattern"]:
                continue

            # Special handling for all rate combo boxes
            if param == 'rate':
                rate_tag = f"ui_{gen_tag_prefix}_rate"
                if dpg.does_item_exist(rate_tag):
                    for k, v in self.rate_options.items():
                        if v == value:
                            dpg.set_value(rate_tag, k)
                            break
                continue

            tag = f"ui_{gen_tag_prefix}_{param}"
            if dpg.does_item_exist(tag):
                dpg.set_value(tag, value)

        # Update looper controls
        looper_state = state.get("looper_state", {})
        recapture_period = looper_state.get("auto_recapture_period", 0)
        for label, period in self.looper_recapture_period_map.items():
            if period == recapture_period:
                if dpg.does_item_exist("looper_recapture_period"):
                    dpg.set_value("looper_recapture_period", label)
                break

        # Sliders for intensity are not stored with tags, so they are not restored here,
        # but their values are set directly on the looper object in load_preset.

    def run(self):
        self.setup_ui()
        while dpg.is_dearpygui_running():
            # Check for messages from the sequencer
            try:
                msg = self.gui_queue.get_nowait()
                if msg == "blink":
                    if dpg.does_item_exist("blinker_circle"):
                        dpg.configure_item("blinker_circle", fill=self.blinker_on_color, color=self.blinker_on_color)
                    elif dpg.does_item_exist("blinker_circle_seq"):
                        dpg.configure_item("blinker_circle_seq", fill=self.blinker_on_color, color=self.blinker_on_color)
                    self.blinker_last_on_time = time.time()
                elif msg[0] == 'recording_started':
                    dpg.bind_item_theme("looper_record_button", "looper_record_button_active_theme")
                elif msg[0] == 'recording_finished':
                    dpg.bind_item_theme("looper_record_button", 0)
                elif msg[0] == 'playback_started':
                    dpg.set_item_label("looper_play_button", "СТОП")
                    dpg.bind_item_theme("looper_play_button", "looper_play_button_active_theme")
                elif msg[0] == 'playback_stopped':
                    dpg.set_item_label("looper_play_button", "PLAY")
                    dpg.bind_item_theme("looper_play_button", 0)
                elif msg == "recapture":
                    self.logger.info("Auto-recapture triggered by looper.")
                    self._do_auto_recapture()
                elif isinstance(msg, tuple) and msg[0] == 'live_note':
                    self._handle_live_note(msg[1])
            except queue.Empty:
                pass

            # Update timeline continuously
            self._looper_update_timeline()

            # Turn blinker off
            if time.time() - self.blinker_last_on_time > self.blinker_duration:
                if dpg.does_item_exist("blinker_circle"):
                    dpg.configure_item("blinker_circle", fill=self.blinker_off_color, color=self.blinker_off_color)
                elif dpg.does_item_exist("blinker_circle_seq"):
                    dpg.configure_item("blinker_circle_seq", fill=self.blinker_off_color, color=self.blinker_off_color)

            dpg.render_dearpygui_frame()

    def toggle_playback(self, s, d):
        if self.sequencer.is_playing():
            self.sequencer.pause()
            dpg.set_item_label(s, "СТАРТ")
        else:
            self.sequencer.play()
            dpg.set_item_label(s, "СТОП")

    def cleanup(self):
        if self.looper.is_playing: self.looper.stop()
        if self.sequencer.is_alive(): self.sequencer.stop();self.sequencer.join(1)
        if self.midi_player.is_alive(): self.midi_player.stop();self.midi_player.join(1)
        if dpg.is_dearpygui_running(): dpg.destroy_context()

    def on_port_selected(self, s, d):
        """Обработчик выбора MIDI порта с защитой от ошибок."""
        try:
            if d and d != "Нет доступных портов":
                self.midi_player.open_port(d)
                print(f"MIDI порт выбран: {d}")
                # Обновляем цвет индикатора при успешном подключении
                if dpg.does_item_exist("blinker_circle_midi"):
                    dpg.configure_item("blinker_circle_midi", fill=(0, 255, 0, 255), color=(0, 255, 0, 255))
                if dpg.does_item_exist("blinker_circle_seq"):
                    dpg.configure_item("blinker_circle_seq", fill=(0, 255, 0, 255), color=(0, 255, 0, 255))
            else:
                print("Предупреждение: Нет доступных MIDI портов")
                if dpg.does_item_exist("blinker_circle_midi"):
                    dpg.configure_item("blinker_circle_midi", fill=(255, 0, 0, 255), color=(255, 0, 0, 255))
                if dpg.does_item_exist("blinker_circle_seq"):
                    dpg.configure_item("blinker_circle_seq", fill=(255, 0, 0, 255), color=(255, 0, 0, 255))
        except Exception as e:
            print(f"Ошибка при выборе MIDI порта: {e}")
            if dpg.does_item_exist("blinker_circle_midi"):
                dpg.configure_item("blinker_circle_midi", fill=(255, 0, 0, 255), color=(255, 0, 0, 255))
            if dpg.does_item_exist("blinker_circle_seq"):
                dpg.configure_item("blinker_circle_seq", fill=(255, 0, 0, 255), color=(255, 0, 0, 255))

    def _load_and_bind_font(self):
        """Загружает шрифт с поддержкой кириллицы."""
        try:
            # Проверка доступных системных шрифтов (в порядке приоритета для кириллицы)
            system_fonts = [
                "C:/Windows/Fonts/segoeui.ttf",  # Segoe UI - отличная поддержка кириллицы
                "C:/Windows/Fonts/tahoma.ttf",   # Tahoma - хорошая поддержка кириллицы
                "C:/Windows/Fonts/verdana.ttf",  # Verdana - хорошая поддержка кириллицы
                "C:/Windows/Fonts/arial.ttf",    # Arial - может не содержать всех кириллических глифов
                "C:/Windows/Fonts/times.ttf"     # Times New Roman
            ]

            font_path = None
            for sys_font in system_fonts:
                exists = os.path.exists(sys_font)
                if exists and not font_path:
                    font_path = sys_font

            # Проверка локальных шрифтов
            local_fonts = [
                "src/tahoma.ttf",
                "tahoma.ttf",
                "../tahoma.ttf"
            ]

            for local_font in local_fonts:
                exists = os.path.exists(local_font)
                if exists and not font_path:
                    font_path = os.path.abspath(local_font)

            if not font_path:
                self.logger.error("Ни один шрифт не найден, используется системный")
                return

            # Увеличиваем размер шрифта для лучшей читаемости
            font_size = 18

            with dpg.font_registry():
                try:
                    # Создаем шрифт с ручным указанием диапазона кириллицы
                    font = dpg.add_font(font_path, font_size)

                    # Проверяем, что шрифт действительно создан
                    if not dpg.does_item_exist(font):
                        self.logger.error("Шрифт не был создан успешно")
                        return

                    # Добавляем диапазон для кириллицы вручную
                    # Basic Cyrillic: U+0400-U+04FF
                    dpg.add_font_range(0x0400, 0x04FF, parent=font)

                    # Добавляем также расширенную кириллицу для полноты
                    # Cyrillic Supplement: U+0500-U+052F
                    dpg.add_font_range(0x0500, 0x052F, parent=font)

                except Exception as e1:
                    self.logger.warning(f"Ошибка при создании шрифта: {e1}")
                    try:
                        # Резервный вариант - только базовый шрифт
                        font = dpg.add_font(font_path, font_size)

                        # Проверяем, что шрифт действительно создан
                        if not dpg.does_item_exist(font):
                            self.logger.error("Шрифт не был создан успешно")
                            return

                    except Exception as e2:
                        self.logger.error(f"Ошибка загрузки шрифта: {e2}")
                        return

            # Применяем шрифт глобально
            dpg.bind_font(font)

            # Сохраняем ссылку на шрифт для последующего использования
            self.main_font = font

            # Принудительно применяем шрифт ко всем существующим элементам
            try:
                # Получаем все элементы интерфейса
                all_items = dpg.get_all_items()

                # Применяем шрифт ко всем элементам
                applied_count = 0
                for item in all_items:
                    try:
                        dpg.bind_item_font(item, font)
                        applied_count += 1
                    except:
                        pass

            except Exception as e:
                self.logger.warning(f"Предупреждение при применении шрифта: {e}")

        except Exception as e:
            self.logger.error(f"Критическая ошибка загрузки шрифта: {e}")

    def _refresh_midi_ports(self):
        """Обновляет список доступных MIDI портов в UI."""
        try:
            port_names = self.midi_player.get_available_ports()
            if not port_names:
                port_names = ["Нет доступных портов"]

            if dpg.does_item_exist("midi_port_combo"):
                dpg.configure_item("midi_port_combo", items=port_names)

            # Автоматически выбираем первый доступный порт
            if port_names and port_names[0] != "Нет доступных портов":
                self.midi_player.open_port(port_names[0])
                dpg.set_value("midi_port_combo", port_names[0])
                print(f"Автоматически выбран MIDI порт: {port_names[0]}")
                if dpg.does_item_exist("blinker_circle_midi"):
                    dpg.configure_item("blinker_circle_midi", fill=(0, 255, 0, 255), color=(0, 255, 0, 255))
                if dpg.does_item_exist("blinker_circle_seq"):
                    dpg.configure_item("blinker_circle_seq", fill=(0, 255, 0, 255), color=(0, 255, 0, 255))
            else:
                print("Предупреждение: Нет доступных MIDI портов")
                if dpg.does_item_exist("blinker_circle_midi"):
                    dpg.configure_item("blinker_circle_midi", fill=(255, 0, 0, 255), color=(255, 0, 0, 255))
                if dpg.does_item_exist("blinker_circle_seq"):
                    dpg.configure_item("blinker_circle_seq", fill=(255, 0, 0, 255), color=(255, 0, 0, 255))

        except Exception as e:
            print(f"Ошибка при обновлении списка MIDI портов: {e}")

    #_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-
    # Looper Callbacks & Methods
    #_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-

    def create_looper_controls(self):
        """Creates the UI controls for the Looper Module v2."""
        dpg.add_text("ЛУПЕРАТОР", tag="looper_header_text")
        dpg.bind_item_theme("looper_header_text", "section_text_theme")

        # --- Row 1: Playback & Quantize ---
        with dpg.group(horizontal=True):
            play_button = dpg.add_button(label="PLAY", callback=self._looper_toggle_play, tag="looper_play_button", width=85)
            with dpg.tooltip(play_button):
                dpg.add_text("Запустить/остановить воспроизведение лупа.")
            through_checkbox = dpg.add_checkbox(label="THR", tag="looper_through_checkbox", default_value=False, callback=self._on_looper_through_changed)
            with dpg.tooltip(through_checkbox):
                dpg.add_text("Пропускать MIDI от генератора напрямую, когда лупер играет.")
            pad_mode_checkbox = dpg.add_checkbox(label="PAD", tag="looper_pad_mode_checkbox", default_value=False, callback=lambda s, a: self.looper.set_pad_mode(a))
            with dpg.tooltip(pad_mode_checkbox):
                dpg.add_text("Режим 'Pad': луп играет до конца и останавливается.")
            double_button = dpg.add_button(label="x2", callback=self._looper_double, width=40)
            with dpg.tooltip(double_button):
                dpg.add_text("Удвоить длину и содержимое лупа.")
            split_button = dpg.add_button(label="/2", callback=self._looper_start_split_mode, width=40)
            with dpg.tooltip(split_button):
                dpg.add_text("Разделить луп пополам.")
            quantize_button = dpg.add_button(label="Квантайз", callback=self._looper_quantize, width=85)
            with dpg.tooltip(quantize_button):
                dpg.add_text("Применить квантование к нотам в лупе согласно выбранной сетке.")
            quantize_grid_combo = dpg.add_combo(list(self.looper_quantize_map.keys()), label="", default_value="Выкл.", width=80, tag="looper_quantize_grid")
            with dpg.tooltip(quantize_grid_combo):
                dpg.add_text("Сетка квантования: Выбрать сетку для квантования нот.")
            variation_button = dpg.add_button(label="Вариация", callback=self._looper_generate_variations, width=85)
            with dpg.tooltip(variation_button):
                dpg.add_text("Создать мелодическую вариацию текущего лупа.")

        # --- Row 2: Capture ---
        with dpg.group(horizontal=True):
            capture_button = dpg.add_button(label="ЗАХВАТ", callback=self._looper_capture, width=85)
            with dpg.tooltip(capture_button):
                dpg.add_text("Захватить следующий паттерн из генератора в луп.")
            capture_duration_combo = dpg.add_combo(list(self.looper_capture_duration_map.keys()), label="", default_value="1 такт", width=130, tag="looper_capture_duration")
            with dpg.tooltip(capture_duration_combo):
                dpg.add_text("Длина захвата: Установить длительность для захвата из генератора.")
            capture_overdub_checkbox = dpg.add_checkbox(label="ОВЕР", tag="looper_capture_overdub_checkbox", default_value=False)
            with dpg.tooltip(capture_overdub_checkbox):
                dpg.add_text("При захвате, новые ноты будут добавлены поверх существующих.")
            recapture_period_combo = dpg.add_combo(list(self.looper_recapture_period_map.keys()), label="Автозахват", default_value="Выкл.", width=130, tag="looper_recapture_period", callback=self._looper_set_recapture_period)
            with dpg.tooltip(recapture_period_combo):
                dpg.add_text("Автоматически перезаписывать луп через заданный интервал.")

        # --- Row 3: Recording ---
        with dpg.group(horizontal=True):
            record_button = dpg.add_button(label="ЗАПИСЬ", callback=self._looper_toggle_record, tag="looper_record_button", width=85)
            with dpg.tooltip(record_button):
                dpg.add_text("Начать/остановить запись MIDI в лупер.")
            record_length_combo = dpg.add_combo(list(self.looper_record_length_map.keys()), label="", default_value="4 такта", width=130, tag="looper_record_length")
            with dpg.tooltip(record_length_combo):
                dpg.add_text("Длина записи: Установить максимальную длину для новой записи.")
            overdub_checkbox = dpg.add_checkbox(label="ОВЕР", tag="looper_overdub_checkbox", default_value=False)
            with dpg.tooltip(overdub_checkbox):
                dpg.add_text("При записи, новые ноты будут добавлены поверх существующих.")
            action_quantize_combo = dpg.add_combo(list(self.action_quantize_map.keys()), label="", default_value="След. такт", width=130, tag="looper_action_quantize")
            with dpg.tooltip(action_quantize_combo):
                dpg.add_text("Квантование действия: Квантовать следующее действие (play, record, и т.д.) к сетке секвенсора.")
            save_button = dpg.add_button(label="Сохранить", callback=self._looper_save, width=85)
            with dpg.tooltip(save_button):
                dpg.add_text("Сохранить текущий луп как MIDI файл.")

        dpg.add_separator()


        dpg.add_slider_float(label="Интенсивность Баса", min_value=0.0, max_value=1.0, callback=lambda s, a: self._looper_set_intensity('bass', a))
        dpg.add_slider_float(label="Интенсивность Середины", min_value=0.0, max_value=1.0, callback=lambda s, a: self._looper_set_intensity('mid', a))
        dpg.add_slider_float(label="Интенсивность Верха", min_value=0.0, max_value=1.0, callback=lambda s, a: self._looper_set_intensity('high', a))

        # --- Timeline Visualization ---
        with dpg.drawlist(width=655, height=self.timeline_height, tag="looper_timeline"):
            # Background
            dpg.draw_rectangle((0, 0), (655, self.timeline_height), color=(20, 20, 20, 255), fill=(20, 20, 20, 255), tag="timeline_bg")
            # Create persistent cursors, initially hidden
            dpg.draw_line((0,0), (0, self.timeline_height), color=(255, 255, 255, 100), thickness=1, tag="playback_cursor", show=False)
            dpg.draw_line((0,0), (0, self.timeline_height), color=(255, 0, 0, 200), thickness=2, tag="record_cursor", show=False)
            dpg.draw_line((655,0), (655, self.timeline_height), color=(255, 255, 255, 100), thickness=1, tag="live_cursor", show=False)

        # --- Item Handlers & Dialogs ---
        with dpg.item_handler_registry(tag="looper_timeline_handler"):
            dpg.add_item_clicked_handler(callback=self._looper_handle_timeline_click)
        dpg.bind_item_handler_registry("looper_timeline", "looper_timeline_handler")

        with dpg.file_dialog(directory_selector=False, show=False, callback=self._looper_save_callback, tag="looper_save_file_dialog", cancel_callback=lambda:dpg.hide_item("looper_save_file_dialog")):
            dpg.add_file_extension(".mid", color=(0, 255, 0, 255))
            dpg.add_file_extension(".*")

        with dpg.theme(tag="looper_play_button_active_theme"):
            with dpg.theme_component(dpg.mvButton):
                dpg.add_theme_color(dpg.mvThemeCol_Button, (0, 150, 0, 255))
                dpg.add_theme_color(dpg.mvThemeCol_ButtonHovered, (0, 200, 0, 255))
                dpg.add_theme_color(dpg.mvThemeCol_ButtonActive, (0, 255, 0, 255))

        with dpg.theme(tag="looper_record_button_active_theme"):
            with dpg.theme_component(dpg.mvButton):
                dpg.add_theme_color(dpg.mvThemeCol_Button, (200, 0, 0, 255))
                dpg.add_theme_color(dpg.mvThemeCol_ButtonHovered, (230, 0, 0, 255))
                dpg.add_theme_color(dpg.mvThemeCol_ButtonActive, (255, 0, 0, 255))

    def _looper_capture(self):
        """Sends a command to the sequencer to capture the generator's output."""
        quantize_label = dpg.get_value("looper_action_quantize")
        quantize_mode = self.action_quantize_map.get(quantize_label, "bar")

        duration_label = dpg.get_value("looper_capture_duration")
        duration_beats = self.looper_capture_duration_map.get(duration_label, 4.0)

        is_through = dpg.get_value("looper_through_checkbox")
        is_overdub = dpg.get_value("looper_capture_overdub_checkbox")

        args = {
            'quantize': quantize_mode,
            'duration_beats': duration_beats,
            'root_note': self.root_note,
            'scale_name': self.scale_name,
            'is_through': is_through,
            'is_overdub': is_overdub
        }
        self.command_queue.put(('looper_capture', args))
        self.logger.info(f"Sent command 'looper_capture' with quantize: {quantize_mode}, overdub: {is_overdub}")

    def _do_auto_recapture(self):
        """Sends a command to the sequencer to perform an instant capture."""
        if not self.looper.loop:
            self.logger.warning("Auto-recapture skipped: no loop to get duration from.")
            return

        # Calculate duration from the existing loop
        duration_beats = self.looper.loop_duration_beats

        is_through = dpg.get_value("looper_through_checkbox")
        is_overdub = dpg.get_value("looper_capture_overdub_checkbox")
        args = {
            'quantize': 'instant',
            'duration_beats': duration_beats,
            'root_note': self.root_note,
            'scale_name': self.scale_name,
            'is_through': is_through,
            'is_overdub': is_overdub
        }
        self.command_queue.put(('looper_capture', args))
        self.logger.info(f"Sent command 'looper_capture' for auto-recapture of {duration_beats:.2f} beats.")

    def _looper_double(self):
        """Sends a command to the sequencer to double the loop."""
        quantize_label = dpg.get_value("looper_action_quantize")
        quantize_mode = self.action_quantize_map.get(quantize_label, "bar")

        args = {'quantize': quantize_mode}
        self.command_queue.put(('looper_double', args))
        self.logger.info(f"Sent command 'looper_double' with quantize: {quantize_mode}")

    def _looper_generate_variations(self):
        """Callback to generate variations and update the timeline."""
        self.looper.generate_variations()
        self._looper_update_timeline()

    def _looper_toggle_record(self):
        """Sends a command to the sequencer to start or stop recording."""
        quantize_label = dpg.get_value("looper_action_quantize")
        quantize_mode = self.action_quantize_map.get(quantize_label, "bar")

        is_overdub = dpg.get_value("looper_overdub_checkbox")
        length_label = dpg.get_value("looper_record_length")
        length_beats = self.looper_record_length_map.get(length_label, 16.0)
        is_through = dpg.get_value("looper_through_checkbox")

        args = {
            'quantize': quantize_mode,
            'is_overdub': is_overdub,
            'length_beats': length_beats,
            'is_through': is_through
        }
        self.command_queue.put(('looper_toggle_record', args))
        self.logger.info(f"Sent command 'looper_toggle_record' with quantize: {quantize_mode}")

    def _on_looper_through_changed(self, sender, app_data):
        """Called when the 'THR' checkbox is changed."""
        is_through = dpg.get_value(sender)
        # This now simply mutes/unmutes the sequencer if the looper is playing.
        # The command-based play logic handles the initial mute.
        if self.looper.is_playing():
            self.sequencer.set_muted(not is_through)

    def _looper_toggle_play(self):
        """Sends a command to the sequencer to toggle looper playback."""
        # Optimistic UI update for the label to give instant feedback
        if self.looper.is_playing():
            dpg.set_item_label("looper_play_button", "PLAY")
        else:
            dpg.set_item_label("looper_play_button", "СТОП")

        quantize_label = dpg.get_value("looper_action_quantize")
        quantize_mode = self.action_quantize_map.get(quantize_label, "bar")
        is_through = dpg.get_value("looper_through_checkbox")

        args = {
            'quantize': quantize_mode,
            'is_through': is_through
        }
        self.command_queue.put(('looper_toggle_play', args))
        self.logger.info(f"Sent command 'looper_toggle_play' with quantize: {quantize_mode}")

    def _looper_set_recapture_period(self, sender, app_data):
        """Sets the auto-recapture period on the looper module."""
        period_label = dpg.get_value(sender)
        period_value = self.looper_recapture_period_map.get(period_label, 0)
        self.looper.set_auto_recapture_period(period_value)

    def _looper_start_split_mode(self):
        """Sends a command to the sequencer to schedule a loop split."""
        if not self.looper.loop:
            self.logger.warning("Cannot schedule split: loop is empty.")
            return

        quantize_label = dpg.get_value("looper_action_quantize")
        quantize_mode = self.action_quantize_map.get(quantize_label, "bar")

        args = {'quantize': quantize_mode}
        self.command_queue.put(('looper_schedule_split', args))
        self.logger.info(f"Sent command 'looper_schedule_split' with quantize: {quantize_mode}")

    def _looper_handle_timeline_click(self, sender, app_data):
        """Handles clicks on the timeline, primarily for the split function."""
        if not self.looper_is_in_split_mode:
            return

        # app_data[1] is the mouse position relative to the item
        mouse_pos = dpg.get_mouse_pos(local=True)
        width = 655 # must match drawlist width

        keep_first_half = mouse_pos[0] < (width / 2)

        self.looper.split_loop(keep_first_half=keep_first_half)
        self.looper_is_in_split_mode = False
        self._looper_update_timeline()

    def _looper_set_intensity(self, band, value):
        """Updates the intensity for a specific frequency band."""
        self.looper.intensity[band] = value

    def _looper_save(self):
        """Shows the save file dialog for the looper."""
        dpg.show_item("looper_save_file_dialog")

    def _looper_save_callback(self, sender, app_data):
        """Callback for the looper's save file dialog."""
        if 'file_path_name' in app_data:
            try:
                self.looper.save(app_data['file_path_name'])
                self.logger.info(f"Looper MIDI saved to {app_data['file_path_name']}")
            except Exception as e:
                self.logger.error(f"Failed to save looper MIDI: {e}")
        dpg.hide_item("looper_save_file_dialog")

    def _handle_live_note(self, note_info):
        """Processes a live note from the sequencer for visualization."""
        # Add end_beat to the dictionary for timeline drawing
        note_info['end_beat'] = note_info['start_beat'] + note_info['duration_beats']

        # Trim old notes that are way off-screen
        view_window_beats = 16.0
        cutoff_beat = self.last_sequencer_beat - (view_window_beats * 2)
        self.live_notes = [n for n in self.live_notes if n['end_beat'] > cutoff_beat]

        # Add new note
        self.live_notes.append(note_info)

        # Update vertical zoom range
        if note_info['pitch'] > self.live_notes_max_pitch:
            self.live_notes_max_pitch = note_info['pitch']
        if note_info['pitch'] < self.live_notes_min_pitch:
            self.live_notes_min_pitch = note_info['pitch']

    def _draw_note_on_timeline(self, drawlist_tag, x_start, x_end, y_pos, velocity, note_thickness=4, shadow_offset=1):
        """Helper function to draw a single note with shadow and velocity-based color."""
        # Ensure even the shortest note has a visible width of at least 1 pixel.
        x_end = max(x_end, x_start + 1.0)

        shadow_color = (10, 10, 10, 150)

        hue = 0.083  # Orange
        saturation = 0.9
        brightness = 0.5 + (velocity / 127.0) * 0.5
        rgb_float = colorsys.hsv_to_rgb(hue, saturation, brightness)
        color = [int(c * 255) for c in rgb_float]

        shadow_item = dpg.draw_rectangle(
            (x_start + shadow_offset, y_pos + shadow_offset),
            (x_end + shadow_offset, y_pos + note_thickness + shadow_offset),
            color=shadow_color, fill=shadow_color, parent=drawlist_tag
        )
        note_item = dpg.draw_rectangle(
            (x_start, y_pos),
            (x_end, y_pos + note_thickness),
            color=color, fill=color, parent=drawlist_tag
        )
        self.timeline_transient_items.extend([shadow_item, note_item])

    def _draw_timeline_grid(self, drawlist_tag, width, height, total_beats, grid_resolution):
        """Draws beat and bar lines on a timeline, synchronized with quantization."""
        if total_beats <= 0:
            return

        num_bars = int(total_beats / 4)
        if num_bars == 0 and total_beats > 0: num_bars = 1
        for i in range(1, num_bars + 1):
            bar_beat = i * 4
            if bar_beat < total_beats:
                 x = (bar_beat / total_beats) * width
                 bar_line = dpg.draw_line((x, 0), (x, height), color=(80, 80, 80, 255), thickness=2, parent=drawlist_tag)
                 self.timeline_transient_items.append(bar_line)

        if grid_resolution > 0:
            num_grid_lines = int(total_beats / grid_resolution)
            for i in range(1, num_grid_lines):
                line_beat = i * grid_resolution
                if line_beat % 4 == 0:
                    continue

                x = (line_beat / total_beats) * width
                is_beat_line = line_beat % 1 == 0
                color = (60, 60, 60, 255) if is_beat_line else (40, 40, 40, 255)
                thickness = 1
                grid_line = dpg.draw_line((x, 0), (x, height), color=color, thickness=thickness, parent=drawlist_tag)
                self.timeline_transient_items.append(grid_line)


    def _get_timeline_mode(self):
        if self.looper.is_recording:
            return 'RECORDING'
        if self.looper.is_playing():
            return 'PLAYBACK'
        return 'LIVE'

    def _looper_update_timeline(self):
        """Redraws the looper's piano roll visualization based on the current mode."""
        drawlist_tag = "looper_timeline"
        if not dpg.does_item_exist(drawlist_tag): return

        # --- Direct State "Pull" Model ---
        # Instead of interpolating from messages, we pull the current state directly.
        # This is thread-safe for reading simple types like floats and booleans.
        current_beat = self.sequencer.current_beat

        # --- Drawing Logic ---
        # Clear old transient drawings (notes, grid lines)
        for item in self.timeline_transient_items:
            if dpg.does_item_exist(item):
                dpg.delete_item(item)
        self.timeline_transient_items.clear()

        width, height = 655, self.timeline_height
        note_thickness = 5
        grid_label = dpg.get_value("looper_quantize_grid")
        grid_resolution = self.looper_quantize_map.get(grid_label, 0.0)
        visual_grid_resolution = grid_resolution if grid_resolution > 0 else 0.5
        timeline_mode = self._get_timeline_mode()

        # Hide all cursors by default, we will show the correct one later
        dpg.configure_item("playback_cursor", show=False)
        dpg.configure_item("record_cursor", show=False)
        dpg.configure_item("live_cursor", show=False)

        # --- RECORDING VIEW ---
        if timeline_mode == 'RECORDING':
            total_beats = self.looper.record_length_beats
            if total_beats <= 0: return
            self._draw_timeline_grid(drawlist_tag, width, height, total_beats, visual_grid_resolution)

            all_pitches = [n['pitch'] for n in self.looper.recorded_notes] if self.looper.recorded_notes else []
            min_pitch = min(all_pitches) - 2 if all_pitches else 24
            max_pitch = max(all_pitches) + 2 if all_pitches else 108
            pitch_range = max_pitch - min_pitch
            if pitch_range <= 0: pitch_range = 1

            for note in self.looper.recorded_notes:
                note_start_in_loop = note['start_beat'] - self.looper.recording_start_beat
                note_end_in_loop = note['end_beat'] - self.looper.recording_start_beat
                x_start = (note_start_in_loop / total_beats) * width
                x_end = (note_end_in_loop / total_beats) * width
                y_pos = height - ((note['pitch'] - min_pitch) / pitch_range) * height if pitch_range > 0 else height / 2
                y_pos = max(0, min(height - note_thickness, y_pos))
                self._draw_note_on_timeline(drawlist_tag, x_start, x_end, y_pos, note['velocity'], note_thickness)

            # Use current beat for the cursor
            progress_beats = current_beat - self.looper.recording_start_beat
            x_pos = (progress_beats / total_beats) * width
            dpg.configure_item("record_cursor", p1=(x_pos, 0), p2=(x_pos, height), show=True)

        # --- PLAYBACK VIEW ---
        elif timeline_mode == 'PLAYBACK':
            total_beats = self.looper.loop_duration_beats
            if total_beats <= 0: return

            # Calculate current loop position based on the current master beat
            elapsed_beats = current_beat - self.looper.loop_start_beat
            current_loop_pos_beats = elapsed_beats % total_beats

            # Define a visible window, e.g., 16 beats.
            view_window_beats = 16.0
            display_beats = total_beats
            window_start_beat = 0.0

            # If the total loop is larger than the window, create a scrolling view.
            if total_beats > view_window_beats:
                display_beats = view_window_beats
                # Center the playhead in the window.
                window_start_beat = current_loop_pos_beats - (view_window_beats / 2)
                # Clamp the window to the loop boundaries.
                window_start_beat = max(0, window_start_beat)
                if window_start_beat + view_window_beats > total_beats:
                    window_start_beat = total_beats - view_window_beats

            self._draw_timeline_grid(drawlist_tag, width, height, display_beats, visual_grid_resolution)

            if self.looper.loop:
                # Filter notes to only those visible in the current window
                window_end_beat = window_start_beat + display_beats
                visible_notes = [n for n in self.looper.loop if n['end_beat'] > window_start_beat and n['start_beat'] < window_end_beat]

                all_pitches = [n['pitch'] for n in visible_notes] if visible_notes else []
                min_pitch = min(all_pitches) - 2 if all_pitches else 24
                max_pitch = max(all_pitches) + 2 if all_pitches else 108
                pitch_range = max_pitch - min_pitch
                if pitch_range <= 0: pitch_range = 1

                for note in visible_notes:
                    # Calculate position relative to the window's start
                    note_start_in_window = note['start_beat'] - window_start_beat
                    note_end_in_window = note['end_beat'] - window_start_beat

                    x_start = (note_start_in_window / display_beats) * width
                    x_end = (note_end_in_window / display_beats) * width

                    y_pos = height - ((note['pitch'] - min_pitch) / pitch_range) * height if pitch_range > 0 else height / 2
                    y_pos = max(0, min(height - note_thickness, y_pos))
                    self._draw_note_on_timeline(drawlist_tag, x_start, x_end, y_pos, note['velocity'], note_thickness)

            # Calculate cursor position relative to the window
            cursor_pos_in_window_beats = current_loop_pos_beats - window_start_beat
            x_pos = (cursor_pos_in_window_beats / display_beats) * width

            if 0 <= x_pos <= width:
                dpg.configure_item("playback_cursor", p1=(x_pos, 0), p2=(x_pos, height), show=True)

        # --- LIVE VIEW ---
        else: # LIVE
            view_window_beats = 16.0
            self._draw_timeline_grid(drawlist_tag, width, height, view_window_beats, visual_grid_resolution)

            current_beat_floored = int(current_beat)
            start_beat = current_beat_floored - (current_beat_floored % view_window_beats)
            visible_notes = [n for n in self.live_notes if n['end_beat'] > start_beat and n['start_beat'] < start_beat + view_window_beats]
            if visible_notes:
                self.live_notes_min_pitch = min(n['pitch'] for n in visible_notes) - 2
                self.live_notes_max_pitch = max(n['pitch'] for n in visible_notes) + 2

            min_pitch = self.live_notes_min_pitch
            max_pitch = self.live_notes_max_pitch
            pitch_range = max_pitch - min_pitch
            if pitch_range <= 0: pitch_range = 24

            for note in self.live_notes:
                # Use current beat for scrolling notes
                x_start = ((note['start_beat'] - current_beat + view_window_beats) / view_window_beats) * width
                x_end = ((note['end_beat'] - current_beat + view_window_beats) / view_window_beats) * width
                if x_end < 0 or x_start > width: continue
                y_pos = height - ((note['pitch'] - min_pitch) / pitch_range) * height if pitch_range > 0 else height / 2
                y_pos = max(0, min(height - note_thickness, y_pos))
                self._draw_note_on_timeline(drawlist_tag, x_start, x_end, y_pos, note['velocity'], note_thickness)

            dpg.configure_item("live_cursor", show=True) # Live cursor is always at the end

    def _looper_draw_split_overlay(self):
        """Draws a visual overlay on the timeline to indicate split mode."""
        width, height = 580, self.timeline_height
        midpoint_x = width / 2

        # Highlight first half
        dpg.draw_rectangle((0, 0), (midpoint_x, height), color=(255, 255, 0, 50), fill=(255, 255, 0, 50), parent="looper_timeline")
        # Highlight second half
        dpg.draw_rectangle((midpoint_x, 0), (width, height), color=(0, 255, 255, 50), fill=(0, 255, 255, 50), parent="looper_timeline")

    def _looper_quantize(self):
        """Applies or reverts quantization based on the selected grid."""
        grid_label = dpg.get_value("looper_quantize_grid")
        grid_resolution = self.looper_quantize_map.get(grid_label, 0.0)

        # If grid is 'Off', revert to the pristine loop
        if grid_resolution == 0.0:
            self.looper.unquantize_notes()
            self.logger.info("Reverted to unquantized loop.")
        else:
            # Otherwise, apply quantization from the pristine loop
            self.looper.quantize_notes(grid_resolution)
            self.logger.info(f"Applied {grid_label} quantization.")

        # Always update the timeline to show the result
        self._looper_update_timeline()
