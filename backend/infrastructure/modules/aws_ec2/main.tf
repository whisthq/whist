resource "aws_ec2_availability_zone_group" "enabled_regions" {
    for_each = var.enabled_regions
    group_name    = each.value
    opt_in_status = "opted-in"
}
