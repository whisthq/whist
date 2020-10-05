"""Custom Marshmallow fields."""

from marshmallow import ValidationError
from marshmallow.fields import Field


class Bytes(Field):
    """A field containing binary data."""

    def _serialize(self, value, attr, obj, **kwargs):
        """Print the hexadecimal representation of the binary data."""

        return value.hex()

    def _deserialize(self, value, attr, obj, **kwargs):
        """Disallow deserialization."""

        raise ValidationError
