from app.serializers.hardware import UserContainer


def get_container_user(container_name):
    container = UserContainer.query.get(container_name)

    if container:
        return str(container.user_id)

    return "None"
