# Output

`rdap-cli` provides concise human-readable terminal output and complete JSON
output.

## Human-readable domain output

A domain response can include fields such as:

```text
Type: domain
Name: OSBORNEPRO.COM
Handle: 2371440810_DOMAIN_COM-VRSN
Status: client transfer prohibited

Registrar:
  Name: Cloudflare, Inc.
  IANA ID: 1910

Abuse Contact:
  Email: registrar-abuse@cloudflare.com
  Phone: +1.6503198930

Events:
  Registration             2019-03-20T20:17:59Z
  Expiration               2035-03-20T20:17:59Z
  Last changed             2025-10-31T18:32:15Z

Nameservers:
  MILLIE.NS.CLOUDFLARE.COM
  TADEO.NS.CLOUDFLARE.COM

DNSSEC:
  Delegation signed: yes
```

Actual values change over time and depend on the authoritative RDAP provider.

## Registrar and abuse information

The client can display registrar name and IANA ID as well as published abuse
contact details. Registry and registrar responses may publish different contact
information, which is one reason `--follow` can be useful.

## Events

Common event actions are displayed with friendlier labels such as:

```text
Registration
Expiration
Last changed
RDAP database updated
```

## DNSSEC

If `secureDNS` data is present, output may include whether delegation is signed
and DS details such as key tag, algorithm, digest type, and digest.

## IPv4 and IPv6 output

Network responses can include type, name, handle, registration type, country,
IP version, parent handle, start address, end address, status, and events.

## ASN output

ASN responses can include type, name, handle, start ASN, end ASN, country,
status, and events.

## Full output

```bash
rdap --full osbornepro.com
```

Extended human-readable data can include entity roles, handles, names,
organizations, email addresses, phone numbers, postal addresses, contact URIs,
public identifiers, remarks, redaction notices, RDAP conformance values,
language, `port43` metadata, and nameserver details.

`--full` automatically enables notices and links.

## Registrar follow output

```bash
rdap --follow osbornepro.com
```

When a suitable registrar referral is available, the registry summary is
followed by a separate `Registrar RDAP:` section.

## Complete JSON

```bash
rdap --json osbornepro.com
```

JSON mode preserves the complete RDAP object returned by the authoritative
service and is preferred for scripting.

## JSON with registrar follow

```bash
rdap --json --follow osbornepro.com
```

The result is wrapped as:

```json
{
  "registry": {},
  "registrar": {}
}
```

If no registrar response is available, `registrar` is `null`.

## Standard output and standard error

Normal human-readable and JSON data are written to standard output. Verbose
diagnostics are written to standard error, so this works cleanly:

```bash
rdap --verbose --json osbornepro.com > response.json
```

## Server-provided port43 metadata

Some RDAP responses include a `port43` field. `rdap-cli` may display it in
extended output because it is part of the RDAP response, but the client does
**not** connect to that WHOIS server.
