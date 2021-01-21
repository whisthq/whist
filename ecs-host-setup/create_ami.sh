# create new ami in us-east-1 using packer.io and configuration file
SOURCE_AMI=$(grep -oE 'ami-[0-9a-z]+' <<< $(packer build ami_config.json)
SOURCE_REGION='us-east-1'

# add us-east-1 ami to db
Q1='{"query":"mutation NewAMI($ami_id: String = \"'
Q2='\", $_eq: String = \"'
Q3='\") {\n  update_hardware_region_to_ami(where: {region_name: {_eq: $_eq}}, _set: {ami_id: $ami_id}) {\n    returning {\n      region_name\n      ami_id\n    }\n  }\n}","variables":{}}'
QUERY="$Q1$SOURCE_AMI$Q2$SOURCE_REGION$Q3"

curl --location --request POST 'http://dev-database.fractal.co/v1/graphql' \
--header 'Content-Type: application/json' \
--header 'x-hasura-admin-secret: WhFpeUYnKxGsa8L' \
--data-raw "$QUERY"

# create copy of us-east-1 ami for all regions
REGION='eu-central-1'
AMI=$(grep -oE 'ami-[0-9a-z]+' <<< $(aws ec2 copy-image --source-image-id $SOURCE_AMI --source-region $SOURCE_REGION --region $REGION --name "testing-packer-ami-copying"))

# build query to update database with new ami's
Q1='{"query":"mutation NewAMI($ami_id: String = \"'
Q2='\", $_eq: String = \"'
Q3='\") {\n  update_hardware_region_to_ami(where: {region_name: {_eq: $_eq}}, _set: {ami_id: $ami_id}) {\n    returning {\n      region_name\n      ami_id\n    }\n  }\n}","variables":{}}'
QUERY="$Q1$AMI$Q2$REGION$Q3"

curl --location --request POST 'http://dev-database.fractal.co/v1/graphql' \
--header 'Content-Type: application/json' \
--header 'x-hasura-admin-secret: WhFpeUYnKxGsa8L' \
--data-raw "$QUERY"
