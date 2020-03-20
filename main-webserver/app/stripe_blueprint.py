from .imports import *
from .helperFuncs import *

stripe_bp = Blueprint('stripe_bp', __name__)

@stripe_bp.route('/stripe/<action>', methods = ['POST'])
def stripe(action):
	stripe.api_key = os.getenv('STRIPE_SECRET') 

	if action == 'charge':
		body = request.get_json()

		token = body['token']
		email = body['email']

		customers = fetchCustomers()
		for customer in customers:
			if email == customer['email']:
				return jsonify({'status': 400}), 400


		new_customer = stripe.Customer.create(
		  email = email,
		  source = token
		)

		stripe.Subscription.create(
		  customer = new_customer['id'],
		  items = [{"plan": "plan_GwV769WQdZOUJR"}]
		)

		insertCustomer(email, new_customer['id'])

		return jsonify({'status': 200}), 200

	elif action == 'retrieve':
		body = request.get_json()

		email = body['email']
		customers = fetchCustomers()
		for customer in customers:
			if email == customer['email']:
				return jsonify({'status': 200}), 200

		return jsonify({'status': 400}), 400