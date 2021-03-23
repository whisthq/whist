
lambda-func:
	rm -rf ./package
	@echo "Installing dependencies in requirements.txt to ./package"
	pip install --target ./package -r requirements.txt
	rm -f my-deployment-package.zip
	cd package && zip -r ../my-deployment-package.zip .
	# these are all the files to package; update if things change
	# zip follows symlinks so the scripts folder (really symlinked to the scripts folder 
	# in main-webserver root) will be packaged
	zip -r -g my-deployment-package.zip lambda_function.py load_test_core.py scripts/

	# upload the package to AWS lambda
	@echo ""
	@echo "Exporting to AWS..."
	aws lambda update-function-code --function-name LoadTest --zip-file fileb://my-deployment-package.zip

