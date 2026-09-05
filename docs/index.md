# rdap-cli

**rdap-cli** is a lightweight RDAP-only command-line client written in C.

It provides a modern alternative to the traditional `whois` command by querying
Registration Data Access Protocol (RDAP) services over HTTPS and returning either
concise human-readable information or complete RDAP JSON.

The project intentionally does **not** fall back to WHOIS.

## Features

- Written in C11.
- RDAP only; no WHOIS fallback.
- Automatic query type detection.
- Domain, IPv4, IPv6, and ASN lookups.
- Automatic authoritative RDAP discovery using IANA bootstrap registries.
- HTTPS-only RDAP service access.
- Concise human-readable output by default.
- Complete JSON output with `--json`.
- Registrar RDAP referral following with `--follow`.
- Extended human-readable output with `--full`.
- RDAP notices and links.
- DNSSEC and DS record display for domains.
- Registrar and abuse-contact information.
- Verbose discovery and request diagnostics.

## Quick start

Build the client:

```bash
make
```

Run a domain lookup:

```bash
./rdap osbornepro.com
```

Look up an IPv4 address:

```bash
./rdap 1.1.1.1
```

Look up an IPv6 address:

```bash
./rdap 2606:4700:4700::1111
```

Look up an ASN:

```bash
./rdap AS13335
```

Numeric ASNs are also accepted:

```bash
./rdap 13335
```

## Complete JSON

Use `--json` when you need the complete RDAP response:

```bash
rdap --json osbornepro.com
```

When `--json` and `--follow` are used together, the output contains both the
registry and registrar responses:

```json
{
  "registry": {},
  "registrar": {}
}
```

If no registrar RDAP referral is available, the `registrar` value is `null`.

## Authoritative service discovery

`rdap-cli` uses the appropriate IANA RDAP bootstrap registry:

```text
https://data.iana.org/rdap/dns.json
https://data.iana.org/rdap/ipv4.json
https://data.iana.org/rdap/ipv6.json
https://data.iana.org/rdap/asn.json
```

The selected authoritative service is queried directly over HTTPS. This follows
the RDAP bootstrap model described by RFC 9224.

## Project

Source code:

```text
https://github.com/OsbornePro/rdap-cli
```

The project is licensed under the GNU General Public License version 3.
