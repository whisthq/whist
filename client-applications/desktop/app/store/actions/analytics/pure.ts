import { Timer } from "store/reducers/analytics/default"

export const UPDATE_TIMER = "UPDATE_TIMER"

export const updateTimer = (body: Timer) => {
    return {
        type: UPDATE_TIMER,
        body,
    }
}
