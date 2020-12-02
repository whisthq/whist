import * as MainAction from "store/actions/pure"
import { DEFAULT } from "store/reducers/states"

import { deep_copy } from "shared/utils/reducerHelpers"

const MainReducer = <T extends {}>(state = DEFAULT, action: T) => {
    var stateCopy = deep_copy(state)
    switch (action.type) {
        case MainAction.RESET_STATE:
            return DEFAULT
        case MainAction.UPDATE_AUTH:
            return {
                ...stateCopy,
                auth: Object.assign(stateCopy.auth, action.body),
            }
        case MainAction.UPDATE_CLIENT:
            return {
                ...stateCopy,
                client: Object.assign(stateCopy.client, action.body),
            }
        case MainAction.UPDATE_CONTAINER:
            return {
                ...stateCopy,
                container: Object.assign(stateCopy.container, action.body),
            }
        case MainAction.UPDATE_LOADING:
            return {
                ...stateCopy,
                loading: Object.assign(stateCopy.loading, action.body),
            }
        case MainAction.UPDATE_PAYMENT:
            return {
                ...stateCopy,
                payment: Object.assign(stateCopy.payment, action.body),
            }
        default:
            return state
    }
}

export default MainReducer
