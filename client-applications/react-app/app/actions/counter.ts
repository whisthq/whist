import { GetState, Dispatch } from '../reducers/types';

export const STORE_USER_INFO = "STORE_USER_INFO";
export const TRACK_USER_ACTIVITY = "TRACK_USER_ACTIVITY";

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