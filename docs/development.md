# Development

Contributions to `rdap-cli` are welcome.

## Design goals

The project is intentionally focused on:

```text
RDAP only
No WHOIS fallback
IANA bootstrap discovery
HTTPS-only RDAP service access
Portable C11
Minimal dependencies
Concise human-readable output
Complete JSON output
Safe handling of untrusted network data
```

## Clone

```bash
git clone https://github.com/OsbornePro/rdap-cli.git
cd rdap-cli
```

## Dependencies

RHEL 10 / Fedora:

```bash
sudo dnf install gcc make pkgconf-pkg-config libcurl-devel json-c-devel
```

Debian / Ubuntu / Kali:

```bash
sudo apt update
sudo apt install build-essential pkg-config libcurl4-openssl-dev libjson-c-dev
```

## Build

```bash
make
```

Clean rebuild:

```bash
make clean
make
```

New code should compile cleanly with the project's C11 warning flags.

## Test suite

```bash
make test
```

The current tests may require network connectivity because IANA bootstrap and
RDAP services are network resources.

## Manual testing

```bash
./rdap osbornepro.com
./rdap 1.1.1.1
./rdap 2606:4700:4700::1111
./rdap AS13335
./rdap 13335
```

Test output modes:

```bash
./rdap --notices osbornepro.com
./rdap --links osbornepro.com
./rdap --follow osbornepro.com
./rdap --full osbornepro.com
./rdap --full --follow osbornepro.com
./rdap --json osbornepro.com
./rdap --json --follow osbornepro.com
./rdap --server osbornepro.com
./rdap --verbose osbornepro.com
```

## JSON output contract

Without `--follow`, JSON output is the complete RDAP object returned by the
authoritative service.

With `--json --follow`, output is wrapped as:

```json
{
  "registry": {},
  "registrar": {}
}
```

If no registrar response is available, `registrar` is `null`. Changes to this
behavior can break scripts and should be treated as a compatibility concern.

## Human-readable output

Default output should remain concise. Fields that are useful but too verbose for
the default display should generally be exposed through `--full`, `--notices`,
`--links`, or `--json`.

## IANA bootstrap discovery

The client uses:

```text
https://data.iana.org/rdap/dns.json
https://data.iana.org/rdap/ipv4.json
https://data.iana.org/rdap/ipv6.json
https://data.iana.org/rdap/asn.json
```

Domain discovery uses DNS suffix matching. IP discovery uses prefix matching.
ASN discovery selects a service whose advertised ASN range contains the query.

## Memory ownership

The project uses json-c. Be especially careful with ownership and reference
counts around `json_object_get()`, `json_object_put()`, inserted objects,
wrapper objects, followed registrar responses, and cleanup paths.

## libcurl

Changes involving libcurl should preserve TLS certificate validation,
HTTPS-only behavior, safe redirect handling, useful error messages, and clean
resource cleanup.

## Error handling

Errors should go to standard error and should identify the failing stage where
possible: bootstrap retrieval, parsing, service discovery, URL construction,
RDAP request, JSON parsing, or referral handling.

## Security

All remote content is untrusted. Extra care is required when changing buffer
handling, string construction, URL processing, recursive entity parsing, CIDR
matching, ASN range matching, redirect behavior, and network response handling.

Potential vulnerabilities should not be reported through public issues.

## Documentation

If a change affects user-visible behavior, update the relevant documentation,
including `README.md`, `docs/usage.md`, `docs/examples.md`, `docs/output.md`,
`docs/installation.md`, `CONTRIBUTING.md`, or `SECURITY.md` as appropriate.

## Pull requests

Before submitting a pull request:

```bash
make clean
make
make test
```

Also run representative manual queries and document what changed, why it is
needed, how it was tested, whether CLI or JSON behavior changed, and whether the
documentation was updated.

## Licensing

`rdap-cli` is licensed under GNU GPL v3. New source files should use:

```text
SPDX-License-Identifier: GPL-3.0-only
```

## Project philosophy

The goal is better RDAP support, not WHOIS compatibility through fallback.
