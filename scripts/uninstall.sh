#!/bin/zsh
set -euo pipefail

QUEUE_NAME="PM220"
FILTER_NAME="pm220-filter"
PPD_FILE="PM220.ppd"

FILTER_FILE="/usr/libexec/cups/filter/$FILTER_NAME"
PPD_FILE_INSTALLED="/Library/Printers/PPDs/Contents/Resources/$PPD_FILE"

echo "PM220 macOS CUPS Driver Uninstall Script"
echo "-----------------------------------------"
echo ""

echo "Step 1: Cancelling queued PM220 jobs"
cancel -a "$QUEUE_NAME" 2>/dev/null || true
echo "Step 1: Done"

echo ""

echo "Step 2: Removing CUPS queue: $QUEUE_NAME"
sudo lpadmin -x "$QUEUE_NAME" 2>/dev/null || true
echo "Step 2: Queue removed, or it did not exist"

echo ""

echo "Step 3: Removing CUPS filter"
if [[ -f "$FILTER_FILE" ]]; then
    sudo rm -f "$FILTER_FILE"
    echo "Step 3: Removed $FILTER_FILE"
else
    echo "Step 3: Filter not found at $FILTER_FILE"
fi

echo ""

echo "Step 4: Removing PPD"
if [[ -f "$PPD_FILE_INSTALLED" ]]; then
    sudo rm -f "$PPD_FILE_INSTALLED"
    echo "Step 4: Removed $PPD_FILE_INSTALLED"
else
    echo "Step 4: PPD not found at $PPD_FILE_INSTALLED"
fi

echo ""

echo "Step 5: Clearing temporary build artifact"
rm -f "/tmp/$FILTER_NAME" 2>/dev/null || true
echo "Step 5: Done"

echo ""

echo "Uninstalled successfully"
echo ""
echo "Verify with:"
echo "  lpstat -v $QUEUE_NAME"
echo ""
echo "As the queue was removed, lpstat should report that it is unknown or not found."
