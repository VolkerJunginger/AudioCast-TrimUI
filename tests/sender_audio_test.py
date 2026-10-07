"""Receive actual Link Audio and exercise delayed PCM without >250 ms gaps."""
import argparse,math,os,struct,subprocess,tempfile,time
from pathlib import Path

p=argparse.ArgumentParser();p.add_argument('build',type=Path);args=p.parse_args()
build=args.build.resolve()
def clock():return time.clock_gettime(time.CLOCK_MONOTONIC)

def run(directory,name,recovery,slow):
    received=directory/(name+'-received.txt');sent=directory/(name+'-sent.txt')
    with received.open('w') as rx,sent.open('w') as tx:
        sender=subprocess.Popen([str(build/'linkaudio-send')],stdin=subprocess.PIPE,stdout=tx,stderr=tx,
            env=dict(os.environ,AUDIOCAST_AUDIO_RECOVERY=str(int(recovery)),AUDIOCAST_AUDIO_DIAGNOSTICS='1'))
        peer=subprocess.Popen([str(build/'link-audio-peer')]+(['8'] if slow else []),stdout=rx,stderr=rx)
        try:
            target=clock();deadline=target+55;frames=0;ready=False
            while clock()<deadline and peer.poll() is None:
                if not ready:ready='\nREADY\n' in received.read_text()
                n=804
                samples=[int(12000*math.sin(2*math.pi*1000*(frames+j)/48000)) for j in range(n)]
                data=struct.pack('=%dh'%(n*2),*(s for v in samples for s in (v,-v)))
                # Split some PCM frames across reads, as the relay's 1024-byte
                # writes need not align to either commit size.
                if frames%1608==0:sender.stdin.write(data[:17]);sender.stdin.flush();data=data[17:]
                sender.stdin.write(data);sender.stdin.flush();frames+=n
                target+=.05 if ready and slow else n/48000
                delay=target-clock()
                if delay>0:time.sleep(delay)
            assert peer.poll() is not None,'Receiver test timed out'
            sender.stdin.close();assert sender.wait(timeout=8)==0
            assert peer.wait(timeout=8)==0
        finally:
            for child in [sender,peer]:
                if child.poll() is None:child.terminate();child.wait(timeout=5)
    rows=[];measured=False
    for line in received.read_text().splitlines():
        if line=='READY':measured=True
        if measured and line.startswith('BUFFER '):
            v=line.split();rows.append(dict(count=int(v[1]),frames=int(v[2]),channels=int(v[3]),rate=int(v[4]),
                beat=float(v[5]),tempo=float(v[6]),stamp=int(v[7]),arrival=int(v[8]),peak=int(v[9])))
    assert len(rows)>800,len(rows)
    assert all(r['frames']==125 and r['channels']==2 and r['rate']==48000 for r in rows)
    assert max(r['peak'] for r in rows)==12000
    assert all(b['count']==a['count']+1 for a,b in zip(rows,rows[1:])), 'Packet sequence loss'
    age=max(r['arrival']-r['stamp'] for r in rows)
    diagnostics=sent.read_text();assert 'commit_rejected=0' in diagnostics
    assert 'policy='+('bounded' if recovery else 'legacy') in diagnostics
    if not slow:
        assert {120.,150.}.issubset({r['tempo'] for r in rows})
        errors=[(b['beat']-a['beat'])*60000000/a['tempo']-a['frames']*1000000/48000
                for a,b in zip(rows,rows[1:]) if a['tempo']==b['tempo']]
        assert max(abs(e) for e in errors)<100, (min(errors),max(errors))
        assert age<500000,age
    print('%s: received=%d max_timestamp_age_us=%d'%(name,len(rows),age))
    return age

with tempfile.TemporaryDirectory(prefix='ac-link-audio-') as directory:
    directory=Path(directory)
    run(directory,'steady-with-tempo-change',True,False)
    legacy=run(directory,'short-stalls-legacy',False,True)
    recovered=run(directory,'short-stalls-recovery',True,True)
    # A receiver playing at four-beat latency / 120 BPM cannot play a packet
    # that is already >2 seconds old. This is an age test, not a Push emulator.
    assert legacy>2000000,legacy
    assert recovered<250000,recovered
print('PASS: actual non-silent stereo PCM, fragmented input, tempo change and packet continuity; legacy timestamps expire under short stalls while recovery stays fresh')
