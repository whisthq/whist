import { takeEvery, all, call } from "redux-saga/effects"

import { apiPost } from "shared/utils/api"

import * as WaitlistAction from "store/actions/auth/waitlist"

import moment from "moment"

const JOIN_EMAIL_ENABLED = false

function* insertWaitlist(action: any) {
    const date = moment(action.closingDate).format("MMMM Do, YYYY")
    if (action.user_id && JOIN_EMAIL_ENABLED) {
        yield call(
            apiPost,
            "/mail/joinWaitlist",
            {
                email: action.user_id,
                name: action.name,
                date: date,
            },
            ""
        )
    }
}

function* referEmail(action: any) {
    if (action.user_id) {
        const { json } = yield call(
            apiPost,
            "/mail/waitlistReferral",
            {
                email: action.user_id,
                name: action.name,
                code: action.code,
                recipient: action.recipient,
            },
            ""
        )
    }
}

export default function* () {
    yield all([
        takeEvery(WaitlistAction.INSERT_WAITLIST, insertWaitlist),
        takeEvery(WaitlistAction.REFER_EMAIL, referEmail),
    ])
}
