import { Observable } from "rxjs"
import { map, tap } from "rxjs/operators"
import { stringify } from "postcss"

import { loginError } from "@app/main/testing/state"

export const mockState = (res: any) => {
  let obj = JSON.parse(res)
  obj = { ...obj, ...loginError }
  return obj
}
