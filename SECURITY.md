# Security Policy

## Supported Versions

Security fixes are currently provided for the latest release of `rdap-cli`.

| Version | Supported |
| ------- | --------- |
| 0.3.x   | Yes       |
| < 0.3   | No        |

Users are encouraged to run the latest available release.

## Reporting a Vulnerability

Please **do not** report suspected security vulnerabilities through a public GitHub issue, discussion, pull request, or other public channel.

Use GitHub's private vulnerability reporting feature for this repository if it is enabled.

If private vulnerability reporting is unavailable, contact the repository maintainer privately using the contact information listed on their GitHub profile.

When submitting a report, please include as much of the following information as possible:

- the affected `rdap-cli` version
- operating system and architecture
- a description of the vulnerability
- steps required to reproduce the issue
- the query that triggered the issue, when applicable
- the RDAP or IANA bootstrap response involved, when applicable
- expected and actual behavior
- crash output, debugger output, or sanitizer output, if available
- the potential security impact

Please allow reasonable time for the issue to be investigated and corrected before publicly disclosing the vulnerability.

## Security Scope

`rdap-cli` is a network-facing command-line application written in C. 
It processes data received from IANA bootstrap services and third-party RDAP servers. 
This data must be considered untrusted.

Security reports involving the following areas are particularly relevant:

- memory corruption or other memory-safety errors
- buffer overflows or out-of-bounds access
- use-after-free or double-free conditions
- integer overflow or underflow
- crashes caused by malformed or unexpected JSON
- malformed IANA RDAP bootstrap data
- malformed RDAP responses
- unsafe URL construction or parsing
- unsafe redirect handling
- HTTPS or TLS certificate validation problems
- protocol downgrade behavior
- unexpected non-HTTPS network requests
- incorrect authoritative RDAP service discovery
- IPv4 or IPv6 prefix matching errors with security implications
- ASN range parsing errors with security implications
- excessive memory or resource consumption caused by network responses
- arbitrary code execution
- unintended disclosure of local information

Normal lookup failures, unsupported RDAP server behavior, incomplete registration data, or differences between registry and registrar responses are generally not security vulnerabilities unless they result in a security impact.

## Network Security

`rdap-cli` intentionally performs registration lookups using RDAP over HTTPS.

The client does **not** fall back to WHOIS and does **not** intentionally make TCP port 43 queries.

Only HTTPS RDAP service URLs discovered through the IANA bootstrap registries are accepted.

HTTP redirects are restricted to HTTPS destinations.

Some RDAP responses may contain a `port43` field identifying a legacy WHOIS server. 
This value is server-provided metadata. `rdap-cli` may display this information but does not connect to the listed WHOIS server.

## Untrusted Input

All network responses must be treated as untrusted input.

This includes data received from:

- IANA RDAP bootstrap registries
- domain registries
- domain registrars
- Regional Internet Registries (RIRs)
- other authoritative RDAP services

Malformed, unusually large, deeply nested, incomplete, or otherwise unexpected responses should fail safely and must not cause memory corruption or arbitrary code execution.

Command-line queries supplied by users must also be treated as untrusted input.

## RDAP Referral Following

The `--follow` option may follow a related RDAP URL supplied by the initial RDAP response.

Related URLs must use HTTPS.

A related URL should never cause `rdap-cli` to downgrade to an insecure protocol or invoke a WHOIS service.

Because referral URLs originate from network responses, they must be treated as untrusted input.

## JSON Output

The `--json` option prints data received from RDAP servers.

When `--json` and `--follow` are used together, responses from both the registry and registrar may be included in the resulting JSON document.

Consumers of this output should treat all returned fields as untrusted data, especially when passing the output to other programs or scripts.

## Full Output

The `--full` option can expose additional information supplied by RDAP servers, including entity information, contact information, remarks, redaction notices, links, and other metadata.

This information is displayed as supplied by the relevant RDAP service and should not be interpreted as trusted local data.

`rdap-cli` does not attempt to bypass RDAP redaction or authorization mechanisms.

## Dependencies

`rdap-cli` relies on external libraries for important security-sensitive
functionality, including:

- libcurl for HTTPS networking and TLS handling
- json-c for JSON parsing

Users should keep these libraries and their operating system security updates current.

Security vulnerabilities originating entirely within an upstream dependency should generally be reported to that project's maintainers. 
Reports are still welcome if `rdap-cli` uses an affected dependency in a way that creates a specific vulnerability in this project.

## Denial of Service and Resource Limits

RDAP and bootstrap responses are received from remote systems and may vary significantly in size and complexity.

Reports involving responses that cause unreasonable memory consumption, excessive CPU usage, hangs, or crashes are welcome, particularly when the behavior can be triggered by a remote RDAP service.

Future releases may introduce additional limits on response size, nesting, or other resource usage.

## Privacy

RDAP queries are sent to remote RDAP services over HTTPS.

Users should understand that the remote service can observe information necessary to process the request, including the requested domain name, IP address, or ASN, as well as network information normally visible to an HTTPS server.

`rdap-cli` does not provide anonymity.

## Security Design Goals

The project aims to maintain the following security properties:

1. Registration lookups use RDAP rather than WHOIS.
2. RDAP network requests use HTTPS.
3. Redirects do not downgrade requests to insecure protocols.
4. Network-provided JSON is treated as untrusted input.
5. Malformed responses fail safely.
6. Registrar referral following remains HTTPS-only.
7. Server-provided `port43` values are treated only as metadata.
8. The client does not attempt to circumvent RDAP access controls or
   redaction.
9. Errors should fail closed where practical rather than silently falling back
   to a less secure protocol.

## Disclosure Process

After receiving a vulnerability report, the maintainer will attempt to:

1. acknowledge the report
2. reproduce and assess the issue
3. determine affected versions
4. develop and test a correction
5. prepare a new release when necessary
6. coordinate public disclosure when appropriate

The exact timeline will depend on the complexity and severity of the issue.

Thank you for helping keep `rdap-cli` and its users secure.
