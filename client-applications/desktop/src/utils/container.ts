import { screen } from "electron"
import { pick } from "lodash"
import { containerRequest, taskStatus } from "@app/utils/api"
import { AsyncReturnType } from "@app/utils/types"

const getDPI = () => screen.getPrimaryDisplay().scaleFactor * 72

const chooseRegion = () => {
    return [
        "us-east-1",
        "us-east-2",
        "us-west-1",
        "us-west-2",
        "ca-central-1",
        "eu-central-1",
    ][0]
}

export const containerCreate = async (email: string, accessToken: string) =>
    await containerRequest(email, accessToken, chooseRegion(), getDPI())

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
