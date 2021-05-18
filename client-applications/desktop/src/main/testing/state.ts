// export const loginError = {
//   status: 400,
//   json: {
//     access_token: "",
//   },
// }
import { EMPTY, of } from "rxjs"

export const loginError = {
  authFlow: {
    success: () => EMPTY,
    failure: () => of({ status: 400, json: { access_token: "" } }),
  },
}
