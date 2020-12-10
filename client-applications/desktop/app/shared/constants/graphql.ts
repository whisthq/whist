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

// TODO maybe make this query by user and have users be unique?
// when we use this query let's get currentAppID from the action.ID when they click it
export const GET_CONTAINER_INFO = gql`
    query GetContainerInfo($container_id: String!) {
        hardware_user_containers(
            where: { container_id: { _eq: $container_id } }
        ) {
            user_id
            state
            port_32273
            port_32263
            port_32262
            ip
            location
            cluster
            container_id
            secret_key
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
