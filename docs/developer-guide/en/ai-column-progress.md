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
  The previous uncommitted runtime work was lost when the execution workspace
  was cleaned; it is not part of this remote checkpoint.
- Durable queue, background worker, recovery, bounded retries and version guards.
- Atomic publication of every affected AI column in a request group.
- SHOW AI COLUMNS, scoped RETRY and strict-read semantics.
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

## Follow-up checkpoint: status and completion instructions

- Completion format instructions now use the provider's system-message channel;
  the original user prompt is preserved. The private option is filtered out of
  provider configuration in scalar and batch completion paths.
- Added native `AI_COLUMN_STATUS(column)` binding to the hidden state column,
  with rejection of non-AI arguments and NULL propagation for outer joins.
- Added a status mysqltest case requiring no model endpoint.
- These follow-up changes have passed `git diff --check` only. Native build and
  runtime verification are pending on the user's isolated WSL worktree.

## Follow-up checkpoint: strict reads

- Added the session `ai_column_strict_read` variable, default OFF, with plan-cache
  influence and session serialization. Its metadata/factory files were generated
  from the native JSON definition; existing initializer ordering is preserved.
- Strict SELECT resolution guards AI references in output, predicates, grouping,
  sorting and joins. Status inspection and ordinary-column reads remain available.
- Added native error 11120 with table/column identifiers, and preserved result
  metadata for JSON and VECTOR. Missing outer-join rows propagate NULL.
- Extended the status mysqltest with strict-read cases. The generator runs and
  `git diff --check` passed; compilation and SQL execution remain pending.

## WSL build and startup follow-up

- The user reported a complete native Release build of commit `11aaf265`:
  `[100%] Built target seekdb`. SQL runtime verification is still pending.
- The first isolated launch used a 1 GiB log disk and failed with
  `OB_RESOURCE_UNIT_VALUE_INVALID` (-4736). The native resource minimum is
  2 GiB (`ObServerResource::SERVER_MIN_LOG_DISK_SIZE`).
- `bash unittest/sql/run_ai_column_checks.sh` now creates fresh storage with
  a 2 GiB log disk, captures connection and server diagnostics, runs the initial
  DDL/status cases, and shuts down only the process that it started.
- The launcher passed Bash syntax and diff checks; it has not run against a
  seekdb binary in this workspace. Async DML, worker and RETRY remain incomplete.

## Runtime regression and durable queue checkpoint

- The user reported a successful incremental build and both initial SQL cases
  passing after `721ea3af`: `ai_column_ddl` and `ai_column_status`, using fresh
  instance `/home/magic/seekdb-ai-column-run-7wm5nO`. This validates the tested
  DDL/status/strict-read paths, not the unfinished async worker.
- Added native system table `__all_ai_column_task` (1135) with row/column key,
  request grouping and token, definition version, retry deadline/counters,
  cached result and error code. Native schema generation and diff checks pass.
- Enqueue, transaction rollback integration and background execution are still
  pending; the new queue definition alone does not schedule model requests.

## Background worker checkpoint (not yet runtime verified)

- The user reported the DDL/status cases passing again after `965370b7`,
  including INSERT through the native transactional queue hook.
- Added a dedicated observer timer, durable group selection, native AI calls
  outside publication transactions, cached results, retry deadlines, request
  token/definition guards, and atomic grouped publication through native UPDATE.
- The launcher now includes the existing T1 and worker cases, runs from the
  required mock-script working directory, and prints server error context.
- Bash syntax and diff checks pass; native compilation and all worker runtime
  cases remain pending. RETRY syntax, SHOW AI COLUMNS, ALTER backfill and recovery
  discovery remain incomplete. A conversion failure currently fails the entire
  publication group; per-column conversion validation still needs implementation.

## Confirmed worker regression results

After `e2ad0d6f`, the user reported a successful native build and the worker
case completing in `/home/magic/seekdb-ai-column-run-QgKj7d`. The log was then
checked separately: all 12 named boolean results were present and equal to 1.
This covers grouped visibility/publication, retry state and completion,
terminal NULL/task cleanup, and latest-request/stale-result guards.
T1 and vector-index results still need the same strict result verification.
The result checker now includes the vector suite's 12 boolean assertions.

## T1/vector verification and ALTER backfill checkpoint

The user reported all 23 T1 and 12 vector-index boolean assertions passing in
`/home/magic/seekdb-ai-column-run-VKo78h`. Together with the worker checks,
47 named boolean assertions have passed on the tested implementation.

Added batched historical-row discovery for ALTER ADD: NULL original request
tokens are renewed with native UPDATE, retaining foreground transactional
enqueue while allowing internal hidden-column access. An ALTER backfill case
checks two preexisting rows, two added AI columns, drained tasks and a later
INSERT. Bash/Python/diff checks pass; new C++ build and runtime test pending.
Manual RETRY, column-wide strict-read semantics, SHOW AI COLUMNS and other
previously documented limitations remain incomplete.


