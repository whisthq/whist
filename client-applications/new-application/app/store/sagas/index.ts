import { put, takeEvery, all, call, select, delay } from 'redux-saga/effects'
import { apiPost, apiGet } from 'utils/api'
import * as Action from 'store/actions/main'
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
            yield put(Action.storeJWT(json.access_token, json.refresh_token))
            yield call(fetchPaymentInfo, action)
            yield call(getPromoCode, action)
            history.push('/loading')
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
        state.MainReducer.access_token
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
        state.MainReducer.access_token
    )

    console.log(json)

    if (json && json.status === 200) {
        yield put(Action.storePromoCode(json.code))
    }
}

function* fetchContainer(action: any) {
    const state = yield select()
    // const username = 'fractal-admin@gmail.com'
    // const app = 'test'
    // var { json, response } = yield call(
    //     apiPost,
    //     `${config.url.PRIMARY_SERVER}/container/create`,
    //     { username: username, app: app },
    //     state.MainReducer.access_token
    // )
    var { json, response } = yield call(
        apiGet,
        `${config.url.PRIMARY_SERVER}/dummy`,
        state.MainReducer.access_token
    )

    const id = json.ID
    console.log(id)
    var { json, response } = yield call(
        apiGet,
        `${config.url.PRIMARY_SERVER}/status/` + id,
        state.MainReducer.access_token
    )
    console.log(json)
    while (json.state !== 'SUCCESS' && json.state !== 'FAILURE') {
        var { json, response } = yield call(
            apiGet,
            `${config.url.PRIMARY_SERVER}/status/` + id,
            state.MainReducer.access_token
        )

        if (response && response.status && response.status === 500) {
            const warning =
                `(${moment().format('hh:mm:ss')}) ` +
                'Unexpectedly lost connection with server. Please close the app and try again.'
            yield put(Action.changePercentLoaded(0))
            yield put(Action.changeStatusMessage(warning))
        }

        if (json && json.state === 'PENDING' && json.output) {
            // NOTE: actual container/create endpoint does not currently return progress
            var message = json.output.msg
            var percent = json.output.progress
            if (message && percent) {
                yield put(Action.changePercentLoaded(percent))
                yield put(Action.changeStatusMessage(message))
            }
        }

        yield delay(5000)
    }
    // testing params : -w200 -h200 -p32262:32780,32263:32778,32273:32779 34.206.64.200
    if (json && json.state && json.state === 'SUCCESS') {
        if (json.output) {
            // TODO (adriano) these should be removed once we are ready to plug and play
            const test_container_id = 'container_id' // TODO
            const test_cluster = 'cluster' // TODO
            const test_ip = '34.206.64.200'
            const test_port_32262 = '32780'
            const test_port_32263 = '32778'
            const test_port_32273 = '32779'
            const test_location = 'location' // TODO

            const test_width = 200
            const test_height = 200
            const test_codec = 'h264'

            // TODO (adriano) add a signaling param or something to say that it's the 'test'
            // or it's the actual thing
            const container_id = json.output.container_id
                ? json.output.container_id
                : test_container_id
            const cluster = json.output.cluster
                ? json.output.cluster
                : test_cluster
            const ip = json.output.ip ? json.output.ip : test_ip
            const port_32262 = json.output.port_32262
                ? json.output.port_32262
                : test_port_32262
            const port_32263 = json.output.port_32263
                ? json.output.port_32263
                : test_port_32263
            const port_32273 = json.output.port_32273
                ? json.output.port_32273
                : test_port_32273
            const location = json.output.location
                ? json.output.location
                : test_location

            const width = test_width
            const height = test_height
            const codec = test_codec

            yield put(Action.storeDimensions(width, height))

            yield put(Action.storeIP(ip))

            yield put(Action.storeCodec(codec))

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
            // yield put(Action.deleteContainer(username, container_id))
        }

        yield put(Action.changePercentLoaded(100))
        yield put(Action.changeStatusMessage('Successfully created container.'))
    } else {
        var warning =
            `(${moment().format('hh:mm:ss')}) ` +
            `Unexpectedly lost connection with server. Please close the app and try again.`
        yield put(Action.changeStatusMessage(warning))
    }
}

function* deleteContainer(action: any) {
    const state = yield select()
    var { json, response } = yield call(
        apiPost,
        `${config.url.PRIMARY_SERVER}/container/delete`,
        { username: action.username, container_id: action.container_id },
        state.MainReducer.access_token
    )
    const id = json.id
    console.log('DELETING CONTAINER')
    console.log(action.username)
    console.log(action.container_id)
    console.log(json)
    var { json, response } = yield call(
        apiGet,
        `${config.url.PRIMARY_SERVER}/status/` + id,
        state.MainReducer.access_token
    )

    while (json.state !== 'SUCCESS' && json.state !== 'FAILURE') {
        var { json, response } = yield call(
            apiGet,
            `${config.url.PRIMARY_SERVER}/status/` + id,
            state.MainReducer.access_token
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
            if (message) {
                yield put(Action.changeStatusMessage(message))
            }
        }

        yield delay(5000)
    }

    if (json && json.state && json.state === 'SUCCESS') {
        yield put(Action.changeStatusMessage('Successfully deleted container'))
    } else {
        var warning =
            `(${moment().format('hh:mm:ss')}) ` +
            `Unexpectedly lost connection with server. Please close the app and try again.`
        if (json.output && json.output.msg) {
            warning = json.output.msg
        }
        yield put(Action.changeStatusMessage(warning))
    }
}

export default function* rootSaga() {
    yield all([
        takeEvery(Action.LOGIN_USER, loginUser),
        takeEvery(Action.FETCH_CONTAINER, fetchContainer),
        takeEvery(Action.DELETE_CONTAINER, deleteContainer),
    ])
}
