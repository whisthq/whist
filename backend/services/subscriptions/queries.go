package subscriptions // import "github.com/whisthq/whist/backend/services/subscriptions"

var (

	// QueryInstanceByIdWithStatus returns an instance that matches the given instance_name and status.
	QueryInstanceByIdWithStatus struct {
		WhistInstances []WhistInstance `graphql:"whist_instances(where: {id: {_eq: $id}, _and: {status: {_eq: $status}}})"`
	}

	// QueryInstanceById returns an instance that matches the given instance_name and status.
	QueryInstanceById struct {
		WhistInstances []WhistInstance `graphql:"whist_instances(where: {id: {_eq: $id}})"`
	}

	// QueryInstanceStatus returns any instance that matches the given status.
	QueryInstancesByStatus struct {
		WhistInstances []WhistInstance `graphql:"whist_instances(where: {status: {_eq: $status}})"`
	}

	// QueryInstanceWithCapacity returns an instance that has free space to run a mandelbox.
	// Return results ordered by capacity, so we efficiently allocate mandelboxes.
	QueryInstanceWithCapacity struct {
		WhistInstances []WhistInstance `graphql:"whist_instances(where: {region: {_eq: $region}, _and: {status: {_eq: $status}, _and: {remaining_capacity: {_gt: 0}}}}, order_by: {remaining_capacity: asc})"`
	}

	// QueryInstancesByImageID returns any instance with the given image id.
	QueryInstancesByImageID struct {
		WhistInstances []WhistInstance `graphql:"whist_instances(where: {image_id: {_eq: $image_id}})"`
	}

	// QueryInstancesByStatusOnRegion returns any instance that matches the given status and
	// is located on the given region.
	QueryInstancesByStatusOnRegion struct {
		WhistInstances []WhistInstance `graphql:"whist_instances(where: {status: {_eq: $status} _and: {region: {_eq: $region}}})"`
	}

	// QueryMandelboxesById returns a mandelbox associated with the given instance
	// and that has the given status.
	QueryMandelboxesByInstanceId struct {
		WhistMandelboxes []WhistMandelbox `graphql:"whist_mandelboxes(where: {instance_id: {_eq: $instance_id}, _and: {status: {_eq: $status}}})"`
	}

	// QueryMandelboxesById returns a mandelbox associated with the given instance
	// and that has the given status.
	QueryMandelboxesByUserId struct {
		WhistMandelboxes []WhistMandelbox `graphql:"whist_mandelboxes(where: {user_id: {_eq: $user_id}})"`
	}

	// QueryLatestImage returns the latest image from the database for the specified provider/region pair.
	QueryLatestImage struct {
		WhistImages []WhistImage `graphql:"whist_images(where: {provider: {_eq: $provider}, _and: {region: {_eq: $region}}}, order_by: {updated_at: desc})"`
	}

	// QueryFrontendVersion returns the latest value on the `desktop_app_version` config database table.
	QueryFrontendVersion struct {
		WhistFrontendVersions []WhistFrontendVersion `graphql:"desktop_app_version(where: {id: {_eq: $id}})"`
	}

	// QueryDevConfigurations will return all values in the `dev` table of the config database.
	QueryDevConfigurations struct {
		WhistConfigs `graphql:"dev"`
	}

	// QueryStagingConfigurations will return all values in the `staging` table of the config database.
	QueryStagingConfigurations struct {
		WhistConfigs `graphql:"staging"`
	}

	// QueryProdConfigurations will return all values in the `prod` table of the config database.
	QueryProdConfigurations struct {
		WhistConfigs `graphql:"prod"`
	}
)
