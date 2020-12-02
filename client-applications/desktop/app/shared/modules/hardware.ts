import { varOrNull } from "shared/utils/helpers"

interface AppInterface {
    app_id: string | null
    logo_url: string | null
    task_definition?: string | null
    category: string | null
    description?: string | null
    long_description?: string | null
    url?: string | null
    tos?: string | null
    active: boolean
}

interface BannerInterface {
    heading: string | null
    subheading: string | null
    category: string | null
    background: string | null
    url: string | null
}

export class FractalApp implements AppInterface {
    constructor({
        app_id,
        logo_url,
        task_definition,
        category,
        description,
        long_description,
        url,
        tos,
        active,
    }: {
        app_id: string
        logo_url: string
        task_definition?: string
        category: string
        description?: string
        long_description?: string
        url?: string
        tos?: string
        active: boolean
    }) {
        this.app_id = app_id
        this.logo_url = logo_url
        this.task_definition = varOrNull(task_definition)
        this.category = category
        this.description = varOrNull(description)
        this.long_description = varOrNull(long_description)
        this.url = varOrNull(url)
        this.tos = varOrNull(tos)
        this.active = active
    }
}

export class FractalBanner implements BannerInterface {
    constructor({
        heading,
        subheading,
        category,
        background,
        url,
    }: {
        heading: string
        subheading: string
        category: string
        background: string
        url?: string
    }) {
        this.heading = heading
        this.subheading = subheading
        this.category = category
        this.background = background
        this.url = url
    }
}
