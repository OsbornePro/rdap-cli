#!/bin/sh
set -eu

RDAP="${RDAP:-./rdap}"

"$RDAP" --version
"$RDAP" --help >/dev/null

echo "Testing domain..."
"$RDAP" example.com >/dev/null

echo "Testing IPv4..."
"$RDAP" 8.8.8.8 >/dev/null

echo "Testing IPv6..."
"$RDAP" 2001:4860:4860::8888 >/dev/null

echo "Testing ASN..."
"$RDAP" AS15169 >/dev/null

echo "All smoke tests passed."
