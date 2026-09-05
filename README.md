# rdap-cli

A lightweight, RDAP-only command-line registration lookup client written in C.

`rdap` is designed to feel familiar to users of the classic `whois` command while using the modern Registration Data Access Protocol (RDAP) exclusively.

It automatically detects whether a query is a:

-   domain name
-   IPv4 address
-   IPv6 address
-   Autonomous System Number (ASN)

The client discovers the authoritative RDAP service using the IANA RDAP
bootstrap registries defined by RFC 9224. It does **not** query WHOIS
servers and does **not** use TCP port 43 for lookups.

## Features

-   RDAP-only operation with no WHOIS fallback
-   automatic domain, IPv4, IPv6, and ASN detection
-   authoritative service discovery using the IANA RDAP bootstrap
    registries
-   HTTPS-only RDAP requests
-   concise human-readable output
-   complete JSON output with `--json`
-   RDAP notices with `--notices`
-   RDAP links with `--links`
-   registrar RDAP referral following with `--follow`
-   combined registry and registrar JSON with `--json --follow`
-   extended human-readable output with `--full`
-   authoritative RDAP server discovery with `--server`
-   verbose bootstrap and request diagnostics with `--verbose`
-   registrar and IANA registrar ID display
-   abuse contact information
-   registration events
-   nameserver information
-   DNSSEC and DS record information
-   detailed RDAP entity, contact, remark, redaction, conformance, and
    nameserver information in full mode

## Usage

``` text
Usage: rdap [options] QUERY
```

Common options:

``` text
-j, --json       Print complete RDAP JSON; with --follow, wrap both responses
-n, --notices    Include RDAP notices in human-readable output
-l, --links      Include RDAP links in human-readable output
-f, --follow     Follow a related registrar RDAP link when available
-F, --full       Extended human-readable output; implies --notices and --links
-s, --server     Print the discovered authoritative RDAP base URL
-v, --verbose    Show bootstrap discovery details on stderr
-V, --version    Print version and exit
-h, --help       Show help
```

## Examples

### Domain lookup

``` sh
rdap osbornepro.com
```

### IPv4 lookup

``` sh
rdap 1.1.1.1
```

### IPv6 lookup

``` sh
rdap 2606:4700:4700::1111
```

### ASN lookup

ASNs may be supplied with or without the `AS` prefix:

``` sh
rdap AS13335
rdap 13335
```

For example, AS13335 is useful for testing the ASN lookup path.

### Complete JSON response

``` sh
rdap --json osbornepro.com
```

### Include RDAP notices

``` sh
rdap --notices osbornepro.com
```

### Include RDAP links

``` sh
rdap --links osbornepro.com
```

### Follow a registrar RDAP referral

When the registry response provides a related registrar RDAP endpoint,
`--follow` retrieves it and displays the registrar response after the
registry response:

``` sh
rdap --follow osbornepro.com
```

Options can be combined:

``` sh
rdap --follow --links --notices osbornepro.com
```

### Follow a registrar and return JSON

When `--json` and `--follow` are combined, the output remains a single
valid JSON document:

``` sh
rdap --json --follow osbornepro.com
```

The result is structured as:

``` json
{
  "registry": {
    "...": "registry RDAP response"
  },
  "registrar": {
    "...": "registrar RDAP response"
  }
}
```

If no related registrar RDAP response is available, `registrar` is
`null`.

### Extended human-readable output

`--full` displays additional RDAP information while remaining
human-readable. It also implies `--notices` and `--links`.

``` sh
rdap --full osbornepro.com
```

Full mode can be combined with registrar following:

``` sh
rdap --full --follow osbornepro.com
```

Depending on what the RDAP server publishes, full mode can include:

-   entity roles and handles
-   registrar, registrant, administrative, technical, and abuse entities
-   names and organizations
-   email addresses and telephone numbers
-   postal addresses
-   contact URIs
-   public identifiers
-   entity and top-level remarks
-   privacy/redaction notices
-   RDAP conformance identifiers
-   language metadata
-   legacy `port43` metadata supplied inside RDAP responses
-   additional nameserver status and metadata

`port43` values shown in full mode are metadata returned by an RDAP
server. The client itself still performs RDAP-only HTTPS lookups and
does not contact WHOIS servers.

### Show the authoritative RDAP server

``` sh
rdap --server osbornepro.com
```

### Show discovery details

``` sh
rdap --verbose osbornepro.com
```

Verbose diagnostics are written to standard error and can show the query
type, IANA bootstrap registry, bootstrap match, discovered RDAP server,
request URL, and followed registrar RDAP URL.

## Dependencies

-   a C11 compiler
-   libcurl
-   json-c
-   pkg-config

### Debian / Ubuntu

``` sh
sudo apt install -y build-essential pkg-config libcurl4-openssl-dev libjson-c-dev
```

