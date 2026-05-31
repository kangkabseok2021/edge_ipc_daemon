# Contributing

This project follows the [open62541 contribution guide](https://github.com/open62541/open62541/blob/master/CONTRIBUTING.md).

## DCO sign-off

Every commit must include a `Signed-off-by` line:

```bash
git commit -s -m "feat(server): description"
```

## Branch naming

`fix/<issue-number>-short-description`  
`feat/<issue-number>-short-description`

## Commit message format

```
type(scope): short description (72 chars max)

Body: WHY this change is needed, not WHAT it does.
Reference measured data where relevant (Ir counts, latency percentiles).

Signed-off-by: Your Name <email@example.com>
```

Types: `feat` · `fix` · `perf` · `test` · `docs` · `ci` · `refactor`

## Pull request checklist

- [ ] `Valgrind --tool=memcheck`: 0 leaks
- [ ] `AddressSanitizer`: 0 errors  
- [ ] `callgrind_annotate` output for any hot-path change
- [ ] `docs/BENCHMARK-REPORT.md` updated with before/after P99 numbers
- [ ] `CHANGELOG.md` [Unreleased] section updated

## Code of Conduct

[Contributor Covenant v2.1](CODE-OF-CONDUCT.md)
