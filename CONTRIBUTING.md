# Contributing to rdap-cli

Contributions to `rdap-cli` are welcome.

The project aims to remain a small, understandable, standards-based RDAP command-line client written in C.

Before submitting a change, please make sure it preserves the project's core design goals:

- RDAP only
- no WHOIS fallback
- authoritative discovery using the IANA RDAP bootstrap registries
- HTTPS-only RDAP requests
- useful human-readable output
- complete machine-readable output through `--json`
- minimal mandatory dependencies
- portable C11 code where practical

## Getting the Source

Clone the repository:

```sh
git clone https://github.com/OsbornePro/rdap-cli.git
cd rdap-cli
````

## Dependencies

The project currently requires:

* a C11 compiler
* make
* libcurl
* json-c
* pkg-config

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

## Building

Build the project with:

```sh
make
```

The supplied Makefile enables compiler warnings by default.

A normal build should complete without warnings.

For a clean rebuild:

```sh
make clean
make
```

Please do not introduce additional mandatory dependencies unless there is a strong technical reason.

Optional dependencies for future features may be considered, but their purpose and impact should be explained in the pull request.

## Testing

Run the project's smoke tests before submitting a pull request:

```sh
make test
```

The smoke tests require Internet access because `rdap-cli` performs live IANA bootstrap discovery and sends requests to authoritative RDAP services.

Changes affecting lookup behavior should also be tested manually.

A useful basic test set is:

```sh
./rdap osbornepro.com
./rdap 1.1.1.1
./rdap 2606:4700:4700::1111
./rdap AS13335
./rdap 13335
```

This exercises:

* domain lookup
* IPv4 lookup
* IPv6 lookup
* ASN lookup using the `AS` prefix
* ASN lookup using a bare number

## Testing Output Modes

Changes affecting output should be tested against the relevant output modes.

### Default human-readable output

```sh
./rdap osbornepro.com
```

### Notices

```sh
./rdap --notices osbornepro.com
```

### Links

```sh
./rdap --links osbornepro.com
```

### Registrar referral following

```sh
./rdap --follow osbornepro.com
```

### Extended human-readable output

```sh
./rdap --full osbornepro.com
```

### Extended output with registrar following

```sh
./rdap --full --follow osbornepro.com
```

### Complete JSON

```sh
./rdap --json osbornepro.com
```

### Registry and registrar JSON

```sh
./rdap --json --follow osbornepro.com
```

### Authoritative RDAP server discovery

```sh
./rdap --server osbornepro.com
```

### Verbose discovery

```sh
./rdap --verbose osbornepro.com
```

When changing shared lookup or parsing code, contributors should test more than just domain lookups. IPv4, IPv6, and ASN behavior should also be checked.

## JSON Output

`--json` is intended to provide the complete RDAP response rather than a reformatted subset.

Avoid removing, renaming, or rewriting server-provided fields in JSON output.

When `--json` and `--follow` are combined, the output is a single JSON object containing the registry and registrar responses:

```json
{
  "registry": {
    "...": "registry RDAP response"
  },
  "registrar": {
    "...": "registrar RDAP response"
  }
}
```

If no registrar response is available, `registrar` should be `null`.

Changes to this structure should be treated as an interface change because scripts may depend on it.

## Human-Readable Output

The default human-readable output should remain concise.

It should focus on commonly useful registration information rather than attempting to display every field returned by an RDAP server.

More detailed information belongs in `--full`.

The complete response remains available through `--json`.

When adding new human-readable fields:

* handle missing fields gracefully
* do not assume every RDAP implementation returns the same fields
* preserve meaningful server-provided values
* avoid printing empty sections
* keep formatting readable in a normal terminal
* avoid duplicating information unnecessarily

## Full Output

`--full` is the extended human-readable mode.

It may display additional information such as:

* RDAP entities
* entity roles
* handles
* organizations
* email addresses
* telephone numbers
* postal addresses
* public identifiers
* contact URIs
* remarks
* redaction information
* RDAP conformance identifiers
* nameserver details
* server-provided metadata

`--full` currently implies:

```text
--notices
--links
```

Changes to full output should continue to treat all network-provided values as untrusted data.

## Registrar Referral Following

`--follow` follows a related registrar RDAP link when one is supplied by the initial RDAP response.

Referral handling must remain RDAP-only and HTTPS-only.

Do not add behavior that follows a referral to:

* HTTP
* WHOIS
* TCP port 43
* another insecure protocol

Referral URLs originate from untrusted network responses and must be handled accordingly.

Changes to referral handling should be tested with both:

```sh
./rdap --follow osbornepro.com
./rdap --json --follow osbornepro.com
```

and, where relevant:

```sh
./rdap --full --follow osbornepro.com
```

## IANA Bootstrap Discovery

Authoritative RDAP service discovery should follow the applicable RDAP standards and IANA bootstrap registries.

The current lookup types use:

* `dns.json` for domains
* `ipv4.json` for IPv4
* `ipv6.json` for IPv6
* `asn.json` for Autonomous System Numbers

Matching behavior should preserve:

* longest label-wise suffix matching for domains
* longest CIDR prefix matching for IP addresses
* containing range matching for ASNs

Changes to bootstrap discovery should be tested carefully because incorrect matching can direct queries to the wrong RDAP service.

## C Style

The project uses C11.

Please keep the code straightforward and understandable.

In particular:

* check return values
* check allocation failures
* free allocated resources
* avoid unnecessary global state
* prefer bounded operations
* validate parsed numeric values
* validate array and object types before accessing JSON values
* handle missing JSON fields gracefully
* treat all network data as untrusted
* avoid undefined behavior
* avoid compiler warnings
* keep functions reasonably focused
* use descriptive names
* comment behavior that is not obvious from the code

Do not optimize code at the expense of correctness or readability without a demonstrated need.

## Memory Safety

Because `rdap-cli` is written in C and processes untrusted network responses, memory safety is particularly important.

Changes should be reviewed for:

* buffer overflows
* out-of-bounds access
* use-after-free
* double-free
* memory leaks
* unchecked allocation failures
* integer overflow
* incorrect ownership of json-c objects
* unbounded resource consumption

Remember that some json-c functions transfer ownership or manipulate reference counts. Changes involving `json_object_get()`, `json_object_put()`, or `json_object_object_add()` should be reviewed carefully.

## Network Security

All RDAP network access must remain HTTPS-only.

Do not introduce:

* plaintext HTTP RDAP requests
* insecure redirect handling
* TLS certificate verification bypasses
* WHOIS fallback
* TCP port 43 queries

Some RDAP responses contain a `port43` field. This is server-provided metadata and does not mean the client should connect to that service.

Code handling URLs, redirects, referrals, or bootstrap services should receive additional security review.

## Error Handling

Errors should be useful without being unnecessarily verbose.

Fatal errors should normally be written to standard error.

Machine-readable JSON output should not be contaminated with diagnostic messages on standard output.

Where practical, the program should fail safely rather than silently changing protocols or falling back to another service.

## Compatibility

Avoid unnecessary breaking changes to existing command-line behavior.

Existing options include:

```text
-j, --json
-n, --notices
-l, --links
-f, --follow
-F, --full
-s, --server
-v, --verbose
-V, --version
-h, --help
```

Before changing an option's meaning or output structure, consider whether existing users or scripts may depend on the current behavior.

New options should use a short option only when an appropriate unused character is available.

## Documentation

Changes that affect user-visible behavior should update the relevant documentation.

Depending on the change, this may include:

* `README.md`
* `CONTRIBUTING.md`
* `SECURITY.md`
* command-line `--help`
* tests
* release notes or changelog

Examples in documentation should be kept consistent with actual program behavior.

## Security Issues

Do not open a public issue for a suspected security vulnerability.

Please follow the reporting process described in:

```text
SECURITY.md
```

Security-sensitive changes should avoid including exploit details in public discussion before a fix is available.

## Pull Requests

A useful pull request should clearly explain:

* what problem it solves
* what behavior changes
* why the change is needed
* how the change was tested
* whether command-line behavior changes
* whether output formats change
* whether new dependencies are introduced
* whether documentation was updated

Where appropriate, add or update tests.

Keep pull requests focused. Unrelated cleanup or formatting changes should generally be submitted separately from functional changes.

## Commit Messages

Use concise commit messages that describe the change.

Examples:

```text
Add bootstrap response caching
Add IDN domain normalization
Improve IPv6 bootstrap matching tests
Add response size limit
Fix registrar entity parsing
Document full output mode
```

Avoid vague messages such as:

```text
update
changes
fix stuff
```

## Licensing

By contributing to this project, you agree that your contributions will be distributed under the same license as the project.

`rdap-cli` is licensed under the GNU General Public License v3.0.

Source files should use the project's selected SPDX identifier consistently, for example:

```text
SPDX-License-Identifier: GPL-3.0-only
```

Do not submit code that cannot legally be distributed under the project's license.

## Project Philosophy

`rdap-cli` is intended to provide a modern command-line registration lookup tool without carrying forward the legacy WHOIS protocol.

Please keep that goal in mind when proposing features.

The preferred direction is:

**better RDAP support, not WHOIS compatibility through fallback.**

Thank you for contributing to `rdap-cli`.
