"""create test table

Revision ID: 4c2d39511732
Revises:
Create Date: 2020-06-25 12:54:20.865617

"""
from alembic import op
import sqlalchemy as sa


# revision identifiers, used by Alembic.
revision = "4c2d39511732"
down_revision = None
branch_labels = None
depends_on = None


def upgrade():
    op.create_table(
        "test",
        sa.Column("id", sa.Integer, primary_key=True),
        sa.Column("name", sa.String(50)),
        sa.Column("description", sa.Unicode(200)),
    )


def downgrade():
    op.drop_table("test")
