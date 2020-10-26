import { DEFAULT } from "store/reducers/waitlist/default"

import * as PureAction from "store/actions/waitlist/pure"
import * as SharedAction from "store/actions/shared"

import { deep_copy } from "shared/utils/reducerHelpers"

export default function (state = DEFAULT, action: any) {
    var stateCopy = deep_copy(state)
    switch (action.type) {
        case PureAction.UPDATE_WAITLIST_USER:
            return {
                ...stateCopy,
                waitlistUser: Object.assign(
                    stateCopy.waitlistUser,
                    action.body
                ),
            }
        case PureAction.UPDATE_CLICKS:
            return {
                ...stateCopy,
                clicks: Object.assign(stateCopy.clicks, action.body),
            }
        case PureAction.UPDATE_WAITLIST_DATA:
            return {
                ...stateCopy,
                waitlistData: Object.assign(
                    stateCopy.waitlistData,
                    action.body
                ),
            }

        case PureAction.UPDATE_NAVIGATION:
            return {
                ...stateCopy,
                navigation: Object.assign(stateCopy.navigation, action.body),
            }
        case PureAction.RESET_WAITLIST_USER:
            return {
                ...stateCopy,
                waitlistUser: DEFAULT.waitlistUser,
            }
        case SharedAction.RESET_STATE:
            return DEFAULT
        default:
            return state
    }
}
