import * as Sentry from "@sentry/electron";

Sentry.init({
    dsn:
        "https://b280fc38171e46f1aa270b359536e9a9@o400459.ingest.sentry.io/5412323",
});

const { init } =
    process.type === "browser"
        ? require("@sentry/electron/dist/main")
        : require("@sentry/electron/dist/renderer");
