package subscriptions // import "github.com/whisthq/whist/backend/services/subscriptions"

// QueryInstanceByIdWithStatus returns an instance that matches the given instance_name and status.
var QueryInstanceByIdWithStatus struct {
	WhistInstances []whistInstance `graphql:"whist_instances(where: {id: {_eq: $id}, _and: {status: {_eq: $status}}})"`
}

// QueryInstanceById returns an instance that matches the given instance_name and status.
var QueryInstanceById struct {
	WhistInstances []whistInstance `graphql:"whist_instances(where: {id: {_eq: $id}})"`
}

// QueryInstanceStatus returns any instance that matches the given status.
var QueryInstancesByStatus struct {
	WhistInstances []whistInstance `graphql:"whist_instances(where: {status: {_eq: $status}})"`
}

// QueryInstanceWithCapacity returns an instance that has free space to run a mandelbox.
// Return results ordered by capacity, so we efficiently allocate mandelboxes.
var QueryInstanceWithCapacity struct {
	WhistInstances []whistInstance `graphql:"whist_instances(where: {region: {_eq: $region}, _and: {status: {_eq: $status}, _and: {remaining_capacity: {_gt: 0}}}}, order_by: {remaining_capacity: asc})"`
}

// QueryInstancesByImageID returns any instance with the given image id.
var QueryInstancesByImageID struct {
	WhistInstances []whistInstance `graphql:"whist_instances(where: {image_id: {_eq: $image_id}})"`
}

// QueryInstancesByStatusOnRegion returns any instance that matches the given status and
// is located on the given region.
var QueryInstancesByStatusOnRegion struct {
	WhistInstances []whistInstance `graphql:"whist_instances(where: {status: {_eq: $status} _and: {region: {_eq: $region}}})"`
}

// QueryMandelboxesById returns a mandelbox associated with the given instance
// and that has the given status.
var QueryMandelboxesByInstanceId struct {
	WhistMandelboxes []whistMandelbox `graphql:"whist_mandelboxes(where: {instance_id: {_eq: $instance_id}, _and: {status: {_eq: $status}}})"`
}

// QueryMandelboxStatus returns every mandelbox that matches the given status.
var QueryMandelboxByStatus struct {
	WhistMandelboxes []whistMandelbox `graphql:"whist_mandelboxes(where: {status: {_eq: $status}})"`
}

// QueryLatestImage returns the latest image from the database for the specified provider/region pair.
var QueryLatestImage struct {
	WhistImages []whistImage `graphql:"whist_images(where: {provider: {_eq: $provider}, _and: {region: {_eq: $region}}}, order_by: {updated_at: desc})"`
}

// QueryClientAppVersionChange returns the latest value on the `desktop_app_version` config database table.
var QueryClientAppVersion struct {
	WhistClientAppVersions []whistClientAppVersion `graphql:"desktop_app_version(where: {id: {_eq: $id}})"`
}

// QueryDevConfigurations will return all values in the `dev` table of the config database.
var QueryDevConfigurations struct {
	WhistConfigs `graphql:"dev"`
}

// QueryStagingConfigurations will return all values in the `staging` table of the config database.
var QueryStagingConfigurations struct {
	WhistConfigs `graphql:"staging"`
}

// QueryProdConfigurations will return all values in the `prod` table of the config database.
var QueryProdConfigurations struct {
	WhistConfigs `graphql:"prod"`
}
