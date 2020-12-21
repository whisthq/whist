import { takeEvery, all, call, select } from "redux-saga/effects"

import { apiPost } from "shared/utils/api"

import * as SideEffectWaitlistAction from "store/actions/waitlist/sideEffects"

// const JOIN_EMAIL_ENABLED = false

// function* insertWaitlist(action: any) {
//     const date = moment(action.closingDate).format("MMMM Do, YYYY")
//     if (action.user_id && JOIN_EMAIL_ENABLED) {
//         yield call(
//             apiPost,
//             "/mail/joinWaitlist",
//             {
//                 email: action.user_id,
//                 name: action.name,
//                 date: date,
//             },
//             ""
//         )
//     }
// }

function* sendReferralEmail(action: any) {
    const state = yield select()
    if (state.WaitlistReducer.waitlistUser.user_id) {
        yield call(
            apiPost,
            "/mail/waitlistReferral",
            {
                email: state.WaitlistReducer.waitlistUser.user_id,
                name: state.WaitlistReducer.waitlistUser.name,
                code: state.WaitlistReducer.waitlistUser.referralCode,
                recipient: action.recipient,
            },
            ""
        )
    }
}

export default function* () {
    yield all([
        // takeEvery(WaitlistAction.INSERT_WAITLIST, insertWaitlist),
        takeEvery(
            SideEffectWaitlistAction.SEND_REFERRAL_EMAIL,
            sendReferralEmail
        ),
    ])
}
