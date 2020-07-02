"""add private_key to vm table

Revision ID: 9a6e00000255
Revises: 4c2d39511732
Create Date: 2020-07-02 13:32:39.800547

"""
from alembic import op
import sqlalchemy as sa


# revision identifiers, used by Alembic.
revision = '9a6e00000255'
down_revision = '4c2d39511732'
branch_labels = None
depends_on = None


def upgrade():
    op.add_column('v_ms',
        sa.Column('rsa_private_key', sa.String())
    )


def downgrade():
    op.drop_column('v_ms', 'rsa_private_key')
