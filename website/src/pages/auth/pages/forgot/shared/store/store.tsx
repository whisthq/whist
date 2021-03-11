import { createProvider } from "shared/utils/store"
import DEFAULT from "pages/auth/pages/forgot/shared/store/default"

const STORAGE_KEY = "forgotPasswordState"

export const { FractalProvider, useFractalProvider } = createProvider(DEFAULT, STORAGE_KEY)

