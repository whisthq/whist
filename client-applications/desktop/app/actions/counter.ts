import { GetState, Dispatch } from '../reducers/types';

export const STORE_USER_INFO     = "STORE_USER_INFO";
export const TRACK_USER_ACTIVITY = "TRACK_USER_ACTIVITY";
export const LOGIN_USER          = "LOGIN_USER";
export const LOGIN_FAILED        = "LOGIN_FAILED";
export const CALCULATE_DISTANCE  = "CALCULATE_DISTANCE"
export const STORE_DISTANCE      = "STORE_DISTANCE";

export function loginUser(username, password) {
	console.log("login action fired")
	return {
		type: LOGIN_USER,
		username,
		password
	}
}

export function storeUserInfo(username, public_ip) {
  return {
    type: STORE_USER_INFO,
    username,
    public_ip
  };
}

export function trackUserActivity(logon) {
	return {
		type: TRACK_USER_ACTIVITY,
		logon
	}
}

export function loginFailed() {
	return {
		type: LOGIN_FAILED
	}
}

export function calculateDistance(public_ip) {
	return {
		type: CALCULATE_DISTANCE,
		public_ip
	}
}

export function storeDistance(distance) {
	console.log("store distance action " + distance)
	return {
		type: STORE_DISTANCE,
		distance
	}
}