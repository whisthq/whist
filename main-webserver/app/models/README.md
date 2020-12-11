##Models README

We leverage flask-sqlalchemy as our object-relational mapping (ORM), which helps us to easily execute SQL commands without writing raw SQL and catch inconsistencies between the server's understanding of SQL structure and the actual SQL structure at compile-time rather than run-time.

Specifically, sqlalchemy lets us represent db tables as python objects, making programmatic table modifications more closely resemble the surrounding code.

Files in this directory and its sister directory, serializers, are named after the DB schemata which they're representing in the ORM.

If you're adding a table to the DB, you should add a corresponding model class to the file for the schema that table is part of. Examples can be found in the files.

Models should exactly mimic the DB tables they're based on (down to column names, constraints, and foreign/primary keys), and one model should exist for every DB table. Serializers should use the pattern already shown in the serializers files and are a convenient tool to json-ify SQLAlchemy objects (e.g. rows).
