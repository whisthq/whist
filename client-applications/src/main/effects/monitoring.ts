import { logBase } from "@app/utils/logging"
import { interval } from "rxjs"

const minutes = 10

interval((minutes > 0 ? minutes : 10) * 60 * 1000).subscribe(() => {
  logBase("heartbeat", {}).catch((err) => console.log(err))
})
