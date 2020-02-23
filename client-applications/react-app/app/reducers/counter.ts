import { Action } from 'redux';
import { STORE_USER_INFO } from '../actions/counter';

const DEFAULT = {username: null, public_ip: null}

export default function counter(state = DEFAULT, action: Action<string>) {
  switch (action.type) {
    case STORE_USER_INFO:
      return {
      	...state,
      	username: action.username,
      	public_ip: action.public_ip
      }
    default:
      return state;
  }
}
