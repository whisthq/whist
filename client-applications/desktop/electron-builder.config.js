// This configuration specifies the name, description, and other
// metadata regarding the Fractal application.

const { appDetails, bundleConfig, publishConfig } = require('./config/build')

// electron-builder expects CommonJS syntax
// via dependency on read-config-file
module.exports.default = {
    ...appDetails,
    ...bundleConfig,
    ...publishConfig,
}
