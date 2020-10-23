import { DEFAULT } from "store/reducers/waitlist/default"

import * as PureAction from "store/actions/waitlist/pure"
import * as SharedAction from "store/actions/shared"

import { if_exists_spread } from "shared/utils/reducerHelpers"

export default function (state = DEFAULT, action: any) {
    switch (action.type) {
        case PureAction.UPDATE_WAITLIST_USER:
            return {
                ...state,
                waitlistUser: if_exists_spread(state.waitlistUser, action.body),
            }
        case PureAction.UPDATE_CLICKS:
            return {
                ...state,
                clicks: if_exists_spread(state.clicks, action.body),
            }
        case PureAction.UPDATE_WAITLIST_DATA:
            return {
                ...state,
                waitlistData: if_exists_spread(state.waitlistData, action.body),
            }

        case PureAction.UPDATE_NAVIGATION:
            return {
                ...state,
                navigation: if_exists_spread(state.navigation, action.body),
            }
        case SharedAction.RESET_STATE:
            return DEFAULT
        default:
            return state
    }
}
