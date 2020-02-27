import { put, takeLatest, takeEvery, all, call, select, delay } from 'redux-saga/effects';
import { apiPost, apiGet } from '../utils/api.ts';
import * as Action from '../actions/counter';
import * as geolib from 'geolib';
import { configureStore, history } from '../store/configureStore';

function* loginUser(action) {
  console.log("saga called")
  const {json, response} = yield call(apiPost, 'https://cube-celery-vm.herokuapp.com/user/login', {
    username: action.username,
    password: action.password
  })

  console.log(json)

  if(json.username) {
    yield put(Action.storeUserInfo(action.username, json.public_ip));
    yield put(Action.calculateDistance(json.public_ip));
    history.push("/counter");
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

function* calculateDistance(public_ip) {
    const localIpUrl = require('local-ip-url');
    const iplocation = require("iplocation").default; 

    // var local_ip = localIpUrl('public', 'ipv4');
    var local_ip = "24.39.127.99"

    iplocation(public_ip).then((res1) => {
      iplocation(local_ip).then((res2) => {
         var dist = geolib.getDistance(
         { latitude: res1.latitude, longitude: res1.longitude },
         { latitude: res2.latitude, longitude: res2.longitude });
         dist = dist / 1609.34 + 1;
         console.log("The distance is "+ dist)
         // yield put(Action.storeDistance(500));
      }).catch(err => {
        console.log(err)
      });
    });
}

export default function* rootSaga() {
 	yield all([
     takeEvery(Action.TRACK_USER_ACTIVITY, trackUserActivity),
     takeEvery(Action.LOGIN_USER, loginUser),
     takeEvery(Action.CALCULATE_DISTANCE, calculateDistance)
	]);
}
