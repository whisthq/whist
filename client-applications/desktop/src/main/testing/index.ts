import { Observable } from "rxjs"
import { map, tap } from "rxjs/operators"

export const test = (res) => (res.status = 200)
