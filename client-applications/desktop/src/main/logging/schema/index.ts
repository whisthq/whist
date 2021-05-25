import { LoggingSchema } from "@app/main/logging/utils"
import launch from "@app/main/logging/schema/launch"
import auth from "@app/main/logging/schema/auth"

const schema: LoggingSchema = {
  ...launch,
  ...auth,
}

export default schema
