export const routes = {
    EMPTY: "/",
    ABOUT: "/about",
    COOKIES: "/cookies",
    PRIVACY: "/privacy",
    TOS: "/termsofservice",
    AUTH: "/auth/:first?",
    VERIFY: "/verify",
    RESET: "/reset",
    AUTH_CALLBACK: "/callback",
    DASHBOARD: "/dashboard",
    LANDING: "/:first?/:second?",
    PRODUCTS: "/products",
}

export const routeMap = {
    AUTH: {
        ROOT: "/auth",
        CALLBACK: "/auth/callback",
        FORGOT: {
            ROOT: "/auth/forgot/email",
            RESET: {
                ROOT: "/auth/forgot/reset",
                ALLOWED: "/auth/forgot/reset/allowed",
                ERROR: "/auth/forgot/reset/error",
                SUCCESS: "/auth/forgot/reset/success"
            },
            EMAIL: {
                ROOT: "/auth/forgot/email",
                SUBMITTED: "/auth/forgot/email/submitted"
            }
        },
        SIGNUP: "/auth/signup",
        VERIFY: "/auth/verify"
    },
    DASHBOARD: {
        ROOT: "/dashboard",
        HOME: "/dashboard/home",
        SETTINGS: {
            ROOT: "/dashboard/settings",
            PAYMENT: {
                ROOT: "/dashboard/setings/payment",
                BILLING: "/dashboard/setings/payment/billing",
                CANCEL: "/dashboard/settings/payment/cancel",
                CONFIRMATION: "/dashboard/setings/payment/confirmation",
                PLAN: "/dashboard/setings/payment/plan"
            },
            PROFILE: "/dashboard/settings/profile"
        }
    },
    HOMEPAGE: {
        ROOT: "/",
        ABOUT: "/about",
        PRODUCTS: "/products"
    },
    LEGAL: {
        ROOT: "/legal",
        TOS: "/legal/tos",
        COOKIES: "/legal/cookies",
        PRIVACY: "/legal/privacy"
    },
    ERROR: {
        ROOT: "/"
    }
}

export const fractalRoute = (route: string | Record<any, any>) => {
    if(typeof route === "string") {
        return route
    } else if(route.ROOT) {
        return route.ROOT
    } else {
        return routeMap.ERROR.ROOT
    }
}
