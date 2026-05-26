#!/usr/bin/env python3
import json
import sys
from pathlib import Path


def load_codes(path):
    rows = json.loads(Path(path).read_text(encoding="utf-8"))
    return {
        row["code"]
        for row in rows
        if row["type"] == "EV_KEY"
    }


def main():
    layout = json.loads(Path("config/handle-layout.json").read_text(encoding="utf-8"))
    base_codes = load_codes("captures/base-summary.json")
    fn_codes = load_codes("captures/fn-summary.json")
    errors = []

    for key in layout["physicalKeys"]:
        if key["base"] not in base_codes:
            errors.append(f"{key['id']} base {key['base']} not found in base capture")
        if key["fn"] not in fn_codes:
            errors.append(f"{key['id']} fn {key['fn']} not found in fn capture")

    if errors:
        for error in errors:
            print(error, file=sys.stderr)
        return 1

    print(f"validated {len(layout['physicalKeys'])} physical key pairs")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