### Fedora / RHEL 10

``` sh
sudo dnf install -y gcc make pkgconf-pkg-config libcurl-devel json-c-devel
```

### Arch Linux

``` sh
sudo pacman -S base-devel curl json-c
```

### macOS with Homebrew

``` sh
brew install curl json-c pkg-config
```

## Build

Build the project with:

``` sh
make
```

After a successful build, the `rdap` executable is created in the
project directory:

``` sh
./rdap osbornepro.com
```

### Clock Skew Warning

If `make` reports a warning similar to:

``` text
make: Warning: File 'Makefile' has modification time 21208 s in the future
make: warning: Clock skew detected. Your build may be incomplete.
```

one or more project files have modification timestamps ahead of the
system's current time. This can happen after downloading, extracting,
copying, or transferring the repository between systems.

Reset the project timestamps and rebuild with:

``` sh
find . -exec touch {} +
make clean
make
```

If the warning continues, verify that the system clock is correct and
synchronized:

``` sh
timedatectl
```

On systems using `systemd`, automatic network time synchronization can
be enabled with:

``` sh
sudo timedatectl set-ntp true
```

## Install

The preferred installation method is:

``` sh
sudo make install
```

The default installation path is:

``` text
/usr/local/bin/rdap
```

After installation:

``` sh
rdap osbornepro.com
```

A custom prefix can be used:

``` sh
sudo make PREFIX=/usr install
```

To uninstall:

``` sh
sudo make uninstall
```

For a quick manual installation of a locally built binary, you can also
use:

``` sh
sudo cp rdap /usr/local/bin/rdap
```

If your shell has cached the location of an older `rdap` binary, refresh
it with:

``` sh
hash -r
```

## Tests

The smoke tests make real network requests:

``` sh
make test
```

Useful manual tests include:

``` sh
rdap osbornepro.com
rdap 1.1.1.1
rdap 2606:4700:4700::1111
rdap AS13335
rdap --full --follow osbornepro.com
rdap --json --follow osbornepro.com
```

## How discovery works

The program downloads the appropriate IANA bootstrap registry:

-   `dns.json` for domains
-   `ipv4.json` for IPv4
-   `ipv6.json` for IPv6
-   `asn.json` for AS numbers

It then follows the RFC 9224 matching rules:

-   domain: longest label-wise suffix match
-   IP address: longest CIDR prefix match
-   ASN: containing number range

After finding the RDAP base URL, the program sends the appropriate HTTPS
request:

``` text
<base>/domain/<name>
<base>/ip/<address>
<base>/autnum/<number>
```

Only HTTPS service URLs are accepted.

For domain responses, `--follow` can follow an HTTPS `rel=related` RDAP
link when the registry publishes a related registrar RDAP endpoint.

## Human-readable vs. JSON output

The default output is intentionally concise and focuses on useful
registration information such as:

-   object type and handle
-   domain or network name
-   status
-   registrar information
-   abuse contact
-   registration events
-   nameservers
-   DNSSEC information

Use `--full` when you want additional human-readable entity and metadata
details.

Use `--json` when you need the complete unmodified RDAP response for
scripting, troubleshooting, or fields that are not rendered by the
human-readable formatter.

## Why not WHOIS?

WHOIS is a plain-text protocol traditionally served over TCP port 43.
RDAP provides structured JSON over HTTP(S), standardized service
discovery, and a protocol better suited to modern clients and
automation.

This project intentionally has no WHOIS fallback.

Some RDAP responses may themselves contain a `port43` field identifying
a legacy WHOIS service. `rdap-cli` may display that server-provided
value in full output, but it does not connect to it.

## Current limitations

The client intentionally remains small and focused. Current limitations
include:

-   IANA bootstrap files are retrieved on every invocation rather than
    cached
-   internationalized domain names are not yet converted to
    A-label/Punycode before lookup
-   RDAP search endpoints are not implemented
-   HTTP caching headers are not yet honored
-   human-readable output can only display fields that a particular RDAP
    server actually publishes

The complete server response remains available through `--json`.

Contributions addressing these limitations are welcome.

## Project layout

``` text
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

-   RFC 9082 --- RDAP query format
-   RFC 9083 --- RDAP JSON responses
-   RFC 9224 --- finding authoritative RDAP services

IANA publishes the live RDAP bootstrap registries at:

`https://data.iana.org/rdap/`

## Author

Developed and maintained by **OsbornePro**.

Project repository:

`https://github.com/OsbornePro/rdap-cli`

## License

This project is licensed under the **GNU General Public License v3.0**.

See the `LICENSE` file in the repository for the complete license text.

If the source files use SPDX identifiers, they should match the
repository's GPLv3 licensing choice, for example:

``` text
SPDX-License-Identifier: GPL-3.0-only
```
