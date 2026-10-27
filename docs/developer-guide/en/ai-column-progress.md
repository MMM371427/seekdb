# AI column implementation checkpoint — not ready for submission

Baseline: `5bce9e06d1281347ad61344e4b01fab9158c58a2` (`2026-competition`).

This branch is an incomplete development checkpoint. Do not merge it into a
competition submission or treat the existing mysqltest cases as passed.

## Implemented source changes

- Native MySQL grammar for AI column definitions and async/retry options.
- Separate AI expression and retry-limit fields in column schema, deep copy,
  reset, UNIS serialization, system-table persistence and schema loading.
- Literal model and model-kind validation, vector dimension validation,
  deterministic input checks, rejection of direct AI-column dependencies.
- Native parser source spans preserve argument text while retaining decoded
  model literals. Completion prompts carry result-format instructions;
  omitted embedding dimensions use the declared VECTOR dimension.
- Three hidden physical columns per AI result and a metadata view derived from
  the persisted column schema. CREATE and ALTER ADD allocate the hidden columns.
- SHOW CREATE TABLE prints the AI definition instead of a DEFAULT expression.
- Parser and schema-copy/serialization test sources, with a Bazel test target.

## Verification actually performed

- Repository Bison 2.4.1 / Flex parser generation: passed.
- Generated MySQL parser and lexer Clang C syntax checks: passed.
- Modified column-schema implementation Clang C++20 syntax check: passed.
- New parser and schema test sources Clang C++20 syntax checks: passed.
- Inner-table/view source generation: passed.
- `git diff --check`: passed.

The unit tests have not been linked or run. No seekdb binary has been built
from these changes, and no AI-column mysqltest suite has been run.

Rust 1.98.1 has now been installed locally. Full native build and generated
`config_bridge.h` validation are still pending.

## Remaining work

- Transactional DML task creation, dependency invalidation and rollback.
- Durable queue, background worker, recovery, bounded retries and version guards.
- Atomic publication of every affected AI column in a request group.
- SHOW AI COLUMNS, AI_COLUMN_STATUS, scoped RETRY and strict-read semantics.
- Typed result validation, vector index maintenance and query regressions.
- ALTER MODIFY, rename/drop dependency handling and hidden-column cleanup.
- Upgrade path for existing data directories after adding system-table fields.
- Linked unit tests, complete mysqltest suites and combined-branch regressions.

Use a fresh isolated test data directory when eventually testing these new
system-table fields. Existing data directories have not been upgrade-tested.

The initial unit-test target is:

```sh
bazel test //unittest/sql:sql_tests --test_filter='AIColumn*.*'
```
