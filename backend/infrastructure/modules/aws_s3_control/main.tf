# This is the multi-region access point that is used for accessing and
# routing requests to the user config buckets in order to reduce latency.
resource "aws_s3control_multi_region_access_point" "user-configs-access-point" {
  details {
    name = "whist-user-configs-access-point-${var.env}"

    dynamic "region" {
      for_each = var.buckets
      content {
        bucket = region.value
      }
    }
  }
}
