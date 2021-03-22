
all:
	@echo "Install all dependencies to a folder called package"
	@echo "You can do this with: pip install --target ./package <packages>"
	@echo "Then run: make lambda-func"

lambda-func:
	rm -f my-deployment-package.zip
	cd package && zip -r ../my-deployment-package.zip .
	zip -g my-deployment-package.zip lambda_function.py load_test.py load_test_setup.py load_test_utils.py

	# upload the package to AWS lambda
	@echo ""
	@echo "Exporting to AWS..."
	aws lambda update-function-code --function-name LoadTest --zip-file fileb://my-deployment-package.zip

