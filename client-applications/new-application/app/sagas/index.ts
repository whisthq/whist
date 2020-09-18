import { put, takeEvery, all, call, select, delay } from "redux-saga/effects";
import { apiPost, apiGet } from "utils/api";
import * as Action from "actions/counter";
import { history } from "store/configureStore";

import { config } from "constants/config";

function* loginUser(action) {
    console.log("Login action handled here");
}

export default function* rootSaga() {
    yield all([takeEvery(Action.LOGIN_USER, loginUser)]);
}
