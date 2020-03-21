from .imports import *
from .helperFuncs import *

stripe_bp = Blueprint('stripe_bp', __name__)

@stripe_bp.route('/stripe/<action>', methods = ['POST'])
def payment(action):
	stripe.api_key = os.getenv('STRIPE_SECRET') 

	if action == 'charge':
		body = request.get_json()

		token = body['token']
		email = body['email']
		location = body['location']

		customers = fetchCustomers()
		for customer in customers:
			if email == customer['email']:
				return jsonify({'status': 400}), 400

		try:
			new_customer = stripe.Customer.create(
			  email = email,
			  source = token
			)

			new_subscription = stripe.Subscription.create(
			  customer = new_customer['id'],
			  items = [{"plan": "plan_Gwmtik1r6PD8Dw"}]
			)
		except:
			return jsonify({'status': 402}), 402

		try:
			insertCustomer(email, new_customer['id'], new_subscription['id'], location)
		except:
			return jsonify({'status': 409}), 409

		return jsonify({'status': 200}), 200

	elif action == 'retrieve':
		body = request.get_json()

		email = body['email']
		customers = fetchCustomers()
		for customer in customers:
			if email == customer['email']:
				subscription = customer['subscription']
				payload = stripe.Subscription.retrieve(subscription)
				return jsonify({'status': 200, 'subscription': payload}), 200

		return jsonify({'status': 400}), 400

	elif action == 'cancel':
		body = request.get_json()

		email = body['email']
		customers = fetchCustomers()
		for customer in customers:
			if email == customer['email']:
				subscription = customer['subscription']
				payload = stripe.Subscription.delete(subscription)
				deleteCustomer(email)
				return jsonify({'status': 200}), 200
		return jsonify({'status': 400}), 400
