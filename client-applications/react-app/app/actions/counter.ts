import { GetState, Dispatch } from '../reducers/types';

export const STORE_USER_INFO = "STORE_USER_INFO";

export function storeUserInfo(username, public_ip) {
	console.log("store user info action")
  return {
    type: STORE_USER_INFO,
    username,
    public_ip
  };
}