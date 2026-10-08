# License and third-party notices

LINK4BRICK code and original icon artwork are copyright 2026 Volker Junginger and distributed under **GPL-2.0-or-later**. See [LICENSE](LICENSE). This program is provided without warranty.

## Ableton Link

The sender links against Ableton Link at commit `902aef95bf94af49746fdda5369b42cdcfa1e6d2`, including its pinned submodules. Link is copyright Ableton AG and is used under GPL-2.0-or-later. Its notice is reproduced in [LICENSES/Ableton-Link.md](LICENSES/Ableton-Link.md).

Upstream source: https://github.com/Ableton/link

## Asio

Link includes standalone Asio by Christopher M. Kohlhoff and contributors, distributed under the Boost Software License 1.0. The release carries its upstream license text and the pinned source in the accompanying source bundle.

Upstream source: https://github.com/chriskohlhoff/asio

## Build and runtime components

The ARM64 binaries are built with `ghcr.io/loveretro/tg5040-toolchain:latest` and statically link libstdc++/libgcc using the toolchain's GCC runtime libraries. GCC's runtime components have their own license terms and runtime exceptions; see https://gcc.gnu.org/onlinedocs/libstdc++/manual/license.html and the toolchain's upstream sources at https://github.com/loveretro/tg5040-toolchain.

ALSA is loaded from the device at runtime; no firmware or ALSA library is bundled. Pillow is used to generate menu typography during the build and is not shipped in the app.

The source release includes this project's source and the exact Link/Asio sources fetched by the workflow. The build workflow and developer guide document the toolchain and commands.

Ableton, Link, Push and TrimUI names identify compatible technology/devices. LINK4BRICK is an independent project and is not endorsed by Ableton or TrimUI. Its icons are original artwork, not official vendor logos.

## Private GBA sync core

LINK4BRICK uses mGBA at commit
`3a5e34be33dc7f8f707e5bc9db69e8a430046f21`, under the Mozilla Public License 2.0.
See `LICENSES/mGBA-MPL-2.0.txt`. The compiled inih parser is BSD licensed;
its notice is in `LICENSES/mGBA-inih.txt`. Its integration patch and `sync/mgba-clock.c/.h`
are supplied under MPL-2.0. The clock protocol header and the remaining LINK4BRICK
components use GPL-2.0-or-later. Corresponding release source contains the pinned
mGBA checkout, patch, and all additions. FMS and STEPPER ROMs are not distributed.

## Inter menu typography

Inter is copyright The Inter Project Authors and distributed under SIL Open Font License 1.1. The menu uses generated grayscale glyph masks; the corresponding Inter font, atlas generator and license are supplied with source. The runtime has no font-library dependency. See [LICENSES/Inter-OFL.txt](LICENSES/Inter-OFL.txt) and https://github.com/rsms/inter.
