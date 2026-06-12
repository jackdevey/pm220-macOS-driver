# PM220 macOS Driver

This is a small, unofficial macOS CUPS driver for the PM220 label printer.

The driver installs a CUPS queue named `PM220`, uses a custom raster filter, and sends TSPL commands directly to the printer.

## Current Capabilities

- Label size is configurable through the PPD media size.
- The PPD includes common label sizes from `30 x 20 mm` through `50 x 150 mm`.
- Custom media widths are limited to the PM220-supported `23-54 mm` range.
- Gap is configurable from `0 mm` through `10 mm`.
- Print density, speed, and direction are configurable.
- Resolution is fixed at `203 DPI`.
- Output is monochrome, 1-bit raster data.
- The CUPS queue is created as `PM220`.

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

The input should be prepared for the selected label size. The original `50 x 30 mm` media remains available as `30x50mmRotated.Fullbleed` for compatibility.

Other built-in media names include:

- `20x30mmRotated.Fullbleed` - `30 x 20 mm`
- `25x25mm.Fullbleed` - `25 x 25 mm`
- `30x40mmRotated.Fullbleed` - `40 x 30 mm`
- `40x50mmRotated.Fullbleed` - `50 x 40 mm`
- `50x50mm.Fullbleed` - `50 x 50 mm`
- `50x70mm.Fullbleed` - `50 x 70 mm`
- `50x100mm.Fullbleed` - `50 x 100 mm`
- `50x150mm.Fullbleed` - `50 x 150 mm`

You can also set printer options from the command line:

```sh
lp -p PM220 \
  -o media=40x50mmRotated.Fullbleed \
  -o PM220Gap=3mm \
  -o PM220Density=10 \
  -o PM220Speed=3 \
  -o PM220Direction=0 \
  /path/to/label.pdf
```

The same options are exposed in the macOS print dialog by the PPD.

For custom media, use CUPS custom media syntax. The width must be between `23 mm` and `54 mm`; the current PPD allows lengths from `20 mm` to `150 mm`.

```sh
lp -p PM220 -o media=Custom.40x60mm /path/to/label.pdf
```

For a full-width label:

```sh
lp -p PM220 -o media=Custom.54x80mm /path/to/label.pdf
```

## Check The Queue

```sh
lpstat -v PM220
```

## Uninstall

```sh
./scripts/uninstall.sh
```

This removes the `PM220` queue, installed filter, installed PPD, and temporary build artifact.
