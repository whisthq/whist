import { put, takeEvery, all, call, select, delay } from "redux-saga/effects";
import { apiPost, apiGet } from "utils/api";
import * as Action from "store/actions/counter_actions";
import { history } from "store/configureStore";

import { config } from "constants/config";

function* loginUser(action: any) {
    if (action.username !== "" && action.password !== "") {
        const { json } = yield call(
            apiPost,
            `${config.url.PRIMARY_SERVER}/account/login`,
            {
                username: action.username,
                password: action.password,
            }
        );

        if (json && json.verified) {
            yield put(Action.storeUsername(action.username));
            yield put(Action.storeIsUser(json.is_user));
            yield put(Action.storeJWT(json.access_token, json.refresh_token));
            yield put(Action.fetchDisk(action.username));
            yield call(fetchPaymentInfo, action);
            yield call(getPromoCode, action);
            history.push("/dashboard");
        } else {
            yield put(Action.loginFailed(true));
        }
    } else {
        yield put(Action.loginFailed(true));
    }
}

function* fetchPaymentInfo(action: any) {
    const state = yield select();
    const { json } = yield call(
        apiPost,
        `${config.url.PRIMARY_SERVER}/stripe/retrieve`,
        {
            username: action.username,
        },
        state.counter.access_token
    );

    if (json && json.account_locked) {
        yield put(Action.storePaymentInfo(json.account_locked));
    }
}

function* getPromoCode(action: any) {
    const state = yield select();
    const { json } = yield call(
        apiGet,
        `${config.url.PRIMARY_SERVER}/account/code?username=${action.username}`,
        state.counter.access_token
    );

    console.log(json);

    if (json && json.status === 200) {
        yield put(Action.storePromoCode(json.code));
    }
}

export default function* rootSaga() {
    yield all([takeEvery(Action.LOGIN_USER, loginUser)]);
}
