package subscriptions

import "github.com/hasura/go-graphql-client"

var (

	// InsertInstances inserts multiple instances to the database.
	InsertInstances struct {
		MutationResponse struct {
			AffectedRows graphql.Int `graphql:"affected_rows"`
		} `graphql:"insert_whist_instances(objects: $objects)"`
	}

	// UpdateInstance updates the fields of an instance.
	UpdateInstance struct {
		MutationResponse struct {
			AffectedRows graphql.Int `graphql:"affected_rows"`
		} `graphql:"update_whist_instances(where: {id: {_eq: $id}}, _set: $changes)"`
	}

	// DeleteInstanceStatusById deletes an instance with the given id and its mandelboxes from the database.
	DeleteInstanceById struct {
		MandelboxesMutationResponse struct {
			AffectedRows graphql.Int `graphql:"affected_rows"`
		} `graphql:"delete_whist_mandelboxes(where: {instance_id: {_eq: $id}})"`
		InstancesMutationResponse struct {
			AffectedRows graphql.Int `graphql:"affected_rows"`
		} `graphql:"delete_whist_instances(where: {id: {_eq: $id}})"`
	}

	// InsertInstances inserts multiple instances to the database.
	InsertMandelboxes struct {
		MutationResponse struct {
			AffectedRows graphql.Int `graphql:"affected_rows"`
		} `graphql:"insert_whist_mandelboxes(objects: $objects)"`
	}

	// UpdateMandelbox updates the row of a mandelbox on the database.
	UpdateMandelbox struct {
		MutationResponse struct {
			AffectedRows graphql.Int `graphql:"affected_rows"`
		} `graphql:"update_whist_mandelboxes(where: {id: {_eq: $id}}, _set: $changes)"`
	}

	// InsertImages inserts multiple images to the database.
	InsertImages struct {
		MutationResponse struct {
			AffectedRows graphql.Int `graphql:"affected_rows"`
		} `graphql:"insert_whist_images(objects: $objects)"`
	}

	// UpdateImage updates the row of an image to the given values.
	UpdateImage struct {
		MutationResponse struct {
			AffectedRows graphql.Int `graphql:"affected_rows"`
		} `graphql:"update_whist_images(where: {region: {_eq: $region}, _and: {provider: {_eq: $provider}}}, _set: {client_sha: $client_sha, image_id: $image_id, provider: $provider, region: $region, updated_at: $updated_at})"`
	}
)
