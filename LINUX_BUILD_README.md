# Linux Build Reference

The normal path is one command from the project folder:

```bash
./Scripts/build_and_play_linux.sh
```

The script automatically searches the common locations under your home directory
for Unreal Engine 5.8. If detection fails, set the directory that contains the
Engine folder and rerun it:

```bash
export UE_ROOT="/absolute/path/to/Linux_Unreal_Engine_5.8.1"
./Scripts/build_and_play_linux.sh
```

Only after a successful local playtest, create the Shipping archive with:

```bash
./Scripts/build_linux_shipping.sh
```

Output: `Artifacts/GorillaProtocol-Linux.tar.gz`.
