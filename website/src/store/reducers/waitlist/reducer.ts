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
        case SharedAction.REFRESH_STATE:
            const mergeInto: any = deep_copy(DEFAULT) // yikes if no deepcopy
            // note the use of copy... in future more nested situations we want a full deep copy
            const userType: { [key: string]: any } = stateCopy.waitlistUser // again ts syntax
            // this should be removed later when we remove clicks, for now it's important to copy the previous
            // since we don't want to let them keep reseting the clicks and nothing else
            const clicksType: { [key: string]: any } = stateCopy.clicks // again ts syntax
            const waitlistDataType: { [key: string]: any } =
                stateCopy.waitlistData // again ts syntax

            if (state.waitlistUser && state.waitlistUser.user_id) {
                Object.keys(state.waitlistUser).forEach((key: string) => {
                    if (userType[key]) {
                        mergeInto.waitlistUser[key] = userType[key]
                    }
                })

                Object.keys(state.clicks).forEach((key: string) => {
                    if (clicksType[key]) {
                        mergeInto.clicks[key] = clicksType[key]
                    }
                })

                Object.keys(state.waitlistData).forEach((key: string) => {
                    if (waitlistDataType[key]) {
                        mergeInto.waitlistData[key] = waitlistDataType[key]
                    }
                })
            }

            // waitlist data should be allowed to be updated from the server so it will be fresh

            // navigation is for redirects mainly so we'll reset since if they crashed on application
            // they may want to resubmit
            if (state.navigation && state.navigation.applicationRedirect) {
                // note the use of the copy (again if this were a data structure...)
                mergeInto.navigation.applicationRedirect =
                    stateCopy.navigation.applicationRedirect
            }

            return mergeInto
        default:
            return state
    }
}
