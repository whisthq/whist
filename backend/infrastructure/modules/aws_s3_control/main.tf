resource "aws_s3control_multi_region_access_point" "user-configs-access-point" {
  details {
    name = "user-configs-access-point"

    dynamic "region" {
      for_each = var.regions
      content {
        bucket = "whist-user-app-configs-${region["value"]}-${var.env}"
      }
    }
  }
}
