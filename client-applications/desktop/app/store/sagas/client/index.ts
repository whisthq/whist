import { put, takeEvery, all } from "redux-saga/effects"
import os from "os"

import * as ClientAction from "store/actions/client/sideEffects"
import { updateComputerInfo } from "store/actions/client/pure"

function* getOperatingSystem() {
    const operatingSystem = os.platform()
    console.log("found operating system", operatingSystem)
    yield put(updateComputerInfo({ operatingSystem: operatingSystem }))
}

export default function* clientSaga() {
    yield all([
        takeEvery(ClientAction.GET_OPERATING_SYSTEM, getOperatingSystem),
    ])
}
