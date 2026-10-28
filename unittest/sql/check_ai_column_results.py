#!/usr/bin/env python3
"""Check the named SQL boolean results, beyond mysqltest's exit status."""
import argparse
from pathlib import Path

CHECKS = {
    'ai_column_worker': '''group_starts_hidden one_request_group_for_two_columns
        no_partial_visibility grouped_results_visible grouped_tasks_removed
        retry_state_committed retry_eventually_completes retry_task_removed
        terminal_failure_keeps_null terminal_failure_removes_task
        latest_request_wins late_old_result_cannot_overwrite'''.split(),
    'ai_column_t1_dml': '''result_hidden_until_t2 insert_task_eligible_immediately
        initial_worker_completed initial_value unrelated_update_keeps_result
        unrelated_update_creates_no_task dependency_update_invalidates
        dependency_update_task dependency_update_completed old_primary_key_removed
        primary_key_update_invalidates task_uses_new_primary_key
        primary_key_update_completed primary_key_update_task_drained
        transaction_sees_invalidation transaction_sees_task rollback_restores_row
        rollback_removes_task batch_insert_invalidates_two_rows
        batch_insert_creates_two_tasks delete_cleans_pending_tasks delete_removes_row
        delete_leaves_no_task'''.split(),
    'ai_column_ddl': '''ai_metadata_rows three_hidden_columns_per_ai_column
        result_columns_are_user_visible json_type_prompt_persisted alter_add_metadata'''.split(),
}


def check(case, output):
    expected = set(CHECKS.get(case, ()))
    seen, failures = set(), []
    lines = output.splitlines()
    for index, line in enumerate(lines):
        columns = line.split('\t')
        matched = expected.intersection(columns)
        if not matched:
            continue
        values = lines[index + 1].split('\t') if index + 1 < len(lines) else []
        for name in sorted(matched):
            position = columns.index(name)
            value = values[position] if position < len(values) else '<missing>'
            seen.add(name)
            if value != '1':
                failures.append(f'{name}: expected 1, got {value!r}')
            else:
                print(f'PASS {name}')
    failures.extend(f'{name}: result missing' for name in sorted(expected - seen))
    for failure in failures:
        print(f'FAIL {failure}')
    return not failures


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('case')
    parser.add_argument('log', type=Path)
    args = parser.parse_args()
    raise SystemExit(0 if check(args.case, args.log.read_text(errors='replace')) else 1)
