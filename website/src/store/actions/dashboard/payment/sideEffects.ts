export const ADD_SUBSCRIPTION = "ADD_SUBSCRIPTION"
export const DELETE_SUBSCRIPTION = "DELETE_SUBSCRIPTION"
export const ADD_CARD = "ADD_CARD"
export const DELETE_CARD = "DELETE_CARD"

export function addSubscription(plan: string) {
    return {
        type: ADD_SUBSCRIPTION,
        plan,
    }
}

export function deleteSubscription(message: string) {
    return {
        type: DELETE_SUBSCRIPTION,
        message,
    }
}

export function addCard(source: any) {
    return {
        type: ADD_CARD,
        source,
    }
}

export function deleteCard(source: string) {
    return {
        type: DELETE_CARD,
        source, // this should be replaced?
    }
}
