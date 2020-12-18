export enum FractalAppLocalState {
    INSTALLED = 1,
    NOT_INSTALLED,
    INSTALLING,
    DELETING,
}

export type FractalBanner = {
    heading: string
    subheading: string
    category: string
    background: string
    url: string
}

export type FractalApp = {
    app_id: string
    logo_url: string
    task_definition: string
    category: string
    description: string
    long_description: string
    url: string
    tos: string
    active: boolean
    localState: FractalAppLocalState
}
