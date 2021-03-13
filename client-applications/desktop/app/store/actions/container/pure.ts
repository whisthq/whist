import { Container, Task } from "store/reducers/container/default"

export const UPDATE_CONTAINER = "UPDATE_CONTAINER"
export const UPDATE_TASK = "UPDATE_TASK"

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
