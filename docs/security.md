# Security

`rdap-cli` communicates with remote RDAP and IANA bootstrap services and parses
untrusted JSON received over the network.

## Reporting a vulnerability

Please do not disclose suspected security vulnerabilities through a public
GitHub issue.

Use GitHub's private vulnerability reporting feature for the repository when it
is available.

Security contact information is also published at:

```text
https://osbornepro.com/.well-known/security.txt
```

Pseudonymous reports are welcome. A real name is not required. Encrypted reports
are welcome using the public key referenced by the OsbornePro `security.txt`
file.

## Supported versions

| Version | Supported |
| --- | --- |
| 0.3.x | Yes |
| Earlier versions | No |

## Relevant security issues

Security reports are especially useful when they involve:

- Memory corruption or out-of-bounds access.
- Buffer overflows.
- Use-after-free or double-free conditions.
- Integer overflows.
- Malformed JSON, bootstrap, or RDAP responses.
- Unsafe URL or redirect handling.
- TLS verification or protocol downgrade.
- Incorrect service discovery, CIDR matching, or ASN range matching.
- Resource exhaustion.
- Unexpected local disclosure or arbitrary code execution.

## Network security

RDAP service URLs selected by `rdap-cli` must use HTTPS. The client does not
fall back to WHOIS and does not intentionally query TCP port 43.

A server-provided `port43` field is treated only as metadata.

## IANA bootstrap data

The IANA bootstrap registries are network-provided JSON and must be treated as
untrusted input. Security-sensitive processing includes DNS suffix matching,
IPv4 and IPv6 CIDR matching, ASN range matching, and RDAP service URL selection.

## Registrar referrals

`--follow` may cause an additional HTTPS request to a registrar RDAP service.
The referred response is also untrusted JSON, and referral handling must
preserve the HTTPS-only security model.

## Redirects and TLS

Redirect handling must not allow an unexpected protocol downgrade. TLS
verification is handled through libcurl and the operating system trust store.
Users should keep libcurl and CA certificate packages up to date.

## Untrusted JSON

The parser should safely handle missing members, unexpected types, empty arrays
and objects, nested entities, malformed responses, unknown RDAP extensions, and
unexpected Unicode.

## Resource exhaustion

Remote services can return unusually large or deeply nested responses. Reports
involving excessive CPU or memory use are security relevant. Explicit response
size limits are a useful future hardening area.

## Dependencies

The main runtime libraries are:

```text
libcurl
json-c
```

Keep these and their transitive dependencies updated through the operating
system package manager.

## Privacy

RDAP queries are sent directly to external services. Those services can observe
the query, the client's source IP address, request time, and normal HTTPS
connection metadata. `rdap-cli` does not provide anonymity.

## Security testing

The normal build uses:

```text
-Wall
-Wextra
-Wpedantic
```

For additional sanitizer testing:

```bash
make clean
make \
  CFLAGS="-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer" \
  LDLIBS="$(pkg-config --libs libcurl json-c) -fsanitize=address,undefined"
```

Then run representative domain, IPv4, IPv6, ASN, JSON, full-output, and referral
tests.
