#!/usr/bin/env python3
import argparse
import json
import os
import re
import select
import struct
import sys
import time
from pathlib import Path

EVENT_STRUCT = struct.Struct("llHHI")

CODE_HEADER = Path("/usr/include/linux/input-event-codes.h")


def parse_code_names(prefix):
    names = {}
    if not CODE_HEADER.exists():
        return names

    pattern = re.compile(rf"^#define\s+({prefix}_[A-Z0-9_]+)\s+(\d+|0x[0-9a-fA-F]+)\b")
    with CODE_HEADER.open("r", encoding="utf-8", errors="replace") as header:
        for line in header:
            match = pattern.match(line)
            if match:
                names[int(match.group(2), 0)] = match.group(1)
    return names


EV_NAMES = parse_code_names("EV")
KEY_NAMES = parse_code_names("KEY") | parse_code_names("BTN")
REL_NAMES = parse_code_names("REL")
ABS_NAMES = parse_code_names("ABS")
MSC_NAMES = parse_code_names("MSC")


def event_name(event_type, code):
    if event_type == 0x01:
        return KEY_NAMES.get(code, f"KEY_{code}")
    if event_type == 0x02:
        return REL_NAMES.get(code, f"REL_{code}")
    if event_type == 0x03:
        return ABS_NAMES.get(code, f"ABS_{code}")
    if event_type == 0x04:
        return MSC_NAMES.get(code, f"MSC_{code}")
    return str(code)


def discover_events(device_name):
    devices = []
    current = {}

    with open("/proc/bus/input/devices", "r", encoding="utf-8", errors="replace") as proc:
        for raw_line in proc:
            line = raw_line.rstrip("\n")
            if not line:
                if current:
                    devices.append(current)
                    current = {}
                continue

            if line.startswith("N: Name="):
                current["name"] = line.split("=", 1)[1].strip().strip('"')
            elif line.startswith("H: Handlers="):
                current["handlers"] = line.split("=", 1)[1].split()

    if current:
        devices.append(current)

    paths = []
    for device in devices:
        if device_name not in device.get("name", ""):
            continue
        for handler in device.get("handlers", []):
            if handler.startswith("event"):
                paths.append(f"/dev/input/{handler}")

    return sorted(set(paths))


def summarize(path):
    counts = {}
    with open(path, "r", encoding="utf-8") as capture:
        for line in capture:
            try:
                record = json.loads(line)
            except json.JSONDecodeError:
                continue
            if record.get("type_name") == "EV_SYN":
                continue
            key = (record.get("device"), record.get("type_name"), record.get("code_name"))
            counts[key] = counts.get(key, 0) + 1

    return [
        {"device": device, "type": event_type, "code": code, "count": count}
        for (device, event_type, code), count in sorted(counts.items())
    ]


def open_devices(paths):
    devices = {}
    for path in paths:
      fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
      devices[fd] = path
    return devices


def main():
    parser = argparse.ArgumentParser(description="Record Linux input events from the JD-DZ.COM USB HANDLE.")
    parser.add_argument("events", nargs="*", help="Optional /dev/input/eventX paths. Auto-discovered when omitted.")
    parser.add_argument("--device-name", default="JD-DZ.COM USB HANDLE")
    parser.add_argument("--list", action="store_true", help="List discovered event devices and exit.")
    parser.add_argument("--seconds", type=float, default=20.0)
    parser.add_argument("--output", default="captures/input-events.jsonl")
    parser.add_argument("--summary", default="captures/input-summary.json")
    parser.add_argument("--summarize-only", action="store_true", help="Generate summary from --output without recording.")
    args = parser.parse_args()

    if args.summarize_only:
        summary = summarize(args.output)
        with open(args.summary, "w", encoding="utf-8") as summary_file:
            json.dump(summary, summary_file, indent=2)
            summary_file.write("\n")
        print(f"wrote {args.summary}")
        return

    events = args.events or discover_events(args.device_name)
    if args.list:
        for path in events:
            print(path)
        return

    if not events:
        print(f"no input event devices found for {args.device_name!r}", file=sys.stderr)
        raise SystemExit(1)

    Path(args.output).parent.mkdir(parents=True, exist_ok=True)
    devices = open_devices(events)
    deadline = time.monotonic() + args.seconds

    with open(args.output, "w", encoding="utf-8") as out:
        while time.monotonic() < deadline:
            timeout = max(0.0, deadline - time.monotonic())
            readable, _, _ = select.select(list(devices), [], [], min(timeout, 0.25))
            for fd in readable:
                while True:
                    try:
                        data = os.read(fd, EVENT_STRUCT.size)
                    except BlockingIOError:
                        break

                    if len(data) != EVENT_STRUCT.size:
                        break

                    sec, usec, event_type, code, value = EVENT_STRUCT.unpack(data)
                    record = {
                        "time": sec + usec / 1_000_000,
                        "device": devices[fd],
                        "type": event_type,
                        "type_name": EV_NAMES.get(event_type, f"EV_{event_type}"),
                        "code": code,
                        "code_name": event_name(event_type, code),
                        "value": value,
                    }
                    line = json.dumps(record, separators=(",", ":"))
                    print(line)
                    out.write(line + "\n")
                    out.flush()

    for fd in devices:
        os.close(fd)

    summary = summarize(args.output)
    with open(args.summary, "w", encoding="utf-8") as summary_file:
        json.dump(summary, summary_file, indent=2)
        summary_file.write("\n")
    print(f"wrote {args.output}")
    print(f"wrote {args.summary}")


if __name__ == "__main__":
    try:
        main()
    except PermissionError as error:
        print(f"permission error: {error}", file=sys.stderr)
        print("try: sudo tools/record_linux_input.py", file=sys.stderr)
        raise SystemExit(1)
