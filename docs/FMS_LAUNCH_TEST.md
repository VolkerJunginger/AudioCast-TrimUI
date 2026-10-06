# FMS Launch Test — StockUI compatibility check

**Historical diagnostic, also unsuccessful on the device.** No further
ROM-launching test app is planned. See [virtual link cable integration](VIRTUAL_LINK_CABLE.md).

This is a separate, temporary test app for the TrimUI Brick Hammer. Normal FMS
playback and AudioCast have already been confirmed to work. This app tests the
preview's private mGBA core through the existing, working AudioCast audio route.
It does **not** supply a Link clock or claim that FMS sync works.

1. Keep regular **AudioCast ON**. Quit any running game first.
2. Extract the ZIP on your Mac. Copy **only `AudioCastFMSCheck`** from its `Apps`
   folder into the card's existing `Apps` folder. Do not replace the whole `Apps`
   folder or reinstall AudioCast.
3. The default ROM path is `Roms/GBA/fms.gba`. If needed, edit this test app's
   `rom-path.txt`. FMS is not included.
4. Safely eject the card and reboot the Brick. Open **FMS Launch Test** from Apps.
5. Check whether FMS opens and whether playing notes produces sound on the Brick
   and Push. Use ordinary FMS playback, without switching it to CLOCK input.

The test uses separate saves under `Apps/AudioCastFMSCheck/saves`; your normal FMS
save is left alone. The initial pattern bank will therefore be fresh.

The installed AudioCast sender, ALSA probe and session supervisor are reused
unchanged. The installed GBA launcher's existing directory/environment and CPU
setup are reused unchanged. The private proxy selects only the test core and its
separate saves. No installed launcher, configuration, core or firmware is edited.
No diagnostic log file is created. If normal AudioCast is OFF, its capture route
is unavailable, or the stock GBA invocation is unsupported, this app refuses to
run the compatibility check.

Close the test and remove `Apps/AudioCastFMSCheck` to uninstall. Keep that app's
`saves` folder if you created patterns you want to retain. This test deliberately
uses the regular AudioCast session lifetime; its existing cleanup is unchanged.
