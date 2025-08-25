import pytest
import queue
import mido
import time
from src.midi.midi_handler import MidiPlayer

def test_get_available_ports_returns_list_on_mido_success(mocker):
    """
    Tests that get_available_ports returns a list of ports on success.
    """
    mocker.patch('src.midi.midi_handler.mido.get_output_names', return_value=['port1', 'port2'])
    ports = MidiPlayer.get_available_ports()
    assert ports == ['port1', 'port2']

def test_get_available_ports_returns_empty_list_on_mido_exception(mocker):
    """
    Tests that get_available_ports returns an empty list on failure.
    """
    mocker.patch('src.midi.midi_handler.mido.get_output_names', side_effect=Exception("MIDI Error"))
    ports = MidiPlayer.get_available_ports()
    assert ports == []

def test_open_port_returns_true_and_stores_port_on_success(mocker):
    """
    Tests that a port is successfully opened and stored.
    """
    mock_port = mocker.MagicMock()
    mock_open = mocker.patch('src.midi.midi_handler.mido.open_output', return_value=mock_port)
    player = MidiPlayer(queue.Queue())

    assert player.open_port('port1') is True
    assert player.port == mock_port
    mock_open.assert_called_once_with('port1')

def test_open_port_returns_false_and_port_is_none_on_failure(mocker):
    """
    Tests that the port is None if opening fails.
    """
    mocker.patch('src.midi.midi_handler.mido.open_output', side_effect=IOError("Failed to open"))
    player = MidiPlayer(queue.Queue())

    assert player.open_port('port1') is False
    assert player.port is None

def test_run_loop_sends_message_from_queue_to_port(mocker):
    """
    Tests that the run loop gets a message from the queue and sends it.
    """
    q = queue.Queue()
    player = MidiPlayer(q)

    # Mock the port and its send method
    mock_port = mocker.MagicMock()
    mock_port.closed = False  # Explicitly set the port as not closed
    player.port = mock_port

    # Put a message in the queue
    test_msg = mido.Message('note_on', note=60)
    q.put(test_msg)

    # Start the player thread
    player.start()

    # Give the thread time to process the message
    time.sleep(0.2)

    # Assert that send was called with the message
    mock_port.send.assert_called_once_with(test_msg)

    # Stop the thread
    player.stop()
    player.join()

def test_stop_method_terminates_run_loop_thread(mocker):
    """
    Tests that the stop method causes the run loop to terminate.
    """
    player = MidiPlayer(queue.Queue())
    player.start()
    assert player.is_alive()

    player.stop()
    player.join(timeout=1) # Wait for the thread to finish

    assert not player.is_alive()
