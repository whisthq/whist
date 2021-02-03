import { Timer } from "store/reducers/analytics/default"

export const UPDATE_TIMER = "UPDATE_TIMER"

export const updateTimer = (body: Timer) => {
    console.log("UPDATE TIMER")
    console.log(body)

    return {
        type: UPDATE_TIMER,
        body,
    }
}
