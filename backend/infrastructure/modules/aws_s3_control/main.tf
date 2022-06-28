resource "aws_s3control_multi_region_access_point" "example" {
  details {
    name = "user-configs-access-point"

    dynamic {
      for_each = var.regions
      content {
        region {
          bucket = aws_s3_bucket.foo_bucket.id
        }
      }
    }
  }
}