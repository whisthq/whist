import FractalImg from "assets/images/fractal.svg"
import { FractalApp } from "shared/types/ui"
/* eslint-disable @typescript-eslint/camelcase */

export const adminApp: FractalApp = {
    app_id: "Test App",
    logo_url: FractalImg,
    category: "Test",
    description: "Test app for Fractal admins",
    long_description:
        "You can use the admin app to test if you are a Fractal admin. Go to settings and where the admin settings are set a task ARN, webserver, cluster (optional), and an AWS region. In any field you can enter reset to reset it to the defaults (respectively, fractal-browsers-chrome, dev, null, us-east-1).",
    url: "tryfractal.com",
    tos: "https://www.tryfractal.com/termsofservice",
    active: true, // not used yet
}

export default adminApp
