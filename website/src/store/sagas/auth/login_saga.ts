import { put, takeEvery, all, call, select } from "redux-saga/effects"

import * as LoginAction from "store/actions/auth/login_actions"

// eslint-disable-next-line require-yield
function* googleLogin(action: any) {
    // More to be added later
    if (action.user_id) {
        // console.log(action.email)
    }
}

export default function* () {
    yield all([takeEvery(LoginAction.GOOGLE_LOGIN, googleLogin)])
}
