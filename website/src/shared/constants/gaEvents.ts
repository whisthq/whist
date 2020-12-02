/* eslint-disable */
export const categories = {
    // auth, joining the waitlist, etcetera
    USER: "User",
    // clicking buttons looking around the site
    POINTS: "Points",
    // interacting with Fractal by filling out the essays, referring people, contacting us, etcetera
    INTERACTION: "Interaction",
}

export const actions = {
    USER: {
        LOGIN: "Logged in to an account",
        LOGIN_TYPE: {
            LOGIN_EMAIL: "Logged in to an account with email",
            LOGIN_GOOGLE: "Logged in to an account with google",
            LOGIN_FACEBOOK: "Logged in to an account with facebook",
        },

        SIGNUP: "Created an account",
        SIGNUP_TYPE: {
            SIGNUP_EMAIL: "Created an account with email",
            SIGNUP_GOOGLE: "Created an account with google",
            SIGNUP_FACEBOOK: "Created account with facebook",
        },

        JOIN_WAITLIST: "Joind the waitlist",
        VERIFY_EMAIL: "Verified email",
        FORGOT_PASSWORD: "Forgot password",
        RESET_PASSWORD: "Reset password",

        LOGOUT: "Logged out",
        LOGOUT_WAITLIST: "Logged out from the waitlist",
    },
    POINTS: {
        CLICKED_EASTEREG: "Clicked an easter egg",
        CLICKED_EASTEREGG_LABELED: {
            CLICKED_EASTEREGG_1: "Clicked easter egg 1",
            CLICKED_EASTEREGG_2: "Clicked easter egg 2",
            CLICKED_EASTEREGG_3: "Clicked easter egg 3",
            CLICKED_EASTEREGG_4: "Clicked easter egg 4",
            CLICKED_EASTEREGG_5: "Clicked easter egg 5",
            CLICKED_EASTEREGG_6: "Clicked easter egg 6",
            CLICKED_EASTEREGG_7: "Clicked easter egg 7",
            CLICKED_EASTEREGG_8: "Clicked easter egg 8",
            CLICKED_EASTEREGG_9: "Clicked easter egg 9",
        },

        CLICKED_GOTO_AUTH_BUTTON:
            "Clicked on button guiding users to auth for points",

        // interaction

        ESSAY_PAGE: {
            FILLED_ESSAY_1: "Wrote in the first essay box",
            FILLED_ESSAY_2: "Wrote in the second essay box",
            FILLED_ESSAY_3: "Wrote in the third essay box",
            CLICKED_SUBMIT_ESSAYS: "Clicked submit key on essay page",
            CLICKED_BACK_HOME_ESSAYS:
                "Clicked on the back home button on essays",
        },

        CLICKED_REFER_FRIEND: "Clicked on refer a friend",
        REFERRED_FRIEND: "Referred friend",
        SENT_REFERALL_EMAIL: "Sent Referall Email",

        HEADER_LINKS: {
            CLICKED_SUPPORT_HEADER:
                "Clicked the support email link in the header",
            CLICKED_CAREERS_HEADER:
                "Clicked the careers email link in the header",
            CLICKED_ABOUT_HEADER: "Clicked about in the header",
            CLICKED_AUTH_HEADER: "Clicked auth in the header",
        },

        FOOTER_LINKS: {
            CLICKED_SALES_FOOTER: "Clicked on the sales link in the footer",
            CLICKED_CAREERS_FOOTER:
                "Clicked on the careers email link in the footer",
            CLICKED_SUPPORT_FOOTER:
                "Clicked the support email link in the footer",
            CLICKED_BLOG_FOOTER: "Clicked on blog link in the footer",
            CLICKED_ABOUT_FOOTER: "Clicked on about link in the footer",
            CLICKED_DISCORD_FOOTER: "Clicked on discord link in the footer",
            CLICKED_TERMS_OF_SERVICE_FOOTER:
                "Clicked on the link to the terms of service in the footer",
            CLICKED_PRIVACY_POLICY_FOOTER:
                "Clicked on the privacy policy in the footer",
        },

        FOOTER_IMG_LINKS: {
            CLICKED_TWITTER_IMG_LINK_FOOTER:
                "Clicked on the twitter link (image) in the footer",
            CLICKED_MEDIUM_IMG_LINK_FOOTER:
                "Clicked on the medium link (image) in the footer",
            CLICKED_LINKEDIN_IMG_LINK_FOOTER:
                "Clicked on the linkedin link (image) in the footer",
            CLICKED_INSTA_IMG_LINK_FOOTER:
                "Clicked on the instagram link (image) in the footer",
            CLICKED_FACEBOOK_IMG_LINK_FOOTER:
                "Clicked on the facebook link (image) in the footer",
        },

        ABOUT_US: {
            LOOKED_AT_TEAM_MEMBER:
                "Clicked or hovered over the profile of a team member",
            // potentially add the team members? clearly overkill

            CLICKED_ON_INVESTOR: "Clicked on an investor link in about",
            // potentially add which one? might be overkill
        },
    },
}

export const ga_event = (
    category: string,
    action: string,
    label: string = "",
    value: number | null = null,
    non_interaction: boolean = false
) => {
    let event: any = {
        category: category,
        action: action,
        label: label,
    }
    if (value) {
        event["value"] = value
    }
    if (non_interaction) {
        event["non_interaction"] = true
    }

    return event
}
