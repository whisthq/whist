"""add password_token to users table

Revision ID: fd968799518c
Revises: 9a6e00000255
Create Date: 2020-07-06 13:31:19.625693

"""
from alembic import op
import sqlalchemy as sa


# revision identifiers, used by Alembic.
revision = 'fd968799518c'
down_revision = '9a6e00000255'
branch_labels = None
depends_on = None


def upgrade():
    op.add_column("users", sa.Column("password_token", sa.String()))


def downgrade():
    op.drop_column("users", "password_token")
