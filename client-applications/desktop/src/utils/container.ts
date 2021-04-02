import { screen } from "electron"
import { pick, map } from "lodash"

import { defaultAllowedRegions } from "@app/@types/aws"
import { chooseRegion } from "@app/utils/region"
import { containerRequest, regionRequest, taskStatus } from "@app/utils/api"
import { AsyncReturnType } from "@app/utils/types"

const getDPI = () => screen.getPrimaryDisplay().scaleFactor * 72

export const regionGet = async (email: string, accessToken: string) => {
    const regions = await regionRequest(email, accessToken)

    if (!regions.json) {
        return chooseRegion(defaultAllowedRegions)
    } else {
        return chooseRegion(regions.json)
    }
}

export const containerCreate = async (email: string, accessToken: string) => {
    const region = await regionGet(email, accessToken)
    return await containerRequest(email, accessToken, region, getDPI())
}

export const containerCreateError = (
    response: AsyncReturnType<typeof containerCreate>
) => !response?.json?.ID

export const containerInfo = async (taskID: string, accessToken: string) =>
    await taskStatus(taskID, accessToken)

export const containerInfoError = (
    response: AsyncReturnType<typeof containerInfo>
) => !response?.json?.state

export const containerInfoSuccess = (
    response: AsyncReturnType<typeof containerInfo>
) => response.json.state === "SUCCESS"

export const containerInfoPending = (
    response: AsyncReturnType<typeof containerInfo>
) => response.json.state !== "SUCCESS" && response.json.state !== "FAILURE"

export const containerInfoPorts = (
    response: AsyncReturnType<typeof containerInfo>
) => pick(response.json.output, ["port_32262", "port_32263", "port_32273"])

export const containerInfoIP = (
    response: AsyncReturnType<typeof containerInfo>
) => response.json.output.ip

export const containerInfoSecretKey = (
    response: AsyncReturnType<typeof containerInfo>
) => response.json.output.secret_key
