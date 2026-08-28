# Flash Drive Handoff

The flash-drive folder has one launcher named `START_GORILLA_ON_LINUX.sh`.
Open a Terminal in that folder and run:

```bash
bash START_GORILLA_ON_LINUX.sh
```

It installs a commit-specific copy under `$HOME/Projects` and runs the complete
build/import/play sequence. It never builds directly on the flash drive and never
overwrites an older project folder.

Unreal Engine itself is not copied to the flash drive. The launcher uses the
Unreal Engine 5.8.1 installation already present on the Linux PC.
