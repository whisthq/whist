#!/usr/bin/env python
import os
import subprocess
from ignore import ignore_filter

PATH_SCHEMA_MERGING = os.environ["PATH_SCHEMA_MERGING"]
CURRENT = os.environ["CURRENT"]
MERGING = os.environ["MERGING"]



# run migra to diff schema A and schema B
# decode result of subprocess
def diff(db_uri_a: str, db_uri_b: str):
    completed = subprocess.run(["migra", db_uri_a, db_uri_b, "--unsafe"],
                               stdout=subprocess.PIPE)
    diff = completed.stdout.decode("utf-8")
    return diff.splitlines()


def write_sql(sql_file_path, db_uri):
    subprocess.run(["psql", "-f", "db.sql"],
                   stdout=subprocess.PIPE)


def main():
    result_diff = ignore_filter(diff(CURRENT, MERGING))
    if not result_diff:
        print("Schema is unchanged.")
        return
    else:
        print("Merging branch requires the following schema changes:")
        print("\n")
        for ln in ignore_filter(diff(CURRENT, MERGING)):
            print("  " + ln)


if __name__ == "__main__":
    print("\n")
    main()
    print("\n")
