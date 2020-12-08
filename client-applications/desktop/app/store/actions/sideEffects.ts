export const REMEMBER_ME_LOGIN = "REMEMBER_ME_LOGIN"
export const VALIDATE_ACCESS_TOKEN = "VALIDATE_ACCESS_TOKEN"

export const CREATE_CONTAINER = "CREATE_CONTAINER"

export const SUBMIT_FEEDBACK = "SUBMIT_FEEDBACK"

export const DELETE_CONTAINER = "DELETE_CONTAINER"

export function rememberMeLogin(username: string) {
    return {
        type: REMEMBER_ME_LOGIN,
        username,
    }
}

export function createContainer(app: string, url: null | string, test = false) {
    return {
        type: CREATE_CONTAINER,
        app,
        url,
        test,
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

export function deleteContainer(
    containerID: string,
    privateKey: string,
    test = false
) {
    return {
        type: DELETE_CONTAINER,
        containerID,
        privateKey,
        test,
    }
}
