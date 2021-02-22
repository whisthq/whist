export const ADD_SUBSCRIPTION = "ADD_SUBSCRIPTION"
export const DELETE_SUBSCRIPTION = "DELETE_SUBSCRIPTION"
export const ADD_CARD = "ADD_CARD"
export const DELETE_CARD = "DELETE_CARD"

export const addSubscription = (plan: string) => {
    return {
        type: ADD_SUBSCRIPTION,
        plan,
    }
}

export const deleteSubscription = (message: string) => {
    return {
        type: DELETE_SUBSCRIPTION,
        message,
    }
}

export const addCard = (source: any) => {
    return {
        type: ADD_CARD,
        source,
    }
}

export const deleteCard = (source: string) => {
    return {
        type: DELETE_CARD,
        source, // this should be replaced?
    }
}
