# Contributing

Keep changes scoped to StockUI GB/GBA unless a new device or emulator is explicitly supported and tested. Preserve speaker output, reversible launchers, SD-card-only persistence and the absence of runtime log files.

See [development instructions](docs/DEVELOPMENT.md) for the host integration tests and ARM64 packaging workflow. Changes to launcher management should include tests for original-file preservation, interrupted restoration and invalid backups. Hardware playback results should name the device, firmware and receiver; passing CI alone is not a hardware test.

Open an issue before proposing a different capture strategy or firmware integration. Do not commit ROMs, saves, credentials, personal logs or card snapshots.
