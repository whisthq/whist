export const LOGIN_USER = "LOGIN_USER"
export const GOOGLE_LOGIN = "GOOGLE_LOGIN"
export const REMEMBER_ME_LOGIN = "REMEMBER_ME_LOGIN"

export const CREATE_CONTAINER = "CREATE_CONTAINER"
export const CREATE_TEST_CONTAINER = "CREATE_TEST_CONTAINER"

export const SUBMIT_FEEDBACK = "SUBMIT_FEEDBACK"

export function loginUser(
    username: string,
    password: string,
    rememberMe: boolean
) {
    return {
        type: LOGIN_USER,
        username,
        password,
        rememberMe,
    }
}

export function googleLogin(code: any, rememberMe: boolean) {
    return {
        type: GOOGLE_LOGIN,
        code,
        rememberMe,
    }
}

export function rememberMeLogin(username: string) {
    return {
        type: REMEMBER_ME_LOGIN,
        username,
    }
}

export function createContainer(app: string, url: string) {
    return {
        type: CREATE_CONTAINER,
        app,
        url,
    }
}

export function createTestContainer() {
    return {
        type: CREATE_TEST_CONTAINER,
    }
}

export function submitFeedback(feedback: string, feedback_type: string) {
    return {
        type: SUBMIT_FEEDBACK,
        feedback,
        feedback_type,
    }
}
