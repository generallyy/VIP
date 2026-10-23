#!/usr/bin/env python3
"""Bridge Elegoo Uno serial messages to a virtual macOS MIDI port."""

from __future__ import annotations

import argparse
import sys
from dataclasses import dataclass
from typing import Optional


DEFAULT_BAUD_RATE = 115200
DEFAULT_MIDI_PORT_NAME = "Elegoo Chords and Pan"


@dataclass(frozen=True)
class MidiEvent:
    message_type: str
    data1: int
    data2: int


def parse_serial_message(line: str) -> Optional[MidiEvent]:
    """Parse one validated ON, OFF, or CC serial message."""
    parts = line.strip().split()
    if len(parts) != 3 or parts[0] not in {"ON", "OFF", "CC"}:
        return None

    try:
        data1 = int(parts[1])
        data2 = int(parts[2])
    except ValueError:
        return None

    if not (0 <= data1 <= 127 and 0 <= data2 <= 127):
        return None

    message_types = {
        "ON": "note_on",
        "OFF": "note_off",
        "CC": "control_change",
    }
    return MidiEvent(message_types[parts[0]], data1, data2)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Convert Elegoo chord and pan serial output into a virtual MIDI "
            "input for GarageBand."
        )
    )
    parser.add_argument("--port", help="Uno serial port, such as /dev/cu.usbmodem1101")
    parser.add_argument(
        "--baud",
        type=int,
        default=DEFAULT_BAUD_RATE,
        help=f"serial baud rate (default: {DEFAULT_BAUD_RATE})",
    )
    parser.add_argument(
        "--channel",
        type=int,
        choices=range(1, 17),
        default=1,
        metavar="1-16",
        help="MIDI channel (default: 1)",
    )
    parser.add_argument(
        "--name",
        default=DEFAULT_MIDI_PORT_NAME,
        help=f"virtual MIDI port name (default: {DEFAULT_MIDI_PORT_NAME!r})",
    )
    parser.add_argument("--list-ports", action="store_true", help="list serial ports and exit")
    parser.add_argument("--verbose", action="store_true", help="print forwarded messages")
    return parser


def load_serial_module():
    try:
        import serial
        from serial.tools import list_ports
    except ImportError as exc:
        raise RuntimeError(
            "pyserial is not installed; run: python3 -m pip install -r requirements.txt"
        ) from exc
    return serial, list_ports


def load_mido_module():
    try:
        import mido
        import rtmidi  # noqa: F401 - verifies the CoreMIDI backend is installed
    except ImportError as exc:
        raise RuntimeError(
            "MIDI dependencies are not installed; run: "
            "python3 -m pip install -r requirements.txt"
        ) from exc

    mido.set_backend("mido.backends.rtmidi")
    return mido


def list_serial_ports(list_ports) -> int:
    ports = list(list_ports.comports())
    if not ports:
        print("No serial ports found.")
        return 1

    for port in ports:
        print(f"{port.device}\t{port.description}")
    return 0


def to_mido_message(event: MidiEvent, channel: int, mido):
    if event.message_type == "control_change":
        return mido.Message(
            "control_change",
            channel=channel,
            control=event.data1,
            value=event.data2,
        )

    return mido.Message(
        event.message_type,
        channel=channel,
        note=event.data1,
        velocity=event.data2,
    )


def run_bridge(args: argparse.Namespace, serial, mido) -> int:
    if not args.port:
        raise RuntimeError("Specify the Uno port with --port, or use --list-ports.")

    try:
        serial_connection = serial.Serial(args.port, args.baud, timeout=0.25)
    except serial.SerialException as exc:
        raise RuntimeError(f"Could not open serial port {args.port}: {exc}") from exc

    try:
        serial_connection.reset_input_buffer()
        try:
            midi_output = mido.open_output(args.name, virtual=True)
        except (OSError, RuntimeError, SystemError) as exc:
            raise RuntimeError(f"Could not create virtual MIDI port: {exc}") from exc

        try:
            with midi_output:
                print(f"Serial input: {args.port} at {args.baud} baud")
                print(f"GarageBand MIDI input: {args.name} (channel {args.channel})")
                print("Turn the pan knob or press a chord button. Control-C stops.")

                while True:
                    raw_line = serial_connection.readline()
                    if not raw_line:
                        continue

                    line = raw_line.decode("ascii", errors="replace")
                    event = parse_serial_message(line)
                    if event is None:
                        if args.verbose:
                            print(f"Ignored serial input: {line.rstrip()!r}")
                        continue

                    message = to_mido_message(event, args.channel - 1, mido)
                    midi_output.send(message)
                    if args.verbose:
                        print(message, flush=True)
        except serial.SerialException as exc:
            raise RuntimeError(f"Serial connection failed: {exc}") from exc
    finally:
        serial_connection.close()


def main() -> int:
    args = build_parser().parse_args()
    try:
        serial, list_ports = load_serial_module()
        if args.list_ports:
            return list_serial_ports(list_ports)
        mido = load_mido_module()
        return run_bridge(args, serial, mido)
    except KeyboardInterrupt:
        print("\nMIDI bridge stopped.")
        return 0
    except RuntimeError as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
