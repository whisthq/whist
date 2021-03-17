"""Unit tests for the User model and its serializer."""

from app.serializers.public import UserSchema


def test_serialize(make_user):
    """Serialize an instance of the user model correctly.

    Instead of a created_at key of type str in the serializer's output dictionary, there should be
    a created_timestamp key of type int.
    """

    user = make_user()
    schema = UserSchema()
    user_dict = schema.dump(user)

    assert "created_at" not in user_dict
    assert type(user_dict["created_timestamp"]) == int
