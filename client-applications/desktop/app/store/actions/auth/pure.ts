import { User, AuthFlow } from "store/reducers/auth/default"

export const UPDATE_USER = "UPDATE_USER"
export const UPDATE_AUTH_FLOW = "UPDATE_AUTH_FLOW"

export const updateUser = (body: User) => {
    return {
        type: UPDATE_USER,
        body,
    }
}

export const updateAuthFlow = (body: AuthFlow) => {
    return {
        type: UPDATE_AUTH_FLOW,
        body,
    }
}
