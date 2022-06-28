resource "aws_s3control_multi_region_access_point" "example" {
  details {
    name = "user-configs-access-point"

    dynamic "region" {
      for_each = var.regions
      content {
          bucket = "whist-user-app-configs-${region}-${var.env}"
      }
    }
  }
}
