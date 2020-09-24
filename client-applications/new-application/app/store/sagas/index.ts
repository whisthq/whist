import { put, takeEvery, all, call, select, delay } from 'redux-saga/effects'
import { apiPost, apiGet } from 'utils/api'
import * as Action from 'store/actions/counter'
import { history } from 'store/configureStore'

import { config } from 'constants/config'

import moment from 'moment'

function* loginUser(action: any) {
    if (action.username !== '' && action.password !== '') {
        const { json } = yield call(
            apiPost,
            `${config.url.PRIMARY_SERVER}/account/login`,
            {
                username: action.username,
                password: action.password,
            }
        )

        if (json && json.verified) {
            yield put(Action.storeUsername(action.username))
            yield put(Action.storeIsUser(json.is_user))
            yield put(Action.storeJWT(json.access_token, json.refresh_token))
            yield put(Action.fetchDisk(action.username))
            yield call(fetchPaymentInfo, action)
            yield call(getPromoCode, action)
            history.push('/dashboard')
        } else {
            yield put(Action.loginFailed(true))
        }
    } else {
        yield put(Action.loginFailed(true))
    }
}

function* fetchPaymentInfo(action: any) {
    const state = yield select()
    const { json } = yield call(
        apiPost,
        `${config.url.PRIMARY_SERVER}/stripe/retrieve`,
        {
            username: action.username,
        },
        state.counter.access_token
    )

    if (json && json.account_locked) {
        yield put(Action.storePaymentInfo(json.account_locked))
    }
}

function* getPromoCode(action: any) {
    const state = yield select()
    const { json } = yield call(
        apiGet,
        `${config.url.PRIMARY_SERVER}/account/code?username=${action.username}`,
        state.counter.access_token
    )

    console.log(json)

    if (json && json.status === 200) {
        yield put(Action.storePromoCode(json.code))
    }
}

function* fetchContainer(action) {
    const state = yield select()
    var { json, response } = yield call(
        apiGet,
        `${config.url.PRIMARY_SERVER}/dummy`,
        state.counter.access_token
    )
    const id = json ? json.id : ''

    console.log(json.state)
    while (json.state !== 'SUCCESS' && json.state !== 'FAILURE') {
        var { json, response } = yield call(
            apiGet,
            `${config.url.PRIMARY_SERVER}/status/` + id,
            state.counter.access_token
        )

        if (response && response.status && response.status === 500) {
            const warning =
                'Unexpectedly lost connection with server. Please close the app and log back in.'
            yield put(Action.changeStatusMessage(warning))
        }
        if (json) {
            console.log(json)
        }

        if (json && json.meta && json.state === 'PENDING') {
            var message = json.meta['msg']
            yield put(Action.changeStatusMessage(message))
        }

        yield delay(5000)
    }
    if (json && json.state && json.state === 'SUCCESS') {
        // if (json.output && json.output.ip) {
        //     yield put(Action.storeIP(json.output.ip));
        //     yield put(
        //         Action.storeResources(
        //             json.output.disk_name,
        //             json.output.vm_name,
        //             json.output.location
        //         )
        //     );
        // }
        console.log('SUCCESS')
    } else {
        var message =
            `(${moment().format('hh:mm:ss')}) ` +
            `Unexpectedly lost connection with server. Trying again.`
        yield put(Action.changeStatusMessage(message))
        // yield put(Action.attachDisk());
    }
}

export default function* rootSaga() {
    yield all([
        takeEvery(Action.LOGIN_USER, loginUser),
        takeEvery(Action.FETCH_CONTAINER, fetchContainer),
    ])
}
