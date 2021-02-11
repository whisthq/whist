export const CREATE_CONTAINER = "CREATE_CONTAINER"
export const GET_CONTAINER_INFO = "GET_CONTAINER_INFO"

export const createContainer = () => {
    return {
        type: CREATE_CONTAINER,
    }
}

export const getContainerInfo = (taskID: string) => {
    return {
        type: GET_CONTAINER_INFO,
        taskID,
    }
}
