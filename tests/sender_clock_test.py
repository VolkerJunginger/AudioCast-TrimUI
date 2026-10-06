"""Actual two-participant Link test: optional IPC while PCM input is idle."""
import argparse
import os
from pathlib import Path
import socket
import struct
import subprocess
import tempfile
import time
p = argparse.ArgumentParser()
p.add_argument("build", type=Path)
a = p.parse_args()
with tempfile.TemporaryDirectory(prefix="ac-clock-ipc-") as d:
    path = d + "/clock.sock"
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
    sock.bind(path); sock.settimeout(0.5)
    sender = subprocess.Popen([str(a.build / "linkaudio-send")], stdin=subprocess.PIPE,
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
        env=dict(os.environ, AUDIOCAST_CLOCK_SOCKET=path))
    peer = subprocess.Popen([str(a.build / "link-clock-peer")])
    found = set()
    try:
        deadline = time.monotonic() + 22
        while time.monotonic() < deadline:
            try:
                packet = sock.recv(100)
            except socket.timeout:
                continue
            magic, version, timestamp, beat, tempo, peers, playing = struct.unpack("=IIqddII", packet)
            assert magic == 0x41434C4B and version == 1
            assert 0 <= time.clock_gettime(time.CLOCK_MONOTONIC) * 1e6 - timestamp < 500000
            if peers and abs(tempo - 90) < 0.01: found.add(90)
            if peers and abs(tempo - 150) < 0.01: found.add(150)
            if found == {90, 150}: break
        assert found == {90, 150}, found
        # Receiver disappearance must not take down or block the sender.
        sock.close(); os.unlink(path)
        time.sleep(0.3)
        assert sender.poll() is None
    finally:
        sender.terminate(); sender.communicate(timeout=5)
        peer.terminate(); peer.communicate(timeout=5)
        sock.close()
print("PASS: existing sender Link participant exports 90/150 BPM peer tempo while PCM is idle; absent receiver is harmless")
