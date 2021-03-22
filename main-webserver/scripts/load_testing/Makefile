
lambda-func:
	rm -rf ./package
	@echo "Installing dependencies in requirements.txt to ./package"
	pip install --target ./package -r requirements.txt
	rm -f my-deployment-package.zip
	cd package && zip -r ../my-deployment-package.zip .
	# these are all the files to package
	zip -g my-deployment-package.zip lambda_function.py load_test.py load_test_setup.py load_test_utils.py

	# upload the package to AWS lambda
	@echo ""
	@echo "Exporting to AWS..."
	aws lambda update-function-code --function-name LoadTest --zip-file fileb://my-deployment-package.zip

