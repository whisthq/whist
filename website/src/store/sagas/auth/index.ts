import { put, takeEvery, all, call, select } from "redux-saga/effects"

import * as Action from "store/actions/auth/pure"

// eslint-disable-next-line require-yield
function* googleLogin(action: any) {
    // More to be added later
    if (action.user_id) {
    }
}

export default function* () {
    // yield all([takeEvery(Action.GOOGLE_LOGIN, googleLogin)])
}
