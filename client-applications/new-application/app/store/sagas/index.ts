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
    console.log(json)

    const id = json.ID
    console.log(id)
    var { json, response } = yield call(
        apiGet,
        `${config.url.PRIMARY_SERVER}/status/` + id,
        state.counter.access_token
    )
    console.log(json)
    while (json.state !== 'SUCCESS' && json.state !== 'FAILURE') {
        var { json, response } = yield call(
            apiGet,
            `${config.url.PRIMARY_SERVER}/status/` + id,
            state.counter.access_token
        )

        if (response && response.status && response.status === 500) {
            const warning =
                `(${moment().format('hh:mm:ss')}) ` +
                'Unexpectedly lost connection with server. Please close the app and try again.'
            yield put(Action.changePercentLoaded(0))
            yield put(Action.changeStatusMessage(warning))
        }

        if (json && json.state === 'PENDING' && json.output) {
            var message = json.output.msg
            var percent = json.output.progress
            if (message && percent) {
                yield put(Action.changePercentLoaded(percent))
                yield put(Action.changeStatusMessage(message))
            }
        }

        yield delay(5000)
    }
    if (json && json.state && json.state === 'SUCCESS') {
        if (json.output) {
            // const container_id = json.output.container_id
            // const cluster = json.output.cluster
            // const ip = json.output.ip
            // const port_32262 = json.output.port_32262
            // const port_32263 = json.output.port_32263
            // const port_32273 = json.output.port_32273
            // const location = json.output.location
            const container_id = 'container_id'
            const cluster = 'cluster'
            const ip = 'ip'
            const port_32262 = 'port_32262'
            const port_32263 = 'port_32263'
            const port_32273 = 'port_32273'
            const location = 'location'
            yield put(Action.storeIP(ip))
            yield put(
                Action.storeResources(
                    container_id,
                    cluster,
                    port_32262,
                    port_32263,
                    port_32273,
                    location
                )
            )
        }

        yield put(Action.changePercentLoaded(100))
        yield put(Action.changeStatusMessage('SUCCESS.'))
    } else {
        var warning =
            `(${moment().format('hh:mm:ss')}) ` +
            `Unexpectedly lost connection with server. Please close the app and try again.`
        yield put(Action.changeStatusMessage(warning))
        // yield put(Action.attachDisk());
    }
}

export default function* rootSaga() {
    yield all([
        takeEvery(Action.LOGIN_USER, loginUser),
        takeEvery(Action.FETCH_CONTAINER, fetchContainer),
    ])
}
