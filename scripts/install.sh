#!/bin/zsh
set -euo pipefail

QUEUE_NAME="PM220"
FILTER_NAME="pm220-filter"
PPD_FILE="PM220.ppd"

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

FILTER_SRC="$ROOT_DIR/src/$FILTER_NAME.c"
PPD_SRC="$ROOT_DIR/ppd/$PPD_FILE"

FILTER_DEST="/usr/libexec/cups/filter"
PPD_DEST="/Library/Printers/PPDs/Contents/Resources/$PPD_FILE"

echo "PM220 macOS CUPS Driver Installation Script"
echo "-------------------------------------------"
echo ""

if [[ ! -f "$FILTER_SRC" ]]; then
    echo "Error: $FILTER_SRC not found"
    exit 1
fi

if [[ ! -f "$PPD_SRC" ]]; then
    echo "Error: $PPD_SRC not found"
    exit 1
fi

echo "Step 1: Building CUPS filter"
cc -O2 -o "/tmp/$FILTER_NAME" "$FILTER_SRC" $(cups-config --cflags --libs)
echo "Step 1: CUPS filter built successfully"

echo ""

echo "Step 2: Installing CUPS filter to $FILTER_DEST"
sudo cp "/tmp/$FILTER_NAME" "$FILTER_DEST"
sudo chown root:wheel "$FILTER_DEST"
sudo chmod 555 "$FILTER_DEST"
sudo xattr -cr "$FILTER_DEST" 2>/dev/null || true
echo "Step 2: CUPS filter installed successfully"

echo ""

echo "Step 3: Installing PPD to $PPD_DEST"
sudo cp "$PPD_SRC" "$PPD_DEST"
sudo chown root:wheel "$PPD_DEST"
sudo chmod 644 "$PPD_DEST"
sudo xattr -cr "$PPD_DEST" 2>/dev/null || true
if command -v cupstestppd >/dev/null 2/&1; then
    cuptestppd "$PPD_DEST" || {
        echo ""
        echo "PPD validation failed"
        exit 1
    }
else
    echo "- cuptestppd not found, skipping PPD validation"
fi
echo "Step 3: PPD installed successfully"

echo ""

echo "Step 4: Detecting PM220 USB URI"
URI="$(lpinfo -v | grep -Ei 'usb://.*(Polono|PM220|2B-PM220|Nelko)' | head -n 1 | awk '{print $2}')"
if [[ -z "$URI" ]]; then
    echo ""
    echo "Could not detect PM220 USB URI"
    echo ""
    echo "Plug in and power on the printer, then run:"
    echo "  lpinfo -v | grep -i usb"
    echo ""
    echo "Then create the queue manually using:"
    echo "  sudo lpadmin -p $QUEUE_NAME -E -v '[[URI]]' -P '$PPD_DEST'"
    echo "Remember to replace [[URI]] with the actual URI from the previous command"
    exit 1
fi
echo "Step 4: PM220 USB URI detected: $URI"

echo ""

echo "Step 5: Creating CUPS queue"
sudo lpadmin -x "$QUEUE_NAME" 2>/dev/null || true
sudo lpadmin -p "$QUEUE_NAME" -E -v "$URI" -P "$PPD_DEST"
echo "Step 5: CUPS queue created successfully"

echo ""

echo "Installed successfully"
echo ""
echo "Test with:"
echo "  lp -p $QUEUE_NAME -o media=30x50mmRotated.Fullbleed [[/path/to/label.pdf]]"
echo ""
echo "Check queue:"
echo "  lpstat -v $QUEUE_NAME"
