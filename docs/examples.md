# Examples

## Domain lookups

```bash
rdap osbornepro.com
rdap --json osbornepro.com
rdap --notices osbornepro.com
rdap --links osbornepro.com
rdap --server osbornepro.com
rdap --verbose osbornepro.com
```

## Registrar referral

```bash
rdap --follow osbornepro.com
rdap --full --follow osbornepro.com
rdap --json --follow osbornepro.com
```

## IPv4 lookups

Cloudflare:

```bash
rdap 1.1.1.1
```

Google Public DNS:

```bash
rdap 8.8.8.8
```

Additional modes:

```bash
rdap --full 1.1.1.1
rdap --json 1.1.1.1
rdap --server 1.1.1.1
rdap --verbose 1.1.1.1
```

## IPv6 lookups

Cloudflare:

```bash
rdap 2606:4700:4700::1111
```

Google:

```bash
rdap 2001:4860:4860::8888
```

Additional modes:

```bash
rdap --full 2606:4700:4700::1111
rdap --json 2606:4700:4700::1111
```

## ASN lookups

Cloudflare's primary ASN:

```bash
rdap AS13335
rdap 13335
```

Additional modes:

```bash
rdap --full AS13335
rdap --json AS13335
rdap --server AS13335
rdap --verbose AS13335
```

## Troubleshooting a domain lookup

```bash
rdap --verbose --follow osbornepro.com
```

This helps confirm which bootstrap file was used, which entry matched, which
RDAP service was selected, which URL was requested, and whether a registrar
referral was followed.

## Working with jq

Extract the LDH name:

```bash
rdap --json osbornepro.com | jq '.ldhName'
```

List statuses:

```bash
rdap --json osbornepro.com | jq '.status'
```

Inspect events:

```bash
rdap --json osbornepro.com | jq '.events'
```

Inspect entities:

```bash
rdap --json osbornepro.com | jq '.entities'
```

Inspect the followed registrar response:

```bash
rdap --json --follow osbornepro.com | jq '.registrar'
```
