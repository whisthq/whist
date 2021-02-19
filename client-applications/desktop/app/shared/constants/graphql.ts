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

export const ADD_LOGIN_TOKEN = gql`
    mutation AddLoginToken($object: tokens_insert_input!) {
        insert_tokens_one(object: $object) {
            login_token
            access_token
        }
    }
`

export const SUBSCRIBE_USER_ACCESS_TOKEN = gql`
    subscription GetAccessToken($loginToken: String!) {
        tokens(where: { login_token: { _eq: $loginToken } }) {
            login_token
            access_token
        }
    }
`

export const DELETE_USER_TOKENS = gql`
    mutation DeleteTokens($loginToken: String!) {
        delete_tokens(where: { login_token: { _eq: $loginToken } }) {
            affected_rows
        }
    }
`
