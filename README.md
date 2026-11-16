# Audient iD4 MkII — set volume once at login

This is a small command-line helper plus a macOS LaunchAgent. It attempts to set the iD4 MkII's USB playback Feature Unit volume once after login, then exits.

## Requirements

- macOS
- Audient iD4 MkII (USB VID:PID `2708:0009`)
- Homebrew
- Network access the first time, to install `libusb`

## Install

Unzip this folder, open Terminal, `cd` into the unzipped folder, then run:

```sh
chmod +x install.sh uninstall.sh
./install.sh -20
```

Replace `-20` with the desired integer dB value. Valid range: `-127` to `0`. The default if omitted is `-20 dB`.

The installer builds the helper, creates a per-user LaunchAgent, and runs it immediately once so you can test it. At subsequent logins it runs once again. It waits up to 30 seconds for the interface to become available.

## Logs

```sh
cat "$HOME/Library/Application Support/AudientStartupVolume/startup.log"
```

## Uninstall

```sh
./uninstall.sh
```

This removes the LaunchAgent and installed binary/logs. It does not uninstall Homebrew's `libusb`.

## Important technical notes

- This writes USB Audio Class 2 `SET_CUR` volume requests to Feature Unit `0x0A`, using interface index `0x0A04`, volume selector `0x02`, channels 0, 1 and 2.
- The value is signed 16-bit little-endian in 1/256 dB units. For example, `-20 dB` is `-5120`.
- This is digital attenuation before the interface's physical monitor-volume knob. It cannot raise the output above the knob's setting.
- The USB request format and device IDs are documented by the reverse-engineered iD4 MkII project linked below. This minimal libusb implementation has not been validated on your particular Mac/interface/firmware combination. If macOS or the device refuses the device-level control transfer, the robust fallback is to adapt the original project's `IOUSBInterfaceInterface` implementation, which sends the same request through interface 4.

Protocol/reference project:
https://next.tangled.org/eti.tf/id4/tree/main

Apple USB interface documentation:
https://developer.apple.com/library/archive/documentation/DeviceDrivers/Conceptual/USBBook/USBDeviceInterfaces/USBDevInterfaces.html
