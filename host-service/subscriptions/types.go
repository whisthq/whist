package subscriptions

type Instance struct {
	Instance_name string
	AwsAmiId      string
	Status        string
}

type SubscriptionStatusResult struct {
	Hardware_instance_info []Instance
}
