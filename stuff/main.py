from src.gui.main_window import MainWindow

import logging
import sys

# Настройка логирования
logging.basicConfig(
    level=logging.DEBUG,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        # logging.FileHandler('midi_generator.log', encoding='utf-8'), # Disabled as per user request
        logging.StreamHandler(sys.stdout)
    ]
)

def main():
    """
    Main function to initialize and run the Creative MIDI Generator.
    """
    logger = logging.getLogger(__name__)
    try:
        logger.info("Starting Creative MIDI Generator...")
        app = MainWindow()
        app.run()
        logger.info("Application has been closed successfully.")
    except Exception as e:
        logger.error(f"Critical error in main: {e}", exc_info=True)
        raise

if __name__ == "__main__":
    main()
