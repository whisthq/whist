import { gql } from '@apollo/client'

export const GET_FEATURED_APPS = gql`
    query GetFeaturedApps {
        hardware_supported_app_images {
            app_id
            logo_url
            category
            description
            long_description
            url
        }
    }
`
