/**
 * Copyright Fractal Computers, Inc. 2021
 * @file tray.ts
 * @brief This file contains all RXJS observables created from events caused by clicking on the tray.
 */

import { signout, quit } from "@app/main/utils/tray"
import { trigger } from "@app/main/utils/flows"

trigger("signoutAction", signout)
trigger("quitAction", quit)