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
