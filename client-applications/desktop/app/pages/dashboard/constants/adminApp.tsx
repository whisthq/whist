import FractalImg from "assets/images/fractal.svg"

export const adminApp = {
    app_id: "Test App",
    logo_url: FractalImg,
    category: "Test",
    description: "Test app for Fractal admins",
    long_description:
        "You can use the admin app to test if you are a Fractal admin (i.e. your email ends with @tryfractal). Go to settings and where the admin settings are set a task ARN, webserver (dev | local | staging | prod | a url), and region (us-east-1 | us-west-1 | ca-central-1). In any field you can enter reset to reset it to null. If you try to launch without setting then it will not work since it will be null (or your previous settings if you change one). Cluster is null | string.",
    url: "tryfractal.com",
    tos: "https://www.tryfractal.com/termsofservice",
    active: true, // not used yet
}
