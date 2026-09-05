# rdap-cli

A small, RDAP-only command-line registration lookup client written in C.

`rdap` is intended to feel like the classic `whois` command while using the
modern Registration Data Access Protocol (RDAP) exclusively.

It automatically detects whether the query is a:

- domain name
- IPv4 address
- IPv6 address
- Autonomous System Number (ASN)

It discovers the authoritative RDAP service using the IANA RDAP bootstrap
registries defined by RFC 9224. It does **not** query WHOIS servers and does
**not** use TCP port 43.

## Examples

```sh
rdap osbornepro.com
rdap 1.1.1.1
rdap 2001:4860:4860::8888
rdap AS15169
rdap 15169
```

Print the complete RDAP response:

```sh
rdap --json osbornepro.com
```

See discovery details:

```sh
rdap --verbose osbornepro.com
```

## Dependencies

- a C11 compiler
- libcurl
- json-c
- pkg-config

### Debian / Ubuntu

```sh
sudo apt install -y build-essential pkg-config libcurl4-openssl-dev libjson-c-dev
```

### Fedora / RHEL 10

```sh
sudo dnf install -y gcc make pkgconf-pkg-config libcurl-devel json-c-devel
```

### Arch Linux

```sh
sudo pacman -S base-devel curl json-c
```

### macOS with Homebrew

```sh
brew install curl json-c pkg-config
```

## Build

Build the project with:

```sh
make
```

### Clock Skew Warning

If `make` reports a warning similar to:

```text
make: Warning: File 'Makefile' has modification time 21208 s in the future
make: warning: Clock skew detected. Your build may be incomplete.
```

This means one or more project files have modification timestamps that are ahead of your system's current time. 
This can happen after downloading, extracting, copying, or transferring the repository between systems.
  
To reset the modification timestamps and rebuild the project, run:

```sh
find . -exec touch {} +
make clean
make
```

If the warning continues, verify that your system clock is correct and synchronized:

```sh
timedatectl
```

On systems using `systemd`, automatic network time synchronization can be enabled with:

```sh
sudo timedatectl set-ntp true
```

After a successful build, the `rdap` executable will be created in the project directory and can be executed by doing:
```sh
./rdap osbornepro.com
```

Move it to /usr/local/bin/rdap if you wish to execute it easily in the future:
```sh
cp rdap /usr/local/bin/rdap
```

Now you can use it like this:
```sh
rdap osbornepro.com
```

## Install

```sh
sudo make install
```

The default installation path is `/usr/local/bin/rdap`.

A custom prefix can be used:

```sh
sudo make PREFIX=/usr install
```

Uninstall:

```sh
sudo make uninstall
```

## Tests

The smoke tests make real network requests:

```sh
make test
```

## How discovery works

The program downloads the appropriate IANA bootstrap registry:

- `dns.json` for domains
- `ipv4.json` for IPv4
- `ipv6.json` for IPv6
- `asn.json` for AS numbers

It then follows the RFC 9224 matching rules:

- domain: longest label-wise suffix match
- IP address: longest CIDR prefix match
- ASN: containing number range

After finding the RDAP base URL, the program sends the appropriate HTTPS
request:

```text
<base>/domain/<name>
<base>/ip/<address>
<base>/autnum/<number>
```

Only HTTPS service URLs are accepted.

## Why not WHOIS?

WHOIS is a plain-text protocol traditionally served over TCP port 43. RDAP
provides structured JSON over HTTP(S), standardized service discovery, better
internationalization support, and a more suitable protocol for modern clients.

This program intentionally has no WHOIS fallback.

## Current limitations

This is intentionally a small client. In particular:

- bootstrap files are retrieved on every invocation rather than cached
- internationalized domain names are not converted to A-label/Punycode
- entity details such as registrar/contact vCards are not yet rendered in the
  human-readable summary
- RDAP search endpoints are not implemented
- HTTP caching headers are not yet honored

Contributions addressing these are welcome.

## Project layout

```text
.
├── LICENSE
├── Makefile
├── README.md
├── SECURITY.md
├── CONTRIBUTING.md
├── .gitignore
├── src/
│   └── rdap.c
└── tests/
    └── smoke.sh
```

## Standards

The main standards relevant to this project are:

- RFC 9082 — RDAP query format
- RFC 9083 — RDAP JSON responses
- RFC 9224 — finding authoritative RDAP services

IANA publishes the live bootstrap registries at:

`https://data.iana.org/rdap/`

## License

See [LICENSE](https://github.com/OsbornePro/rdap-cli/blob/main/LICENSE).
