#!/usr/bin/env python3

import argparse
import glob
import os

if __name__ != "__main__":
    raise ImportError(f"{__name__} should not be used as a module.")


def main():
    parser = argparse.ArgumentParser(description="Cleanup old cache files")
    parser.add_argument("timestamp", type=int, help="Unix timestamp cutoff")
    parser.add_argument("directory", help="Path to cache directory")
    args = parser.parse_args()

    ret = 0

    # TODO: Convert to non-hardcoded path
    if os.path.exists("redundant.txt"):
        with open("redundant.txt") as redundant:
            for item in map(str.strip, redundant):
                if os.path.isfile(item):
                    try:
                        os.remove(item)
                    except OSError:
                        print(f'Failed to handle "{item}"; skipping.')
                        ret += 1

    if not os.path.isdir(args.directory):
        return ret

    for file in glob.glob(os.path.join(args.directory, "**", "*"), recursive=True):
        if not os.path.isfile(file):
            continue

        try:
            if os.path.getatime(file) < args.timestamp:
                os.remove(file)
        except OSError:
            print(f'Failed to handle "{file}"; skipping.')
            ret += 1

    return ret


try:
    raise SystemExit(main())
except KeyboardInterrupt:
    import signal

    signal.signal(signal.SIGINT, signal.SIG_DFL)
    os.kill(os.getpid(), signal.SIGINT)
