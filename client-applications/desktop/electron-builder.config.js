// This configuration specifies the name, description, and other
// metadata regarding the Fractal application.

const { iconName, appEnvironment } = require("./config/build")

const appDetails = {
    appId:
        "com.fractalcomputers.fractal" +
        (appEnvironment === "prod" ? "" : `-${appEnvironment}`), // Standard Apple appId format: <domain-extension>.<domain>.<company-name>
    copyright: "Copyright © Fractal Computers, Inc.",
    productName:
        "Fractal" + (appEnvironment === "prod" ? "" : ` (${appEnvironment})`),
}

// This configuration controls how the application is bundled,
// including OS-specific details, icons, and ASAR packing.
const bundleConfig = {
    afterSign: "scripts/afterSign.js",

    // We need to disable an eslint error here because this is electron-builder-specific syntax
    // (https://www.electron.build/configuration/configuration#artifact-file-name-template)
    artifactName: "Fractal.${ext}", // eslint-disable-line no-template-curly-in-string

    asar: true,

    directories: {
        buildResources: "build",
        output: "release",
    },

    // The files we must wrap into our packaged application.
    files: ["build/", "node_modules/", "package.json"],
    // We cannot bundle the protocol binaries -- they must remain separate.
    // This registers the fractal:// URL protocol in the bundle installer.
    protocols: {
        name: "fractal-protocol",
        schemes: ["fractal"],
    },

    extraFiles: ["loading/"],

    extraResources: ["env_overrides.json"],

    mac: {
        category: "public.app-category.productivity",
        darkModeSupport: true,
        // This is recommended to be in build/. I have placed it in public/, from where it will be moved to build/.
        entitlements: "build/entitlements.mac.plist",
        entitlementsInherit: "build/entitlements.mac.plist",
        extendInfo: {
            NSHighResolutionCapable: true,
        },
        gatekeeperAssess: false,
        hardenedRuntime: true,
        icon: `build/${iconName}.png`,
        minimumSystemVersion: "10.15.0",
        // For auto-update to work, "zip" format must be present in addition to .dmg
        target: ["dmg", "zip"],
        type: "distribution",
        extraFiles: [
            {
                from: "protocol-build/client",
                to: "MacOS/",
            },
        ],
    },

    // This controls the positions of the Fractal application and
    // the Applications folder in the macOS DMG installation menu.
    dmg: {
        contents: [
            {
                x: 130,
                y: 220,
            },
            {
                path: "/Applications",
                type: "link",
                x: 410,
                y: 220,
            },
        ],
        icon: `build/${iconName}.png`,
        sign: false,
    },

    win: {
        icon: `build/${iconName}.ico`,
        target: ["nsis"],
        extraFiles: ["protocol-build"],
    },
    linux: {
        icon: `build/${iconName}.png`,
        target: ["AppImage", "pacman", "deb"],
        extraFiles: ["protocol-build"],
    },
}

// This configuration specifies to where and how we publish
// our bundled application.
const publishConfig = {
    // Configuration for publishing to AWS S3.
    publish: {
        bucket: process.env.S3_BUCKET ? process.env.S3_BUCKET : "PLACEHOLDER",
        provider: "s3",
        region: "us-east-1",
    },
}

// electron-builder expects CommonJS syntax
// via dependency on read-config-file
module.exports.default = {
    ...appDetails,
    ...bundleConfig,
    ...publishConfig,
}
