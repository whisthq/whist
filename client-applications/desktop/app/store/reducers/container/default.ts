import { FractalTaskStatus } from "shared/types/containers"

export type Task = {
    taskID?: string
    running?: boolean // True if container is creating or currently being streamed, false otherwise
    status?: FractalTaskStatus
}

export type Container = {
    publicIP?: string
    containerID?: string
    cluster?: string
    port32262?: string
    port32263?: string
    port32273?: string
    location?: string
    secretKey?: string
}

export type ContainerState = {
    task: Task
    container: Container
}

export const DEFAULT: ContainerState = {
    task: {
        taskID: "",
        running: false,
        status: FractalTaskStatus.PENDING,
    },
    container: {
        publicIP: "",
        containerID: "",
        cluster: "",
        port32262: "",
        port32263: "",
        port32273: "",
        location: "",
        secretKey: "",
    },
}
