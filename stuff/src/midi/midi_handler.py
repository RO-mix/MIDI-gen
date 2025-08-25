import threading
import queue
import mido

class MidiPlayer(threading.Thread):
    """
    The MidiPlayer runs in a dedicated thread to send MIDI messages.
    It pulls messages from a queue and sends them to the selected MIDI output port,
    ensuring that the MIDI communication doesn't block the main GUI thread.
    """
    def __init__(self, midi_queue: queue.Queue):
        super().__init__()
        self.midi_queue = midi_queue
        self.daemon = True
        self._running = threading.Event()
        self.port = None

    @staticmethod
    def get_available_ports():
        """Returns a list of available MIDI output port names."""
        try:
            return mido.get_output_names()
        except Exception as e:
            print(f"Error getting MIDI ports: {e}")
            return []

    def open_port(self, port_name: str):
        """Opens a MIDI output port."""
        if self.port and not self.port.closed:
            self.port.close()
        try:
            self.port = mido.open_output(port_name)
            print(f"Successfully opened MIDI port: {port_name}")
            return True
        except (IOError, ValueError) as e:
            print(f"Error opening MIDI port {port_name}: {e}")
            self.port = None
            return False

    def close_port(self):
        """Closes the currently open MIDI port."""
        if self.port and not self.port.closed:
            self.port.close()
            print("MIDI port closed.")
        self.port = None

    def run(self):
        """
        The main loop of the MIDI player thread.
        Continuously checks the queue for new MIDI messages and sends them.
        """
        self._running.set()
        print("MIDI player thread started.")
        while self._running.is_set():
            try:
                # The `get` call will block until a message is available.
                # A timeout is added to allow the thread to check the _running flag
                # and exit gracefully if stop() has been called.
                message = self.midi_queue.get(timeout=0.1)
                if self.port and not self.port.closed:
                    self.port.send(message)
                    # print(f"Sent MIDI message: {message}") # for debugging
            except queue.Empty:
                # This is expected when the queue is empty.
                # The thread will just loop and wait for the next message.
                continue

        self.close_port()
        print("MIDI player thread stopped.")

    def stop(self):
        """Stops the MIDI player thread."""
        self._running.clear()
