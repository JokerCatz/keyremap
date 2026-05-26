#!/usr/bin/env python3
import json
from pathlib import Path


def load_summary(path):
    rows = json.loads(Path(path).read_text(encoding="utf-8"))
    return {
        (row["device"].split("/")[-1], row["type"], row["code"]): row["count"]
        for row in rows
        if row["type"] != "EV_MSC"
    }


def print_table(title, rows, counts):
    print(f"## {title}")
    print()
    print("| Device | Type | Code | Count |")
    print("|---|---:|---|---:|")
    for key in sorted(rows):
        device, event_type, code = key
        print(f"| {device} | {event_type} | `{code}` | {counts[key]} |")
    print()


def main():
    base = load_summary("captures/base-summary.json")
    fn = load_summary("captures/fn-summary.json")
    base_keys = set(base)
    fn_keys = set(fn)

    print("# Handle Capture Analysis")
    print()
    print(f"- Base events: {len(base_keys)}")
    print(f"- Fn events: {len(fn_keys)}")
    print(f"- Common events: {len(base_keys & fn_keys)}")
    print()

    print_table("Base", base_keys, base)
    print_table("Fn", fn_keys, fn)
    print_table("Base Only", base_keys - fn_keys, base)
    print_table("Fn Only", fn_keys - base_keys, fn)

    print("## Common")
    print()
    print("| Device | Type | Code | Base Count | Fn Count |")
    print("|---|---:|---|---:|---:|")
    for key in sorted(base_keys & fn_keys):
        device, event_type, code = key
        print(f"| {device} | {event_type} | `{code}` | {base[key]} | {fn[key]} |")


if __name__ == "__main__":
    main()
