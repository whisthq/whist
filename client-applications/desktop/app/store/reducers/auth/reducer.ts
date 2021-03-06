import { DEFAULT, User, AuthFlow } from "store/reducers/auth/default"
import * as AuthAction from "store/actions/auth/pure"
import { deepCopyObject } from "shared/utils/general/reducer"

export default (
    state = DEFAULT,
    action: {
        body: User | AuthFlow
        type: string
    }
) => {
    /*
        Description:
            Reducer for auth actions
        Arguments:
            body (User | AuthFlow): Action body 
            type (string): Action type
    */

    const stateCopy = deepCopyObject(state)

    switch (action.type) {
        case AuthAction.UPDATE_USER:
            return {
                ...stateCopy,
                user: Object.assign(stateCopy.user, action.body),
            }
        case AuthAction.UPDATE_AUTH_FLOW:
            return {
                ...stateCopy,
                authFlow: Object.assign(stateCopy.authFlow, action.body),
            }
        default:
            return state
    }
}
