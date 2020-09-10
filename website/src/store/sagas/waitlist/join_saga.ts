import { put, takeEvery, all, call, select } from "redux-saga/effects";
import { apiPost } from "utils/Api";
import { config } from "utils/constants";

import * as WaitlistAction from "store/actions/auth/waitlist";

import moment from "moment";

function* insertWaitlist(action) {
    const date = moment(action.date).format("MMMM Do, YYYY");
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
        );
    }
}

export default function* () {
    yield all([takeEvery(WaitlistAction.INSERT_WAITLIST, insertWaitlist)]);
}
