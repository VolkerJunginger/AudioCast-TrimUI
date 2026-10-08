# Troubleshooting LINK4BRICK

## No Brick Out channel

Enable Link on Push and connect both devices to the same network. Launch the game after enabling LINK4BRICK and Link audio. The Brick Out channel exists during a game session, so select it again after changing games. A router that blocks peer discovery can prevent the connection.

## Audio works but sync does not

Match the LINK4BRICK sync mode and PPQ to the program's input. For the hardware-confirmed path, select **FMS GBA / 24 PPQ** and **SYNC IN / GBA** in FMS, then press START. The next four-beat Link “one” starts playback. FMS Clock and STEPPER use different cable signals; selecting one for the other will not work.

Use the normal **Games → GBA** list. Sync requires the mGBA launcher; an alternate emulator core may still stream audio but cannot provide this virtual cable. Game Boy and general multiplayer link connections are not supported.

## Audio is delayed

The audio buffer is fixed at 65 ms. Network transport and receiver buffering add delay. Compensate incoming audio on Push; this release does not alter the clock with delay compensation. Match PPQ and avoid fast-forward, rewind or run-ahead during sync.

## Icon looks wrong

The ON/OFF icon shows enabled launcher routing, rather than peer connectivity. StockUI may cache images. Leave and reopen Apps, or reboot after an update. The release uses the user's final ON/OFF artwork.

## A game falls back to its original core

Close the game and reboot to clear stale temporary sessions. Confirm the complete package was installed using the Terminal installer and that LINK4BRICK is enabled. A failed audio preflight or private-core readiness check deliberately falls back to the normal launcher.

## Updating or removing the app

Use the Terminal installer for updates; it preserves settings and launcher backups. If replacing the old AudioCast app, turn that app OFF before installing LINK4BRICK. Turn LINK4BRICK OFF before removing it or editing GBA launchers.

An OFF operation preserves edited launchers and damaged/missing backups for review. Never delete `.audiocast-original` files prematurely; these compatibility names still contain your original launchers. Retain the local Terminal installer journals for undo. Existing test logs are left untouched; the release creates no new runtime logs.
