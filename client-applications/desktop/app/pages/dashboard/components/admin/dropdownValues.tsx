// here are some preset options for the dropdown
export enum FractalRegions {
    US_EAST_1 = "us-east-1",
    US_WEST_1 = "us-west-1",
    CA_CENTRAL_1 = "ca-central-1",
    RESET = "reset",
}

export enum FractalWebservers {
    LOCAL = "local",
    DEV = "dev",
    STAGING = "staging",
    PROD = "prod",
    RESET = "reset",
}

export enum FractalTaskDefs {
    // browsers
    BRAVE = "fractal-browsers-brave",
    CHROME = "fractal-browsers-chrome",
    FIREFOX = "fractal-browsers-firefox",
    SIDEKICK = "fractal-browsers-sidekick",
    // creative
    BLENDER = "fractal-creative-blender",
    BLOCKBENCH = "fractal-creative-blockbench",
    FIGMA = "fractal-creative-figma",
    GIMP = "fractal-creative-gimp",
    LIGHTWORKS = "fractal-creative-lightworks",
    TEXTURELAB = "fractal-creative-texturelab",
    // productivity
    DISCORD = "fractal-productivity-discord",
    NOTION = "fractal-productivity-notion",
    SLACK = "fractal-productivity-slack",
    RESET = "reset",
}

export enum FractalClusters {
    // no cluster defaults
    RESET = "reset",
}
