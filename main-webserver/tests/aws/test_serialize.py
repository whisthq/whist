"""Custom serializer tests."""

from app.serializers.hardware import UserContainerSchema


def test_serialize(container):
    schema = UserContainerSchema()

    with container() as c:
        data = schema.dump(c)

        assert "secret_key" in data
        assert len(data["secret_key"]) == 16
        assert isinstance(data["secret_key"], str)
