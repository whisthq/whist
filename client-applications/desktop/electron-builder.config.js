// This configuration specifies the name, description, and other
// metadata regarding the Fractal application.
const appDetails = {
    // TODO: Update this appId to something more sensible.
    appId: "com.fractalcomputers.fractal",
    copyright: "Copyright © Fractal Computers, Inc.",
    productName: "FractalLauncher",
}

// This configuration controls how the application is bundled,
// including OS-specific details, icons, and ASAR packing.
const bundleConfig = {
    afterSign: "build/afterSign.js",

    artifactName: "Fractal.${ext}",
    
    asar: true,

    directories: {
        buildResources: "build",
        output: "release",
    },

    // The files we must wrap into our packaged application.
    files: ["build/**/*", "node_modules/", "package.json"],
    // We cannot bundle the protocol binaries -- they must remain separate.
    extraFiles: ["protocol-build/"],

    // This registers the fractal:// URL protocol in the bundle installer.
    protocols: {
        name: "fractal-protocol",
        schemes: ["fractal"],
    },

    mac: {
        category: "public.app-category.productivity",
        darkModeSupport: true,
        entitlements: "public/entitlements.mac.plist",
        entitlementsInherit: "public/entitlements.mac.plist",
        extendInfo: {
            NSHighResolutionCapable: true,
        },
        extraFiles: [
            // This allows the protocol and client app to share a dock
            // icon, by moving the former to the bundle's macOS folder.
            {
                from: "protocol-build/client/",
                to: "MacOS/",
            },
        ],
        gatekeeperAssess: false,
        hardenedRuntime: true,
        icon: "public/icon.png",
        minimumSystemVersion: "10.13.0",
        target: ["dmg"],
        type: "distribution",
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
        icon: "public/icon.png",
        sign: false,
    },

    win: {
        icon: "public/icon.ico",
        target: ["nsis"],
    },
}

// This configuration specifies to where and how we publish
// our bundled application.
const publishConfig = {
    // Configuration for publishing to AWS S3.
    publish: {
        // TODO: Determine bucket from an environment variable to be set by
        // the publishing script.
        bucket: "PLACEHOLDER",
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
