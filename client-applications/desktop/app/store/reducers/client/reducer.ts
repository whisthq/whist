import { DEFAULT, Timer } from "store/reducers/client/default"
import * as ClientAction from "store/actions/client/pure"
import { deepCopyObject } from "shared/utils/general/reducer"

export default (
    state = DEFAULT,
    action: {
        body: Timer
        type: string
    }
) => {
    /*
        Description:
            Reducer for analytics actions
        Arguments:
            body (Timer): Action body 
            type (string): Action type
    */

    const stateCopy = deepCopyObject(state)

    switch (action.type) {
        case ClientAction.UPDATE_TIMER:
            return {
                ...stateCopy,
                timer: Object.assign(stateCopy.timer, action.body),
            }
        case ClientAction.UPDATE_COMPUTER_INFO:
            return {
                ...stateCopy,
                computerInfo: Object.assign(
                    stateCopy.computerInfo,
                    action.body
                ),
            }
        default:
            return state
    }
}
