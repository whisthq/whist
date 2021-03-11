/**
 * Read notion for more info. TestIDs are to encapsulate components that we want to check for existence
 * with wrapper divs. They are used for conditional rendering tests.
 *
 * The naming convention for shared is just a global string.
 * The naming convention for all page-specific things is (page_name)(-.*)*
 * Preferably, if you are checking something like a view or component, put
 * (page_name)-(view or component name)-(something descriptive...)
 */

//// shared
export const HEADER = "header"
export const FOOTER = "footer"
export const SIDE_BY_SIDE = "side-by-side"
export const NEVER = "never" // for something that is never supposed to show up

// more singletons...
// components...
// views...

//// dashboard

export const DASHBOARD_IDS = {
    PUFF: "dashboard-puff-animation",
    BACK: "dashboard-back-to-home",
    RIGHT: "dashboard-right",
    DOWNLOAD: "dashboard-download",
}

// components
export const DASHBOARD_DOWNLOAD_BOX_IDS = {
    OS_WARN: "download-box-os-non-supported-warning",
    DOWNLOAD_OS: "download-box-download-button-os",
    DOWNLOAD_AGAINST_OS: "download-box-download-button-against-os",
}

//// landing

export const LANDING_IDS = {
    TOP_VIEW: "landing-top-view",
    MID_VIEW: "landing-middle-view",
    LEADERBOARD_VIEW: "landing-leaderboard-view",
    BOT_VIEW: "landing-bottom-view",
}

// components ....
// views...

//// application

export const APPLICATION_IDS = {
    FLOW_1: "application-flow-page-1",
    FLOW_2: "application-flow-page-2",
    FLOW_3: "application-flow-page-3",
    FLOW_DONE: "application-flow-page-done",
    FLOW_BUTTON: "application-flow-continue-and-submit-button",
}

//// about

export const ABOUT_IDS = {
    TEAM: "about-team",
    INVESTORS: "about-investors",
}

// components...

//// auth

export const AUTH_IDS = {
    LOGIN: "auth-login-view",
    SIGNUP: "auth-signup-view",
    FORGOT: "auth-forgot-view",
    SWITCH: "auth-switch-mode-component",
    FORM: "auth-form",
    BUTTON: "auth-button",
    WARN: "auth-warn",
    LOADING: "auth-loading",
    EMAILTOKEN: "auth-email-token",
    USERID: "auth-userid",
    REFRESHTOKEN: "auth-refresh-token",
    ACCESSTOKEN: "auth-access-token",
    RESETTOKEN: "auth-reset-token",
    RESETURL: "auth-reset-url",
}

export const USER_AUTH_IDS = {
    EMAIL: "auth-email-field",
    PASSWORD: "auth-password-field",
    CONFIRMPASSWORD: "auth-confirm-password-field",
    NAME: "auth-name-field",
}

export const VERIFY_IDS = {
    RETRY: "verify-retry-button",
    SUCCESS: "verify-success",
    FAIL: "verify-failure",
    LOAD: "verify-loading",
    INVALID: "verify-invalid-token",
    BACK: "verify-back-to-login",
}

export const RESET_IDS = {
    LOAD: "reset-loading",
    FORM: "reset-form",
    BUTTON: "reset-button",
    FAIL: "reset-failure",
    SUCCESS: "reset-success",
}

// components...
// views...

//// auth callback
// no ids, not tested for now

//// legal
// no ids

//// payment
export const PAYMENT_IDS = {
    PAY: "payment-payment-forms",
    SUBMIT: "payment-submit-button",
    CARD_FORM: "payment-card-form",
    INFO: "payment-price-info",
    WARN: "payment-fail-warning",
    ADDCARD: "payment-add-card",
    CARDFIELD: "payment-card-field",
}

// components...

//// plan
export const PLAN_IDS = {
    INFO: "plan-info",
    NEXT: "plan-next-button",
    BOXES: "plan-boxes",
    ADDPLAN: "plan-add",
    PLANBOX: "plan-box",
}

// components...

//// profile
export const PROFILE_IDS = {
    NAME: "profile-name-form",
    PASSWORD: "profile-password-form",
    PLAN: "profile-plan-form",
    PAY: "profile-payment-form",
    OUT: "profile-signout-button",
    EDITICON: "profile-edit-icon",
    ADDNAME: "profile-add-name",
    SAVE: "profile-save",
}

// components...

//// confirmation
export const CONFIRMATION_IDS = {
    INFO: "confirmation-plan-info",
    BACK: "confirmation-back-button",
}

//// cancel
export const CANCEL_IDS = {
    WARN: "cancel-warning",
    SUBMIT: "cancel-submit-button",
    BACK: "cancel-back-button",
    FEEDBACK: "cancel-feedback-form",
    INFO: "cancel-info",
}

export const CANCELED_IDS = {
    INFO: "canceled-info",
    BACK: "canceled-back-button",
    ADD: "canceled-new-plan-button",
}

// e2e testing ids
export const E2E_HOMEPAGE_IDS = {
    SIGNIN: "signin",
}

export const E2E_AUTH_IDS = {
    EMAIL: "emailInput",
    NAME: "nameInput",
    PASSWORD: "passwordInput",
    CONFIRMPASSWORD: "confirmPasswordInput",
    LOGINSWITCH: "loginSwitch",
    SIGNUPSWITCH: "signupSwitch",
    FORGOTSWITCH: "forgotSwitch",
}

export const E2E_DASHBOARD_IDS = {
    HOME: "dashboardHome",
    SETTINGS: "dashboardSettings",
    SIGNOUT: "dashboardSignout",
    EDITNAME: "editName",
    NAMEFIELD: "nameField",
    EDITPASSWORD: "editPassword",
    PASSWORDFIELD: "passwordField",
    CURRENTPASSWORD: "currentPassword",
    SETTINGSPROFILE: "profile",
    SETTINGSPAYMENT: "payment",
    EDITCARD: "editCard",
    EDITPLAN: "editPlan",
    ADDPLAN: "addPlan",
    CANCELPLAN: "cancelPlan",
}

export const LOCAL_URL = "http://localhost:3000"
export const EMAIL_API_KEY =
    "ade68d47a8ba39c57a8e8358e5e86d6d99a04cf8aeebf9c11c08f851f2fa438f"
