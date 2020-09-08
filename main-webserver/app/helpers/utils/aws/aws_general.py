from app.serializers.hardware import UserContainer

def getContainerUser(container_name):
    container = UserContainer.query.get(container_name)

    if container:
        return str(container.user_id)

    return "None"