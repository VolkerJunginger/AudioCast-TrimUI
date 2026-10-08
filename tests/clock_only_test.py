"""Actual Link peer checks tempo changes, PCM drainage and absent audio channel."""
import os,socket,struct,subprocess,tempfile,threading,time
from pathlib import Path
import sys
build=Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory(prefix='ac-clock-only-') as d:
    path=d+'/clock.sock';sock=socket.socket(socket.AF_UNIX,socket.SOCK_DGRAM);sock.bind(path);sock.settimeout(.5)
    with open(d+'/sender.txt','w+') as log:
        sender=subprocess.Popen([str(build/'linkaudio-send')],stdin=subprocess.PIPE,stdout=log,stderr=log,
            env=dict(os.environ,AUDIOCAST_CLOCK_SOCKET=path,AUDIOCAST_LINK_AUDIO='0',AUDIOCAST_AUDIO_DIAGNOSTICS='1',AUDIOCAST_CLOCK_MODE='',AUDIOCAST_CLOCK_REFRESH_MS=''))
        peer=subprocess.Popen([str(build/'link-audio-peer'),'--clock-only'],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
        stop=threading.Event();written=[0]
        def feed():
            while not stop.is_set():
                try:sender.stdin.write(b'\x01\x01\x02\x02'*480);sender.stdin.flush();written[0]+=480
                except (BrokenPipeError,ValueError):return
                time.sleep(.01)
        writer=threading.Thread(target=feed);writer.start();found=set();heartbeats=[]
        try:
            deadline=time.monotonic()+18
            while time.monotonic()<deadline and peer.poll() is None:
                try:packet=sock.recv(100)
                except socket.timeout:continue
                magic,version,stamp,beat,tempo,peers,playing=struct.unpack('=IIqddII',packet)
                assert magic==0x41434c4b and version==1
                if peers:found.add(round(tempo))
                heartbeats.append(stamp)
            out=peer.communicate(timeout=4)[0];assert peer.returncode==0,out
            assert {90,150}<=found,found
            stop.set();writer.join(timeout=3);assert not writer.is_alive();sender.stdin.close();assert sender.wait(timeout=5)==0
            log.seek(0);diagnostics=log.read();assert 'LINK_AUDIO=off' in diagnostics and 'committed=0' in diagnostics and 'clock_only: enabled=1 drained_buffers=' in diagnostics
            assert written[0]>48000*8 and len(heartbeats)>50,(written,len(heartbeats))
            print(out.strip());print('PASS: continuous PCM drained, live 90/150 BPM, no Link Audio channel, clean EOF')
        finally:
            stop.set()
            for child in [sender,peer]:
                if child.poll() is None:child.terminate();child.wait(timeout=4)
            writer.join(timeout=3);sock.close()
