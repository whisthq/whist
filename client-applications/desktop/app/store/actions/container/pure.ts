import { Container, Task, HostService } from "store/reducers/container/default"

export const UPDATE_CONTAINER = "UPDATE_CONTAINER"
export const UPDATE_TASK = "UPDATE_TASK"
export const UPDATE_HOST_SERVICE = "UPDATE_HOST_SERVICE"

export const updateContainer = (body: Container) => {
    return {
        type: UPDATE_CONTAINER,
        body,
    }
}

export const updateTask = (body: Task) => {
    return {
        type: UPDATE_TASK,
        body,
    }
}

export const updateHostService = (body: HostService) => {
    return {
        type: UPDATE_HOST_SERVICE,
        body,
    }
}
