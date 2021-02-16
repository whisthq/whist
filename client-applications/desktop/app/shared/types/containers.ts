// State of container creation from GraphQL
export enum FractalAppState {
    PENDING = "PENDING", // container being assigned
    READY = "READY", // successfully spun up, ready to connect
    FAILURE = "FAILURE", // failed to spin up, cannot be connected to
    CANCELLED = "CANCELLED", // has been cancelled (treated as a FAILURE effectivelly)
    SPINNING_UP_NEW = "SPINNING_UP_NEW", // new container being started
}

// State of task status from /status endpoint
export enum FractalTaskStatus {
    SUCCESS = "SUCCESS",
    FAILURE = "FAILURE",
    PENDING = "PENDING",
}
