// State of container creation from GraphQL
export enum FractalAppState {
    PENDING = "PENDING", // is spinning up
    READY = "READY", // successfully spun up, ready to connect
    FAILURE = "FAILURE", // failed to spin up, cannot be connected to
    CANCELLED = "CANCELLED", // has been cancelled (treated as a FAILURE effectivelly)
    SPINNING_UP_NEW = "SPINNING_UP_NEW",
}

// State of task status from /status endpoint
export enum FractalTaskStatus {
    SUCCESS = "SUCCESS",
    FAILURE = "FAILURE",
    PENDING = "PENDING",
}
