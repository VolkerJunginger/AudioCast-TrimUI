#!/usr/bin/env python3
"""Exercise real installer transactions against an isolated fake StockUI card."""
import importlib.util
import json
from pathlib import Path
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('installer', ROOT / 'tools/install.py')
i = importlib.util.module_from_spec(spec)
spec.loader.exec_module(i)


def write(root, name, data):
    path = root / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)


def snapshot(card):
    return {str(p.relative_to(card)): p.read_bytes() for p in card.rglob('*') if p.is_file()}


def refuses(call):
    try:
        call()
    except (ValueError, OSError):
        return
    raise AssertionError('Unsafe operation was accepted')


with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    card = root / 'card'
    (card / 'Emus/GBA').mkdir(parents=True)
    (card / 'RetroArch').mkdir()
    write(card, 'Emus/GBA/launch.sh', b'original game launcher')
    write(card, 'RetroArch/retroarch.cfg', b'original config')
    write(card, 'Roms/GBA/game.gba', b'private ROM never managed')
    header = b'\x7fELF\x02\x01' + b'\0' * 12 + (183).to_bytes(2, 'little') + b'\0' * 44
    files = {'config.json': (b'{"label":"LINK4BRICK","launch":"launch.sh"}', 0o644),
             'control.sh': (b'#!/bin/sh\n', 0o755), 'settings.txt': (b'LINK_AUDIO=on\n', 0o644),
             'cable/config.txt': (b'PROTOCOL=fms-gba\nPPQN=24\n', 0o644),
             'icon.png': (b'icon-on', 0o644), 'cores/mgba-link_libretro.so': (header, 0o644)}
    files.update({'bin/' + n: (header, 0o755) for n in i.BINS})
    manifest = {'product': 'LINK4BRICK', 'version': i.VERSION, 'installable': True,
                'files': {n: i.digest(data) for n, (data, _) in files.items()}}
    files['release.json'] = (json.dumps(manifest).encode(), 0o644)
    package = root / 'release.zip'
    with zipfile.ZipFile(package, 'w') as z:
        for name, (data, _) in files.items():
            z.writestr(i.APP + name, data)
    loaded = i.load_package(package)
    baseline = snapshot(card)
    receipt = i.install(card, loaded, root / 'fresh-backup')
    assert i.install(card, loaded, root / 'unused-backup') is None
    assert (card / 'Emus/GBA/launch.sh').read_bytes() == baseline['Emus/GBA/launch.sh']
    assert not list(card.rglob('*.log'))
    write(card, i.APP + 'icon.png', b'icon-off')
    i.undo(receipt, card)
    assert (card / i.APP / 'icon.png').read_bytes() == b'icon-off'
    (card / i.APP / 'icon.png').unlink()
    assert snapshot(card) == baseline

    # Upgrade an enabled installation with custom settings, launcher backups,
    # private state and unrelated BrickTools items. Undo restores exact originals.
    preserved = {'enabled': b'', 'launchers.list': b'legacy manifest',
                 'settings.txt': b'LINK_AUDIO=off\n', 'cable/config.txt': b'PROTOCOL=stepper-gba\nPPQN=48\n',
                 'icon.png': b'custom-current-icon', 'cable/states/user.state': b'private state'}
    for name, data in preserved.items():
        write(card, i.APP + name, data)
    write(card, i.APP + 'control.sh', b'previous version')
    for name in i.RETIRED:
        write(card, i.APP + name, b'previous diagnostic helper')
    write(card, 'Emus/GBA/launch.sh.audiocast-original', b'launcher backup')
    write(card, 'Apps/BrickTools/menu.json', json.dumps([
        {'execute': './scripts/audiocast_check.sh', 'label': 'AudioCast cable check'},
        {'execute': './scripts/other.sh', 'label': 'Other tool'}]).encode())
    write(card, 'Apps/BrickTools/scripts/audiocast_check.sh', b'old checker')
    baseline = snapshot(card)
    receipt = i.install(card, loaded, root / 'update-backup')
    for name, data in preserved.items():
        assert (card / i.APP / name).read_bytes() == data
    assert not any((card / i.APP / name).exists() for name in i.RETIRED)
    assert json.loads((card / 'Apps/BrickTools/menu.json').read_text()) == [
        {'execute': './scripts/other.sh', 'label': 'Other tool'}]
    i.undo(receipt, card)
    assert snapshot(card) == baseline

    # An interrupted write rolls back every changed file without copying macOS flags.
    original_write = i.atomic_write
    failed = [False]
    def fail_once(path, data, mode=0o644):
        if path == card / i.APP / 'control.sh' and not failed[0]:
            failed[0] = True
            raise OSError('simulated card write failure')
        return original_write(path, data, mode)
    i.atomic_write = fail_once
    refuses(lambda: i.install(card, loaded, root / 'failure-backup'))
    i.atomic_write = original_write
    assert snapshot(card) == baseline

    # Symlinks, corrupt manifests and edited files/backups are refused before mutation.
    (card / i.APP / 'control.sh').unlink()
    (card / i.APP / 'control.sh').symlink_to(root / 'outside')
    refuses(lambda: i.changes_for(card, loaded))
    (card / i.APP / 'control.sh').unlink()
    write(card, i.APP + 'control.sh', b'previous version')
    receipt = i.install(card, loaded, root / 'protected-backup')
    write(card, i.APP + 'control.sh', b'user edit')
    before = snapshot(card)
    refuses(lambda: i.undo(receipt, card))
    assert snapshot(card) == before
    write(card, i.APP + 'control.sh', loaded['control.sh'][0])
    write(receipt.parent, 'originals/' + i.APP + 'control.sh', b'corrupted backup')
    before = snapshot(card)
    refuses(lambda: i.undo(receipt, card))
    assert snapshot(card) == before
    with zipfile.ZipFile(root / 'corrupt.zip', 'w') as z:
        for name, (data, _) in files.items():
            z.writestr(i.APP + name, b'changed' if name == 'control.sh' else data)
    refuses(lambda: i.load_package(root / 'corrupt.zip'))
    with zipfile.ZipFile(root / 'unsafe.zip', 'w') as z:
        z.writestr('../outside', b'unsafe')
    refuses(lambda: i.load_package(root / 'unsafe.zip'))
print('PASS: fresh/update/undo, unchanged settings and ROMs, diagnostic cleanup, rollback, symlinks and corruption')
