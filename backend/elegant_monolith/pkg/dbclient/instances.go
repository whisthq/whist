package dbclient

import "context"

func (client *Client) QueryInstance(ctx context.Context, id string) (Instance, error) {
	var instance Instance
	client.db.First(&instance, id)
	return instance
}

func QueryInstanceWithCapacity(context.Context, string) (Instance, error) {
}

func QueryInstancesByStatusOnRegion(context.Context, string, string) ([]Instance, error) {
}

func QueryInstancesByImage(context.Context, string) ([]Instance, error) {
}

func InsertInstances(context.Context, []Instance) (int, error) {
}

func UpdateInstance(context.Context, Instance) (int, error) {
}

func DeleteInstance(context.Context, string) (int, error) {
}
