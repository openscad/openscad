#!/usr/bin/env python3

import subprocess
import sys


def main():
    worker = subprocess.Popen(
        [sys.argv[1], "--compute-worker"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    try:
        assert worker.stdout.readline().strip() == "ready"
        worker.stdin.write("ping\n")
        worker.stdin.flush()
        assert worker.stdout.readline().strip() == "pong"
        assert worker.poll() is None
        worker.stdin.write("quit\n")
        worker.stdin.flush()
        assert worker.wait(timeout=5) == 0
    finally:
        if worker.poll() is None:
            worker.kill()


if __name__ == "__main__":
    main()
