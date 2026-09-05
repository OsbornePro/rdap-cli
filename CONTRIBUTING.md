# Contributing

Contributions are welcome.

## Building

```sh
make
```

Please build with warnings enabled (the supplied Makefile does this by default)
and avoid introducing additional mandatory dependencies unless there is a
strong reason.

## Testing

```sh
make test
```

The current smoke tests require Internet access because RDAP is a network
protocol and the client performs live IANA bootstrap discovery.

## Style

- C11
- keep the client small and understandable
- check return values
- prefer bounded operations
- never add a WHOIS fallback: this project is intentionally RDAP-only
- authoritative service discovery should follow the applicable RDAP standards

## Pull requests

A useful pull request should explain what behavior changes and, where
appropriate, add or update tests.
