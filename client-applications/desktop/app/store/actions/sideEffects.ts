export const LOGIN_USER = "LOGIN_USER"
export const GOOGLE_LOGIN = "GOOGLE_LOGIN"

export const FETCH_CONTAINER = "FETCH_CONTAINER"
export const DELETE_CONTAINER = "DELETE_CONTAINER"

export const SUBMIT_FEEDBACK = "SUBMIT_FEEDBACK"

export function loginUser(username: any, password: any) {
    return {
        type: LOGIN_USER,
        username,
        password,
    }
}

export function googleLogin(code: any) {
    return {
        type: GOOGLE_LOGIN,
        code,
    }
}

export function fetchContainer(app: string) {
    return {
        type: FETCH_CONTAINER,
        app,
    }
}

export function deleteContainer(username: string, container_id: string) {
    return {
        type: DELETE_CONTAINER,
        username,
        container_id,
    }
}

export function submitFeedback(feedback: string, feedback_type: string) {
    return {
        type: SUBMIT_FEEDBACK,
        feedback,
        feedback_type,
    }
}
