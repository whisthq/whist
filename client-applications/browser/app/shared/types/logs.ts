export type Logger = {
    componentName: string
    fileName: string
}

export class AvailableLoggers {
    static LOADING: Logger = {
        componentName: "Loading Page",
        fileName: "logs/renderer.log",
    }

    static LAUNCHER: Logger = {
        componentName: "Launcher Page",
        fileName: "logs/renderer.log",
    }

    static LOGIN: Logger = {
        componentName: "Login Page",
        fileName: "logs/renderer.log",
    }

    static UPDATE: Logger = {
        componentName: "Update Page",
        fileName: "logs/renderer.log",
    }
}
