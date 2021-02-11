import { put, takeEvery, all } from "redux-saga/effects"
import os from "os"

import * as ClientAction from "store/actions/client/sideEffects"
import { updateComputerInfo } from "store/actions/client/pure"
import { allowedRegions } from "shared/types/aws"
import { FractalLogger } from "shared/utils/general/logging"
import { setAWSRegion } from "shared/utils/files/aws"

function* getComputerInfo() {
    const logger = new FractalLogger()

    const operatingSystem = os.platform()

    let region = allowedRegions[0]
    try {
        region = yield call(setAWSRegion)
        logger.logInfo(`Fetch AWS region found region ${region}`)
    } catch (err) {
        logger.logError(
            `Fetch AWS region errored with ${err}, using default region ${region}`
        )
    }

    yield put(
        updateComputerInfo({ operatingSystem: operatingSystem, region: region })
    )
}

export default function* clientSaga() {
    yield all([takeEvery(ClientAction.GET_COMPUTER_INFO, getComputerInfo)])
}
