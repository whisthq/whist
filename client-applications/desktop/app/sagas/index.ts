import { put, take, takeLatest, takeEvery, all, call, select, delay } from 'redux-saga/effects';
import { channel } from 'redux-saga';
import { apiPost, apiGet } from '../utils/api.ts';
import * as Action from '../actions/counter';
import { configureStore, history } from '../store/configureStore';


function* loginUser(action) {
  const {json, response} = yield call(apiPost, 'https://cube-celery-vm.herokuapp.com/account/login', {
    username: action.username,
    password: action.password
  })

  if(json && json.verified) {
    yield put(Action.fetchVMs(action.username))

    var email = action.username
    action.username = email.split('@')[0]
    yield put(Action.storeUsername(action.username))
    yield put(Action.storeIsUser(json.is_user))
    history.push("/counter");
  } else {
    yield put(Action.loginFailed());
  }
}

function* fetchVMs(action) {
  console.log("FETCHING VMS")
  const state = yield select()
  const {json, response} = yield call(apiPost, 'https://cube-celery-vm.herokuapp.com/user/login', {
    username: action.username
  })

  console.log(json)
  if(json && Object.keys(json).length > 0) {
    yield put(Action.storeIP(json.public_ip))
  } else {
    yield put(Action.storeIP(''))
  }
  console.log("DONE")
  yield put(Action.fetchVMStatus(true))
}

function* loginStudio(action) {
  const {json, response} = yield call(apiPost, 'https://cube-celery-vm.herokuapp.com/account/login', {
    username: action.username,
    password: action.password
  })

  if(json && json.verified) {
    yield put(Action.storeUsername(action.username))
    yield put(Action.storeIsUser(json.is_user))
    yield put(Action.storeIP(''))
    history.push("/studios");
  } else {
    yield put(Action.loginFailed());
  }
}

function* trackUserActivity(action) {
  const state = yield select()
  if(action.logon) {
    const {json, response} = yield call(apiPost, 'https://cube-celery-vm.herokuapp.com/tracker/logon', {
      username: state.counter.username
    })
  } else {
    const {json, response} = yield call(apiPost, 'https://cube-celery-vm.herokuapp.com/tracker/logoff', {
      username: state.counter.username
    })
  }
}

function* sendFeedback(action) {
  const state = yield select()
  const {json, response} = yield call(apiPost, 'https://fractal-mail-server.herokuapp.com/feedback', {
    username: state.counter.username,
    feedback: action.feedback
  })
  yield put(Action.resetFeedback(true))
}

function* pingIPInfo(action) {
  const state = yield select()
  const {json, response} = yield call(apiGet, 'https://ipinfo.io?token=926e38ce447823')
  yield put(Action.storeIPInfo(json, action.id))
}

function* storeIPInfo(action) {
  const state = yield select()
  var location = action.payload.city + ", " + action.payload.region

  const {json, response} = yield call(apiPost, 'https://cube-celery-vm.herokuapp.com/account/checkComputer', {
    id: action.id,
    username: state.counter.username
  })


  if(json && json.status === 200) {
    if(json.computers[0].found) {
      console.log("ID FOUND")
      console.log(json)
      yield put(Action.fetchComputers())
    } else {
      console.log("ID NOT FOUND")
      console.log(json)
      const {json1, response1} = yield call(apiPost, 'https://cube-celery-vm.herokuapp.com/account/insertComputer', {
        id: action.id,
        username: state.counter.username,
        location: location,
        nickname: json.computers[0].nickname
      })
      yield put(Action.fetchComputers())
    }
  }
}

function* fetchComputers(action) {
  const state = yield select()
  const {json, response} = yield call(apiPost, 'https://cube-celery-vm.herokuapp.com/account/fetchComputers', {
    username: state.counter.username
  })
  yield put(Action.storeComputers(json.computers))
}


export default function* rootSaga() {
 	yield all([
     takeEvery(Action.TRACK_USER_ACTIVITY, trackUserActivity),
     takeEvery(Action.LOGIN_USER, loginUser),
     takeEvery(Action.SEND_FEEDBACK, sendFeedback),
     takeEvery(Action.LOGIN_STUDIO, loginStudio),
     takeEvery(Action.PING_IPINFO, pingIPInfo),
     takeEvery(Action.STORE_IPINFO, storeIPInfo),
     takeEvery(Action.FETCH_COMPUTERS, fetchComputers),
     takeEvery(Action.FETCH_VMS, fetchVMs)
  ])
}
