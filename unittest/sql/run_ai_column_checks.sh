#!/usr/bin/env bash
# Run the initial AI-column SQL cases against a fresh isolated server.
set -euo pipefail

TASK_ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
cd "$TASK_ROOT"
CLIENT=${AI_COLUMN_MYSQL_CLIENT:-/usr/bin/mysql}
MYSQLTEST="$TASK_ROOT/deps/3rd/u01/obclient/bin/mysqltest"
BINARY="$TASK_ROOT/build_release/src/observer/seekdb"
PORT=${AI_COLUMN_TEST_PORT:-2888}
for executable in "$CLIENT" "$MYSQLTEST" "$BINARY"; do
  if [ ! -x "$executable" ]; then
    echo "Missing executable: $executable" >&2
    exit 1
  fi
done
if ss -ltn "sport = :$PORT" | grep -q LISTEN; then
  echo "Port $PORT is occupied; no server was started." >&2
  exit 1
fi

RUN_DIR=$(mktemp -d "$HOME/seekdb-ai-column-run-XXXXXX")
printf '%s\n' "$RUN_DIR" > "$HOME/seekdb-ai-column-last-run.txt"
echo "AI-COL-038 logs: $RUN_DIR"
export LD_LIBRARY_PATH="$TASK_ROOT/deps/3rd/usr/local/oceanbase/deps/devel/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

# ObServerResource::SERVER_MIN_LOG_DISK_SIZE is 2 GiB. Each attempt needs
# fresh storage because a failed initial bootstrap may leave no checkpoint.
"$BINARY" --nodaemon --base-dir "$RUN_DIR" --port "$PORT" \
  --parameter memory_budget=2G --parameter cpu_count=2 \
  --parameter datafile_size=1G --parameter datafile_maxsize=2G \
  --parameter log_disk_size=2G --log-level INFO \
  > "$RUN_DIR/startup.log" 2>&1 &
AI_COLUMN_PID=$!
cleanup() {
  kill "$AI_COLUMN_PID" 2>/dev/null || true
  wait "$AI_COLUMN_PID" 2>/dev/null || true
}
trap cleanup EXIT

READY=0
for ((attempt=1; attempt<=120; ++attempt)); do
  if "$CLIENT" --no-defaults --protocol=TCP -h127.0.0.1 -P"$PORT" \
      -uroot --connect-timeout=2 -N -B -e 'SELECT 1' \
      > "$RUN_DIR/client.log" 2>&1; then
    READY=1
    break
  fi
  if ! kill -0 "$AI_COLUMN_PID" 2>/dev/null; then
    break
  fi
  sleep 1
done
if [ "$READY" != 1 ]; then
  echo 'AI-COL-039 FAIL: server did not become ready'
  for log in "$RUN_DIR/startup.log" "$RUN_DIR/client.log" "$RUN_DIR/log/seekdb.log"; do
    if [ -f "$log" ]; then
      echo "--- $log"
      tail -n 60 "$log"
    fi
  done
  exit 1
fi

echo 'AI-COL-039 server ready'
cd "$TASK_ROOT/tools/deploy"
read -r -a TEST_CASES <<< "${AI_COLUMN_TEST_CASES:-ai_column_ddl ai_column_status ai_column_t1_dml ai_column_worker ai_column_backfill}"
for test_case in "${TEST_CASES[@]}"; do
  echo "AI-COL-040 running $test_case"
  if "$MYSQLTEST" --no-defaults --protocol=TCP --host=127.0.0.1 \
      --port="$PORT" --user=root --database=oceanbase --logdir="$RUN_DIR" \
      --test-file="$TASK_ROOT/tools/deploy/mysql_test/test_suite/ai_column/t/$test_case.test" \
      > "$RUN_DIR/$test_case.log" 2>&1; then
    if ! python3 "$TASK_ROOT/unittest/sql/check_ai_column_results.py" "$test_case" "$RUN_DIR/$test_case.log"; then
      tail -n 100 "$RUN_DIR/$test_case.log"
      grep -n -E -- 'AI column .*failed' "$RUN_DIR/log/seekdb.log" | tail -n 20 | cut -c 1-1600 || true
      exit 1
    fi
    echo "PASS: $test_case"
  else
    echo "FAIL: $test_case"
    tail -n 100 "$RUN_DIR/$test_case.log"
    echo "--- Server error context"
    grep -n -E -- 'AI column .*failed' \
      "$RUN_DIR/log/seekdb.log" | tail -n 20 | cut -c 1-1600 || true
    exit 1
  fi
done
echo "AI-COL-041 selected SQL cases completed; logs: $RUN_DIR"
