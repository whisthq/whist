import { put, takeEvery, all, call, select } from "redux-saga/effects";

import * as LoginAction from "store/actions/auth/login_actions";

function* googleLogin(action) {
    // More to be added later
    if (action.email) {
        console.log(action.email);
    }
}

export default function* () {
    yield all([takeEvery(LoginAction.GOOGLE_LOGIN, googleLogin)]);
}
