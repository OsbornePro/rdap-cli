# Security Policy

## Reporting a vulnerability

Please do not publish a suspected security vulnerability in a public issue.

Use GitHub's private vulnerability reporting feature for this repository if it
is enabled. Otherwise, contact the repository maintainer privately using the
contact method listed on their GitHub profile.

## Scope

`rdap` processes untrusted JSON received over the network. Security reports
involving malformed RDAP/bootstrap responses, memory safety, URL handling,
redirect behavior, or TLS validation are particularly relevant.

The program intentionally permits HTTPS only for bootstrap-selected RDAP
services.
