import { FractalTaskStatus } from "shared/types/containers"
import { AWSRegion } from "shared/types/aws"

export type Task = {
    taskID?: string
    status?: FractalTaskStatus
    shouldLaunchProtocol?: boolean
    protocolKillSignal?: number
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
    region?: AWSRegion | undefined
}

export type ContainerState = {
    task: Task
    container: Container
}

export const DEFAULT: ContainerState = {
    task: {
        taskID: "",
        status: FractalTaskStatus.PENDING,
        shouldLaunchProtocol: false,
        protocolKillSignal: 0,
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
