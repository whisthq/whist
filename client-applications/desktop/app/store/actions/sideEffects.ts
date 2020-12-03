export const REMEMBER_ME_LOGIN = "REMEMBER_ME_LOGIN"
export const VALIDATE_ACCESS_TOKEN = "VALIDATE_ACCESS_TOKEN"

export const CREATE_CONTAINER = "CREATE_CONTAINER"
export const CREATE_TEST_CONTAINER = "CREATE_TEST_CONTAINER"

export const SUBMIT_FEEDBACK = "SUBMIT_FEEDBACK"

export function rememberMeLogin(username: string) {
    return {
        type: REMEMBER_ME_LOGIN,
        username,
    }
}

export function createContainer(app: string) {
    return {
        type: CREATE_CONTAINER,
        app,
    }
}
export function createTestContainer(app: string) {
    return {
        type: CREATE_TEST_CONTAINER,
        app,
    }
}

export function validateAccessToken(accessToken: string) {
    return {
        type: VALIDATE_ACCESS_TOKEN,
        accessToken,
    }
}

export function submitFeedback(feedback: string, feedbackType: string) {
    return {
        type: SUBMIT_FEEDBACK,
        feedback,
        feedbackType,
    }
}
