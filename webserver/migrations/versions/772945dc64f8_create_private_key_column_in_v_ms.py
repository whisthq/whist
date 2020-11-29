"""create private_key column in v_ms

Revision ID: 772945dc64f8
Revises: 4c2d39511732
Create Date: 2020-07-15 09:51:34.059864

"""
from alembic import op
import sqlalchemy as sa


# revision identifiers, used by Alembic.
revision = "772945dc64f8"
down_revision = None
branch_labels = None
depends_on = None


def upgrade():
    op.add_column("v_ms", sa.Column("private_key", sa.String()))


def downgrade():
    op.drop_column("v_ms", "private_key")
