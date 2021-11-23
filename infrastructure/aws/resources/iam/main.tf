provider "aws" {
  region = var.region
}

resource "aws_iam_user" "whist-employees" {
  for_each = var.employee_emails
  name = each.value
}

resource "aws_iam_user" "whist-admins" {
  for_each = var.admin_emails
  name = each.value
}

resource "aws_iam_group" "fractal-engineering-group" {
  name = "FractalEngineering"
}

# This will add each managed policy to the group
resource "aws_iam_group_policy_attachment" "FractalEngineering_policy" {
  group      = aws_iam_group.fractal-engineering-group.name
  for_each   = var.fractal-engineering-managed-policies
  policy_arn = each.key
}

resource "aws_iam_group" "mfa" {
  name = "2FA"
}

resource "aws_iam_group_policy" "mfa-policy" {
  name  = "2FA"
  group = aws_iam_group.mfa.name
  policy = jsonencode(var.mfa-policy)
}

resource "aws_iam_group" "fractal-aws-admins" {
  name = "FractalAWSAdmins"
}

resource "aws_iam_group_policy_attachment" "fractal-aws-admin-policy" {
  group      = aws_iam_group.fractal-aws-admins.name
  for_each   = var.fractal-aws-admins-managed-policies
  policy_arn = each.key
}

resource "aws_iam_user_group_membership" "employee-membership" {
  for_each = aws_iam_user.whist-employees
  user   = aws_iam_user.whist-employees[each.key].name
  groups = [aws_iam_group.fractal-engineering-group.name, aws_iam_group.mfa.name]
}