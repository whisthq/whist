#!/usr/bin/env python

TITLE_ERROR = "An error occured while comparing the database schema."

TITLE_UNCHANGED = "Schema is unchanged, no database migration needed."

TITLE_SAFE = "There's some changes to be made to the schema!"

TITLE_UNSAFE = "This PR introduces destructive changes to the schema!"

TITLE_INVALID = "This PR will not migrate successfully."

BODY_ERROR = "The diff tool `migra` exited with an error."

BODY_UNCHANGED = "Carry on!"

BODY_SAFE = "The SQL commands below will perform the migration."

BODY_UNSAFE = (
    "The schema diff produced some unsafe commands, which can be"
    + " dangerous to run on the database."
    + "\nRemember these will be run automatically upon merge, so be sure"
    + " to review these changes extra carefully."
    + "\nThe SQL commands below will perform the migration."
)

BODY_INVALID = (
    "The schema diff did not pass the migration test. The following"
    + " SQL commands will not be applied properly to the database."
)
