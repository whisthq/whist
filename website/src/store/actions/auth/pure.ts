export const UPDATE_USER = "UPDATE_USER"
export const RESET_STATE = "RESET_STATE"

export function updateUser(body: { user_id: string; name: string }) {
    return {
        type: UPDATE_USER,
        body,
    }
}
