import { takeEvery, all, call } from "redux-saga/effects"
import { apiPost } from "utils/Api"
import { config } from "utils/constants"

import * as WaitlistAction from "store/actions/auth/waitlist"

import moment from "moment"

function* insertWaitlist(action: any) {
    const date = moment(action.closingDate).format("MMMM Do, YYYY")
    console.log("in saga")
    console.log(action.email)
    console.log(action.name)
    console.log(date)
    if (action.email) {
        yield call(
            apiPost,
            config.url.PRIMARY_SERVER + "/mail/joinWaitlist",
            {
                email: action.email,
                name: action.name,
                date: date,
            },
            ""
        )
    }
}

function* referEmail(action: any) {
    if (action.email) {
        yield call(
            apiPost,
            config.url.PRIMARY_SERVER + "/mail/waitlistReferral",
            {
                email: action.email,
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
