#!/usr/bin/env python3
"""Bridge Elegoo Uno serial messages to a virtual macOS MIDI port."""

from __future__ import annotations

import argparse
import sys
from typing import Optional, Tuple


DEFAULT_BAUD_RATE = 115200
DEFAULT_MIDI_PORT_NAME = "Elegoo GarageBand Controls"


def parse_control_change(line: str) -> Optional[Tuple[int, int]]:
    """Return a validated (controller, value) pair or None for other input."""
    parts = line.strip().split()
    if len(parts) != 3 or parts[0] != "CC":
        return None

    try:
        controller = int(parts[1])
        value = int(parts[2])
    except ValueError:
        return None

    if not (0 <= controller <= 127 and 0 <= value <= 127):
        return None

    return controller, value


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Convert the Elegoo Uno's serial output into a virtual MIDI "
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
    parser.add_argument(
        "--list-ports",
        action="store_true",
        help="list serial ports and exit",
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="print every forwarded MIDI message",
    )
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
        import rtmidi  # noqa: F401 - verifies that the CoreMIDI backend is installed
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


def run_bridge(args: argparse.Namespace, serial, mido) -> int:
    if not args.port:
        raise RuntimeError("Specify the Uno port with --port, or use --list-ports.")

    try:
        serial_connection = serial.Serial(
            port=args.port,
            baudrate=args.baud,
            timeout=0.25,
        )
    except serial.SerialException as exc:
        raise RuntimeError(f"Could not open serial port {args.port}: {exc}") from exc

    try:
        # Opening the serial port resets most Uno boards. Discard anything
        # already buffered, then allow the sketch's fresh startup values through.
        serial_connection.reset_input_buffer()

        try:
            midi_output = mido.open_output(args.name, virtual=True)
        except (OSError, RuntimeError, SystemError) as exc:
            raise RuntimeError(f"Could not create virtual MIDI port: {exc}") from exc

        try:
            with midi_output:
                print(f"Serial input: {args.port} at {args.baud} baud")
                print(f"MIDI input for GarageBand: {args.name} (channel {args.channel})")
                print("Move either knob to send automation. Press Control-C to stop.")

                while True:
                    raw_line = serial_connection.readline()
                    if not raw_line:
                        continue

                    line = raw_line.decode("ascii", errors="replace")
                    parsed = parse_control_change(line)
                    if parsed is None:
                        if args.verbose:
                            print(f"Ignored serial input: {line.rstrip()!r}")
                        continue

                    controller, value = parsed
                    message = mido.Message(
                        "control_change",
                        channel=args.channel - 1,
                        control=controller,
                        value=value,
                    )
                    midi_output.send(message)

                    if args.verbose:
                        print(
                            f"CC{controller} value={value} channel={args.channel}",
                            flush=True,
                        )
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
