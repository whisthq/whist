import { gql } from "@apollo/client"

export const SUBSCRIBE_USER_APP_STATE = gql`
    subscription GetContainerInfo($taskID: String!) {
        hardware_user_app_state(where: { task_id: { _eq: $taskID } }) {
            state
            task_id
        }
    }
`

export const QUERY_REGION_TO_AMI = gql`
    query GetRegionToAmi {
        hardware_region_to_ami(where: { allowed: { _eq: true } }) {
            region_name
        }
    }
`
