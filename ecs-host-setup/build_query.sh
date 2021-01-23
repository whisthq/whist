Q1='{"query":"query get_orig_ami {hardware_region_to_ami(where: {region_name: {_eq: \"'
Q2='\"}}) {ami_id}}","variables":{}}'
QUERY="${Q1}${1}${Q2}"
echo $QUERY