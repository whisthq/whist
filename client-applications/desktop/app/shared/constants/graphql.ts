import { gql, useQuery } from "@apollo/client"

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

export const graphqlQuery = (query, data) => {
    const ret = useQuery(query, data)
    if ("error" in ret) {
        console.error(ret.error)
    }
    return ret
}
