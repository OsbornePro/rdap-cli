# Usage

The basic syntax is:

```bash
rdap [options] QUERY
```

`QUERY` may be a domain name, IPv4 address, IPv6 address, ASN prefixed with
`AS`, or a numeric ASN.

## Query type detection

Domain:

```bash
rdap osbornepro.com
```

IPv4:

```bash
rdap 1.1.1.1
```

IPv6:

```bash
rdap 2606:4700:4700::1111
```

ASN:

```bash
rdap AS13335
```

Numeric ASN:

```bash
rdap 13335
```

## Command-line options

```text
-j, --json       Print complete RDAP JSON; with --follow, wrap both responses
-n, --notices    Include RDAP notices in human-readable output
-l, --links      Include top-level RDAP links in human-readable output
-f, --follow     Follow an HTTPS registrar RDAP related link when available
-F, --full       Extended human-readable output; implies --notices and --links
-s, --server     Print the discovered authoritative RDAP base and exit
-v, --verbose    Print bootstrap and request diagnostics to standard error
-V, --version    Print the program version and exit
-h, --help       Print help and exit
```

## Default output

```bash
rdap osbornepro.com
```

The concise human-readable view may include type, name, handle, status,
registrar, registrar IANA ID, abuse contact, events, nameservers, DNSSEC, and DS
records. Not every RDAP server publishes every field.

## Complete JSON

```bash
rdap --json osbornepro.com
```

Use JSON mode for scripts or when you need every field returned by the server.

## Notices

```bash
rdap --notices osbornepro.com
```

## Links

```bash
rdap --links osbornepro.com
```

## Follow registrar RDAP

```bash
rdap --follow osbornepro.com
```

For domain registrations, the registry may provide a related registrar RDAP
link. If an acceptable HTTPS RDAP link is available, the client fetches the
registrar response and prints it after the registry summary.

## JSON with registrar follow

```bash
rdap --json --follow osbornepro.com
```

Output is wrapped as:

```json
{
  "registry": {},
  "registrar": {}
}
```

If no registrar response is available:

```json
{
  "registry": {},
  "registrar": null
}
```

## Full human-readable output

```bash
rdap --full osbornepro.com
```

`--full` automatically enables `--notices` and `--links`. Extended output can
include entity roles and handles, names and organizations, email and phone
numbers, postal addresses, contact URIs, public identifiers, remarks, redaction
information, RDAP conformance identifiers, language, `port43` metadata, and
nameserver details.

## Full output with registrar follow

```bash
rdap --full --follow osbornepro.com
```

## Print the authoritative server

```bash
rdap --server osbornepro.com
```

## Verbose mode

```bash
rdap --verbose osbornepro.com
```

Verbose diagnostics can include the bootstrap URL, matched bootstrap entry,
selected RDAP server, and final request URL. Diagnostics are written to standard
error.

## HTTPS-only behavior

`rdap-cli` uses HTTPS for RDAP service URLs and supported referral links. It does
not use WHOIS and does not query TCP port 43. A `port43` field in an RDAP
response is metadata only.
