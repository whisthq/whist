#!/usr/bin/env python
from pathlib import Path

IGNORE_HEAD = {
    'alter extension "pg_stat_statements" update to'
}

IGNORE_COMPLETE = {
    'alter table "hdb_catalog"."event_invocation_logs" alter column "id" set default public.gen_random_uuid();',
    'alter table "hdb_catalog"."event_log" alter column "id" set default public.gen_random_uuid();',
    'alter table "hdb_catalog"."hdb_action_log" alter column "id" set default public.gen_random_uuid();',
    'alter table "hdb_catalog"."hdb_cron_event_invocation_logs" alter column "id" set default public.gen_random_uuid();',
    'alter table "hdb_catalog"."hdb_cron_events" alter column "id" set default public.gen_random_uuid();',
    'alter table "hdb_catalog"."hdb_scheduled_event_invocation_logs" alter column "id" set default public.gen_random_uuid();',
    'alter table "hdb_catalog"."hdb_scheduled_events" alter column "id" set default public.gen_random_uuid();',
    'alter table "hdb_catalog"."hdb_version" alter column "hasura_uuid" set default public.gen_random_uuid();'
}


def is_ignored_head(ln: str):
    for head in IGNORE_HEAD:
        if ln.startswith(head):
            return True
    return False


def is_ignored_complete(ln: str):
    return ln in IGNORE_COMPLETE

def ignore_filter(lines):
    no_empty = (ln for ln in lines if ln)
    ignored_head = (ln for ln in no_empty if is_ignored_head(ln))
    ignored_complete = (ln for ln in ignored_head if is_ignored_complete(ln))
    return list(ignored_complete)