## Confirmed ALTER backfill and manual retry checkpoint

The user rebuilt commit `8397d9de` and reported all four backfill assertions
passing in `/home/magic/seekdb-ai-column-run-cQkQF2`: historical results, task
drain, later INSERT and source-row preservation. Worker, T1, vector and backfill
now have 51 passing named boolean assertions across their reported runs.

Manual RETRY is lowered by the native grammar to a transactional UPDATE. It
invalidates only the named AI column and renews its request token, retaining
transactional queue insertion and stale-result protection. Retry predicates
map unquoted STATUS to the selected column's state; quoted business columns
keep their normal meaning. Eight new SQL assertions cover selection, rollback,
failed-state selection, quoted STATUS and successful regeneration. These new
C++ paths and SQL assertions still require a native build/runtime run.
Bison 2.4.1 regenerated the grammar with `-Werror` without conflicts; Bash
syntax, Python compilation and whitespace checks passed.
Column-wide strict reads, SHOW AI COLUMNS, lifecycle edge cases and per-column
conversion failures remain outstanding. No merge or competition submission
has been performed.


## RETRY dispatch follow-up

The user successfully built the RETRY implementation, but its first SQL retry
was rejected as an explicit AI-result update. The statement-level
`check_question_mark` macro overwrites `ParseNode::value_` with parameter count,
erasing the retry marker. The marker now lives on the assignment node instead;
the parser regression checks both a zero statement parameter count and the
preserved assignment marker. Native runtime verification remains pending.
Incremental CMake configuration now tracks parser grammar/lexer/enum inputs.


## Confirmed RETRY and strict-read/SHOW implementation checkpoint

The user rebuilt `7599e702` and all eight manual retry assertions passed in
`/home/magic/seekdb-ai-column-run-loDDNB`. Worker, T1, vector, backfill and
manual retry now total 59 passing named assertions across reported runs.

Strict-read guards now use a native uncorrelated aggregate over the referenced
column's STATUS in the same statement and transaction snapshot. A READY row
cannot bypass unfinished rows elsewhere in its column. NULL-extended outer
join rows retain NULL state. UPDATE/DELETE predicates also receive guards.

SHOW AI COLUMNS reuses native SHOW table resolution and checks table visibility,
then exposes Field, Expression, Sync_mode, Max_retries and Definition_version.
LIKE and WHERE use the normal SHOW condition resolver. Mixed retry/ALTER
actions are rejected before native UPDATE execution. Added five SHOW assertions
and nine strict-suite assertions, including the default session mode.
Bison 2.4.1 generation with -Werror, Python compilation, Bash syntax and diff
checks passed; native build and these new runtime cases remain pending.
Lifecycle changes and per-column result conversion handling remain outstanding.


## Confirmed column-wide strict reads and SHOW filter follow-up

The user rebuilt `8987dc5e` and all nine strict-read/retry assertions passed in
`/home/magic/seekdb-ai-column-run-e9Losr`, including expected error checks for
READY-row reads while another row is unfinished and UPDATE/DELETE predicates.
Together with previous worker/T1/vector/backfill/manual checks, 68 named
assertions have passed across reported runs.

SHOW returned the expected AI name, async mode and retry count, but its WHERE
filter could not bind Field: display aliases existed only in the outer SELECT.
All display aliases now live in the derived source, where both native SHOW
WHERE and LIKE resolution bind them. Existing SHOW regression covers both
filters. Whitespace validation passed; native build/runtime rerun pending.


## Unified native regression and isolated conversion failures

The user rebuilt `a17a9ef6` and all nine SQL cases passed in
`/home/magic/seekdb-ai-column-run-l3JBcU`: SHOW, status, manual retry, T1 DML,
worker, vector index, backfill, DDL and strict retry/read. The strict result
checker verified all 78 named booleans in this single run; status negative
checks also passed through mysqltest.

Conversion failures now isolate the invalid result with native UPDATE probes
that always roll back, preserving successful sibling caches. Only the bad
column consumes retries; cached siblings stay hidden until the final atomic
group UPDATE. Terminal bad results publish NULL/FAILED beside READY siblings.
Only recognized value/conversion errors enter this path; schema and transient
errors are not misclassified as bad model output. Added eight conversion SQL
assertions covering invalid integers, overflow, JSON, length and cached retries,
plus an HTTP request-count assertion.

The mock's actual HTTP responses/request counts, a standalone C++ check using
repository error constants, Python syntax and diff checks passed. Native
compilation, conversion SQL and worker/vector regressions remain pending for
this new change. Column-definition lifecycle edge cases remain outstanding.
