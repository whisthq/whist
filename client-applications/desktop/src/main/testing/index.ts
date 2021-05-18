import { Observable, of } from "rxjs"
import { map, tap } from "rxjs/operators"
import { stringify } from "postcss"
import { get } from "lodash"
import { loginError } from "@app/main/testing/state"

export const mockState = (value: any, name: string, key: string) => {
  const func = get(loginError, [name, key], of)
  // console.log("NAME: ", name, "KEY ", key)
  return func(value)
}
