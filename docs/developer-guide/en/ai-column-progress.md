# AI column: frozen evaluation candidate

Implementation commit: `a1baeba4e4136ed2eb61564f9c682a9aa60cb1aa`.
Branch: `MMM371427/seekdb:codex/mmm371427-ai-column`.
Baseline: `5bce9e06d1281347ad61344e4b01fab9158c58a2` (`2026-competition`).

The user requested freezing development and proceeding toward evaluation.
The candidate has passed the local checks below. This is not an official
competition score or a claim that every lifecycle edge case is complete.
No merge has been performed. The repository's `2026-competition` branch was
still at the baseline when the candidate was prepared.

## Implemented behavior

- Native AI COLUMN grammar, async configuration, model/type/dimension checks,
  dependency validation, schema persistence and three hidden columns per result.
- SHOW CREATE TABLE, SHOW AI COLUMNS with WHERE/LIKE, AI_COLUMN_STATUS,
  and session ai_column_strict_read (default OFF).
- Foreground INSERT/UPDATE/DELETE enqueue and invalidate in the caller's
  transaction; dependency changes and primary-key moves renew request tokens.
- A durable task table and timer worker evaluate models outside publication
  transactions, cache sibling results and atomically publish a request group.
- Request-token and definition-version guards reject stale results. Retry
  deadlines and bounded retries cover model and recognized conversion failures.
- Invalid integer/overflow/JSON/string-length results affect their own columns;
  successful siblings remain cached and hidden until group publication.
- Native RETRY AI COLUMN supports ordinary predicates, unquoted STATUS and
  quoted business status fields, including transaction rollback.
- ALTER ADD discovers historical rows in batches. Vector results use native
  UPDATE/index maintenance, including rollback and ANN visibility checks.
- Strict reads check the referenced column across the native statement's
  snapshot, including UPDATE/DELETE predicates; NULL-extended rows stay NULL.

## Native verification evidence

The user built the native Release seekdb binary in WSL and reported all ten
SQL cases passing together at the implementation commit above. The result
checker verified 86 named booleans equal to 1. mysqltest also checked expected
SQL errors and HTTP mock assertions, including request counts.

Log directory reported by the user:
`/home/magic/seekdb-ai-column-run-F04hnm`.
Build directory:
`/home/magic/seekdb-ai-column-check-20261028-034933/build_release`.

| SQL case | Named booleans | Result |
| --- | ---: | --- |
| ai_column_conversion | 8 | PASS |
| ai_column_worker | 12 | PASS |
| ai_column_vector_index | 12 | PASS |
| ai_column_t1_dml | 23 | PASS |
| ai_column_backfill | 4 | PASS |
| ai_column_manual_retry | 8 | PASS |
| ai_column_retry_strict | 9 | PASS |
| ai_column_show | 5 | PASS |
| ai_column_status | expected errors/status SQL | PASS |
| ai_column_ddl | 5 | PASS |
| Total | 86 | PASS |

SQL ran against the compiled seekdb server, with deterministic local HTTP model
mocks. These results do not measure a real external model's quality or latency.
The isolated server used port 2888, 2 GiB memory and a 2 GiB log disk, and the
launcher stopped only its own process.

To reproduce from a prepared native build:

```bash
export RUSTUP_TOOLCHAIN=1.98.1
make -C build_release -j2 seekdb
AI_COLUMN_TEST_CASES="ai_column_conversion ai_column_worker ai_column_vector_index ai_column_t1_dml ai_column_backfill ai_column_manual_retry ai_column_retry_strict ai_column_show ai_column_status ai_column_ddl" \
  bash unittest/sql/run_ai_column_checks.sh
```

Bison 2.4.1 generation, Python/Bash syntax, mock HTTP responses/request counts,
a standalone C++ conversion-error classification check using repository error
constants, and whitespace checks were also performed during development.
Parser/schema unit-test sources have not been linked and executed as unit tests.

## Known limitations and evaluation scope

- AI lifecycle behavior for ALTER MODIFY/CHANGE, column renames and dropping
  dependency columns is not fully implemented or verified. The ALTER ADD case
  passing does not imply that these other operations are covered.
- Tests use fresh storage; upgrading existing system tables has not been
  validated. New metadata/task schemas are required.
- Crash/restart recovery, broad concurrency/performance and official hidden
  tests have not been executed. Durable queue code is not itself restart proof.
- Strict checks currently scan column status. No large-table performance claim
  is made. Additional view/prepared-statement/lifecycle edge cases need coverage.
- Changes shared with AI-function enhancement (AI_COMPLETE configuration and
  model helpers) require conflict review when the other work is integrated.

## Submission state

All implementation changes are pushed to the AI-column branch. The subsequent
change to this document only records the verified candidate and its limits;
it does not change the tested implementation. Evaluation must use the actual
registered repository/branch selected by the competition platform. Opening a
GitHub PR is integration preparation, not a successful platform submission.
