import { createProvider } from "shared/utils/store"
import DEFAULT from "pages/auth/pages/forgot/shared/store/default"

export const { FractalProvider, useFractalProvider } = createProvider(DEFAULT)

