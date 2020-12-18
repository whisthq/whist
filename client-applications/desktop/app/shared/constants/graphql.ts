import { gql } from "@apollo/client"

export const GET_FEATURED_APPS = gql`
    query GetFeaturedApps {
        hardware_supported_app_images {
            app_id
            logo_url
            category
            description
            long_description
            url
            tos
            active
        }
    }
`

export const GET_BANNERS = gql`
    query GetBanners {
        hardware_banners {
            background
            category
            heading
            subheading
            url
        }
    }
`
export const SUBSCRIBE_USER_APP_STATE = gql`
    subscription GetContainerInfo($userID: String!) {
        hardware_user_app_state(where: { user_id: { _eq: $userID } }) {
            state
            task_id
        }
    }
`

// TODO (adriano) in the future we'll want to change the table somehow to signify
// which is the latest (or delete old rows)
export const GET_USER_CONTAINER = gql`
    query GetUserContainers($userID: String!) {
        hardware_user_containers(
            where: { user_id: { _eq: $userID } }
            order_by: { last_pinged: desc_nulls_last }
            limit: 1
        ) {
            ip
            container_id
            cluster
            location
            port_32262
            port_32263
            port_32273
            secret_key
            user_id
        }
    }
`
