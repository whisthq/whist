import { Timer, ComputerInfo } from "store/reducers/client/default"

export const UPDATE_TIMER = "UPDATE_TIMER"
export const UPDATE_COMPUTER_INFO = "UPDATE_COMPUTER_INFO"

export const updateTimer = (body: Timer) => {
    return {
        type: UPDATE_TIMER,
        body,
    }
}

export const updateComputerInfo = (body: ComputerInfo) => {
    return {
        type: UPDATE_COMPUTER_INFO,
        body,
    }
}
