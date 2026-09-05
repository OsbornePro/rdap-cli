# Installation

`rdap-cli` is distributed primarily as source code.

The client requires a C11 compiler, `make`, `pkg-config`, libcurl development
files, and json-c development files.

## Clone the repository

```bash
git clone https://github.com/OsbornePro/rdap-cli.git
cd rdap-cli
```

## RHEL 10 and Fedora

```bash
sudo dnf install gcc make pkgconf-pkg-config libcurl-devel json-c-devel
make
```

Verify:

```bash
./rdap -V
```

For v0.3.2:

```text
rdap 0.3.2
```

## Debian and Ubuntu

```bash
sudo apt update
sudo apt install build-essential pkg-config libcurl4-openssl-dev libjson-c-dev
make
```

## Kali Linux

```bash
sudo apt update
sudo apt install build-essential pkg-config libcurl4-openssl-dev libjson-c-dev
make
```

Verify the resulting binary:

```bash
./rdap -V
file ./rdap
ldd ./rdap
```

## Arch Linux

```bash
sudo pacman -S base-devel pkgconf curl json-c
make
```

## macOS

Using Homebrew:

```bash
brew install pkg-config curl json-c
make
```

Depending on the Homebrew configuration, additional compiler or `pkg-config`
settings may be required for Homebrew's libcurl.

## Clean build

```bash
make clean
make
```

## Install system-wide

```bash
make
sudo make install
hash -r
```

The executable is normally installed as:

```text
/usr/local/bin/rdap
```

Verify:

```bash
which rdap
rdap -V
```

## Manual installation

```bash
sudo cp ./rdap /usr/local/bin/rdap
sudo chmod 0755 /usr/local/bin/rdap
hash -r
```

## Uninstall

```bash
sudo make uninstall
```

## Verify build dependencies

```bash
pkg-config --modversion libcurl
pkg-config --modversion json-c
```

## Clock-skew warnings

If `make` reports a clock-skew warning, first verify the clock:

```bash
timedatectl
```

If the clock is correct but repository files have future timestamps:

```bash
find . -exec touch {} +
make clean
make
```

## Documentation environment

Create and activate a Python virtual environment:

```bash
python3 -m venv .venv
source .venv/bin/activate
```

Install the documentation dependencies:

```bash
python -m pip install -r docs/requirements.txt
```

Run the local documentation server:

```bash
mkdocs serve
```

Then open:

```text
http://127.0.0.1:8000/
```
