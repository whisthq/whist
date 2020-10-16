import { DEFAULT } from "store/reducers/waitlist/default"

import * as PureAction from "store/actions/waitlist/pure"
import * as SharedAction from "store/actions/shared"

export default function (state = DEFAULT, action: any) {
    switch (action.type) {
        case PureAction.UPDATE_WAITLIST_USER:
            return {
                ...state,
                user: Object.assign(state.waitlistUser, action.body),
            }
        case PureAction.UPDATE_CLICKS:
            return {
                ...state,
                clicks: Object.assign(state.clicks, action.body),
            }
        case PureAction.UPDATE_WAITLIST_DATA:
            return {
                ...state,
                waitlistData: Object.assign(state.waitlistData, action.body),
            }

        case PureAction.UPDATE_NAVIGATION:
            return {
                ...state,
                navigation: Object.assign(state.navigation, action.body),
            }
        case SharedAction.RESET_STATE:
            return DEFAULT
        default:
            return state
    }
}
