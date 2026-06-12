# PM220 macOS Driver

This is a small, unofficial macOS CUPS driver for the PM220 label printer.

The driver installs a CUPS queue named `PM220`, uses a custom raster filter, and sends TSPL commands directly to the printer.

## Current Limitations

This driver is intentionally narrow right now:

- Label size is fixed at `50 x 30 mm`.
- Gap is fixed at `5 mm`.
- Resolution is fixed at `203 DPI`.
- Output is monochrome, 1-bit raster data.
- The CUPS queue is created as `PM220`.
- Media/layout options are not currently configurable from the print dialog.

> I may update this in the future to support configurable label sizes, gap sizes, and other printer settings. For now, the constants live in `src/pm220-filter.c` and the matching media definition lives in `ppd/PM220.ppd`.

## Files

- `src/pm220-filter.c` - CUPS raster filter that converts CUPS raster pages into TSPL bitmap print commands.
- `ppd/PM220.ppd` - PPD file that defines the PM220 queue, 203 DPI output, and the fixed 50 x 30 mm media size.
- `scripts/install.sh` - builds and installs the filter, installs the PPD, detects the USB printer URI, and creates the CUPS queue.
- `scripts/uninstall.sh` - removes the queue, installed filter, installed PPD, and temporary build output.

## Requirements

- macOS with CUPS available.
- Command Line Tools or a working `cc` compiler.
- The printer connected over USB and powered on.
- Administrator access for installing the CUPS filter and PPD.

## Install

From the project directory:

```sh
./scripts/install.sh
```

The install script will:

1. Build `src/pm220-filter.c`.
2. Install the filter to `/usr/libexec/cups/filter/pm220-filter`.
3. Install the PPD to `/Library/Printers/PPDs/Contents/Resources/PM220.ppd`.
4. Try to detect the PM220 USB URI.
5. Create a CUPS printer queue named `PM220`.

If the USB URI cannot be detected automatically, run:

```sh
lpinfo -v | grep -i usb
```

Then create the queue manually using the URI shown for the printer:

```sh
sudo lpadmin -p PM220 -E -v 'usb://...' -P '/Library/Printers/PPDs/Contents/Resources/PM220.ppd'
```

## Print

Print a label with:

```sh
lp -p PM220 -o media=30x50mmRotated.Fullbleed /path/to/label.pdf
```

The input should be prepared for a 50 x 30 mm label. The PPD exposes this as `30x50mmRotated.Fullbleed` because of how the page dimensions are represented for CUPS.

> Or of course, a label can be printed with the macOS print dialog throughout the system.

## Check The Queue

```sh
lpstat -v PM220
```

## Uninstall

```sh
./scripts/uninstall.sh
```

This removes the `PM220` queue, installed filter, installed PPD, and temporary build artifact.
