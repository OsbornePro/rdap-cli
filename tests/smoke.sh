#!/bin/sh
set -eu

RDAP="${RDAP:-./rdap}"

"$RDAP" --version
"$RDAP" --help >/dev/null

echo "Testing domain..."
"$RDAP" example.com >/dev/null

echo "Testing IPv4..."
"$RDAP" 1.1.1.1 >/dev/null

echo "Testing IPv6..."
"$RDAP" 2606:4700:4700::1111 >/dev/null

echo "Testing ASN..."
"$RDAP" AS13335 >/dev/null

echo "All smoke tests passed."
