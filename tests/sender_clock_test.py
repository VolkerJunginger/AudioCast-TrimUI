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
p.add_argument("--tempo-hold", action="store_true")
p.add_argument("--tempo-latch", action="store_true")
a = p.parse_args()
with tempfile.TemporaryDirectory(prefix="ac-clock-ipc-") as d:
    path = d + "/clock.sock"
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
    sock.bind(path); sock.settimeout(0.5)
    sender = subprocess.Popen([str(a.build / "linkaudio-send")], stdin=subprocess.PIPE,
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
        env=dict(os.environ, AUDIOCAST_CLOCK_SOCKET=path,
                 AUDIOCAST_CLOCK_MODE="tempo-latch" if a.tempo_latch else "",
                 AUDIOCAST_CLOCK_REFRESH_MS="2000" if a.tempo_hold else ""))
    peer = subprocess.Popen([str(a.build / "link-clock-peer")]+(["4000"] if a.tempo_hold or a.tempo_latch else []))
    found = set(); times = []; previous = None
    try:
        deadline = time.monotonic() + 22
        while time.monotonic() < deadline:
            try:
                packet = sock.recv(100)
            except socket.timeout:
                continue
            magic, version, timestamp, beat, tempo, peers, playing = struct.unpack("=IIqddII", packet)
            times.append(timestamp)
            if a.tempo_latch and peers and previous:
                old_stamp,old_beat,old_tempo=previous
                dt=(timestamp-old_stamp)/1e6
                # A confirmed change can alter slope within this heartbeat,
                # but must never snap phase to the current Link beat.
                assert abs(beat-old_beat-dt*old_tempo/60)<dt*abs(tempo-old_tempo)/60+1e-6
            previous=(timestamp,beat,tempo) if peers else None
            assert magic == 0x41434C4B and version == 1
            assert 0 <= time.clock_gettime(time.CLOCK_MONOTONIC) * 1e6 - timestamp < 500000
            if peers and abs(tempo - 90) < 0.01: found.add(90)
            if peers and abs(tempo - 150) < 0.01: found.add(150)
            if found == {90, 150}: break
        assert found == {90, 150}, found
        if a.tempo_hold or a.tempo_latch:
            intervals = [b-a for a,b in zip(times,times[1:])]
            # Peer join can trigger one early heartbeat; otherwise the bridge
            # must publish around ten times per second, not hundreds.
            assert len(intervals)>20, len(intervals)
            assert sum(i>=90000 for i in intervals)>len(intervals)*0.8, intervals
            assert len(times)*100000 < (times[-1]-times[0])*1.5, len(times)
        # Receiver disappearance must not take down or block the sender.
        sock.close(); os.unlink(path)
        time.sleep(0.3)
        assert sender.poll() is None
    finally:
        sender.terminate(); sender.communicate(timeout=5)
        peer.terminate(); peer.communicate(timeout=5)
        sock.close()
print("PASS: sender exports 90/150 BPM peer tempo while PCM is idle; absent receiver is harmless; tempo-hold="+str(a.tempo_hold)+" tempo-latch="+str(a.tempo_latch))
