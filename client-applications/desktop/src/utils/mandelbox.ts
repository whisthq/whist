/**
 * Copyright Fractal Computers, Inc. 2021
 * @file mandelbox.ts
 * @brief This file contains utility functions interacting with the webserver to create mandelboxes.
 */
import { screen } from "electron"
import { pick } from "lodash"

import { get, post } from "@app/utils/api"
import { defaultAllowedRegions, AWSRegion } from "@app/@types/aws"
import { chooseRegion } from "@app/utils/region"
import { AsyncReturnType } from "@app/@types/state"

// For the purposes of the low-level rendering which is performed by the
// protocol, the default DPI is always 96; this is modified by a scale factor
// on high-resolution monitors, as they scale up assets to keep sizes
// consistent with their low-resolution counterparts.
const getDPI = () => screen.getPrimaryDisplay().scaleFactor * 96

export const regionGet = async (subClaim: string, accessToken: string) => {
    const regions: Record<string, any> = await regionRequest(
        subClaim,
        accessToken
    )
    const allowedRegions = (regions?.json as AWSRegion[]) ?? []

    if (allowedRegions.length === 0) {
        return await chooseRegion(defaultAllowedRegions)
    } else {
        return await chooseRegion(regions.json)
    }
}

export const mandelboxCreate = async (
    subClaim: string,
    accessToken: string,
    region?: AWSRegion
) => {
    region = region ?? (await regionGet(subClaim, accessToken))
    const response = await mandelboxRequest(
        subClaim,
        accessToken,
        region,
        getDPI()
    )
    return response
}

export const mandelboxCreateSuccess = (
    response: AsyncReturnType<typeof mandelboxCreate>
) => [200, 202].includes(response.status as number)

export const mandelboxCreateErrorNoAccess = (
    response: AsyncReturnType<typeof mandelboxCreate>
) => response.status === 402

export const mandelboxCreateErrorUnauthorized = (
    response: AsyncReturnType<typeof mandelboxCreate>
) => response.status === 422 || response.status === 401

export const mandelboxCreateErrorMaintenance = (
    response: AsyncReturnType<typeof mandelboxCreate>
) => response.status === 512

export const mandelboxCreateErrorInternal = (
    response: AsyncReturnType<typeof mandelboxCreate>
) =>
    (response?.json?.ID ?? "") === "" &&
    !mandelboxCreateErrorNoAccess(response) &&
    !mandelboxCreateErrorUnauthorized(response) &&
    !mandelboxCreateErrorMaintenance(response)

export const mandelboxPolling = async (taskID: string, accessToken: string) =>
    await taskStatus(taskID, accessToken)

export const mandelboxPollingError = (
    response: AsyncReturnType<typeof mandelboxPolling>
) => (response?.json?.state ?? "") === ""

export const mandelboxPollingSuccess = (
    response: AsyncReturnType<typeof mandelboxPolling>
) => response?.json?.state === "SUCCESS"

export const mandelboxPollingPending = (
    response: AsyncReturnType<typeof mandelboxPolling>
) => response?.json?.state !== "SUCCESS" && response?.json?.state !== "FAILURE"

export const mandelboxPollingPorts = (
    response: AsyncReturnType<typeof mandelboxPolling>
) => pick(response?.json?.output, ["port_32262", "port_32263", "port_32273"])

export const mandelboxPollingIP = (
    response: AsyncReturnType<typeof mandelboxPolling>
) => response?.json?.output?.ip

export const mandelboxPollingSecretKey = (
    response: AsyncReturnType<typeof mandelboxPolling>
) => response?.json?.output?.secret_key

// Helper functions

const taskStatus = async (taskID: string, accessToken: string) =>
    get({ endpoint: "/status/" + taskID, accessToken })

const mandelboxRequest = async (
    username: string,
    accessToken: string,
    region: string,
    dpi: number
) =>
    post({
        endpoint: "/mandelbox/assign",
        accessToken,
        body: {
            username,
            region,
            dpi,
            app: "Google Chrome",
        },
    })

const regionRequest = async (username: string, accessToken: string) =>
    get({
        endpoint: `/regions?username=${username}`,
        accessToken,
    })
