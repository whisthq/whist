import { FractalTaskStatus } from "shared/types/containers"
import { ChildProcess } from "child_process"

export type Task = {
    taskID?: string
    status?: FractalTaskStatus
    childProcess?: ChildProcess | undefined
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
        status: FractalTaskStatus.PENDING,
        childProcess: undefined
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
