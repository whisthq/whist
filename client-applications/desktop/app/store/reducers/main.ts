import * as MainAction from "store/actions/pure"
import { DEFAULT } from "store/reducers/states"

import { deepCopyObject } from "shared/utils/reducerHelpers"

const MainReducer = (
    state = DEFAULT,
    action: {
        type: string
        body: Record<string, any>
    }
) => {
    const stateCopy = deepCopyObject(state)
    switch (action.type) {
        case MainAction.RESET_STATE:
            return DEFAULT
        case MainAction.UPDATE_ADMIN:
            return {
                ...stateCopy,
                admin: Object.assign(stateCopy.admin, action.body),
            }
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
        case MainAction.UPDATE_APPS:
            return {
                ...stateCopy,
                payment: Object.assign(stateCopy.apps, action.body),
            }
        default:
            return state
    }
}

export default MainReducer
