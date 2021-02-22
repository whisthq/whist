module.exports = {
    launch: {
        headless: process.env.HEADLESS !== "false",
        timeout: 30000,
    },
    server: {
        command: "npm start",
        port: 3000,
        launchTimeout: 120000,
    },
}
