from .imports import *
from .helperFuncs import *

stripe_bp = Blueprint('stripe_bp', __name__)

@stripe_bp.route('/stripe/<action>', methods = ['POST'])
def payment(action):
	stripe.api_key = os.getenv('STRIPE_SECRET') 
	customer_id = ''
	subscription_id = ''

	if action == 'charge':
		body = request.get_json()

		token = body['token']
		email = body['email']
		location = body['location']
		code = body['code']

		customers = fetchCustomers()
		for customer in customers:
			if email == customer['email']:
				return jsonify({'status': 400}), 400

		try:
			new_customer = stripe.Customer.create(
			  email = email,
			  source = token
			)
			customer_id = new_customer['id']
			credits = getUserCredits(email)

			if mapCodeToUser(code):
				credits += 1

			if credits == 0:
				new_subscription = stripe.Subscription.create(
				  customer = new_customer['id'],
				  items = [{"plan": os.getenv("PLAN_ID")}],
				  trial_end = shiftUnixByWeek(dateToUnix(getToday()), 1),
				  trial_from_plan = False
				)
				subscription_id = new_subscription['id']
			else:
				new_subscription = stripe.Subscription.create(
				  customer = new_customer['id'],
				  items = [{"plan": os.getenv("PLAN_ID")}],
				  trial_end = shiftUnixByMonth(dateToUnix(getToday()), credits),
				  trial_from_plan = False
				)
				subscription_id = new_subscription['id']
		except:
			return jsonify({'status': 402}), 402

		try:
			insertCustomer(email, customer_id, subscription_id, location)
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
				try:
					payload = stripe.Subscription.retrieve(subscription)
					return jsonify({'status': 200, 'subscription': payload}), 200
				except:
					return jsonify({'status': 402}), 402

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

	elif action == 'discount':
		body = request.get_json()

		code = body['code']
		metadata = mapCodeToUser(code)

		if not metadata:
			return jsonify({'status': 404}), 404

		creditsOutstanding = metadata['creditsOutstanding']
		email = metadata['email']
		has_subscription = False
		subscription_id = None

		customers = fetchCustomers()
		for customer in customers:
			if email == customer['email']:
				has_subscription = True
				subscription_id = customer['subscription']

		if has_subscription:
			new_subscription = stripe.Subscription.retrieve(subscription_id)
			if new_subscription['trial_end']:
			    if new_subscription['trial_end'] <= dateToUnix(getToday()):
			        modified_subscription = stripe.Subscription.modify(
			            new_subscription['id'],
			            trial_end = shiftUnixByMonth(dateToUnix(getToday()), 1),
			            trial_from_plan = False
			        )
			        
			    modified_subscription = stripe.Subscription.modify(
			        new_subscription['id'],
			        trial_end = shiftUnixByMonth(retreived_subscription['trial_end'], 1),
			        trial_from_plan = False
			    )
			else:
			    modified_subscription = stripe.Subscription.modify(
			        new_subscription['id'],
			        trial_end = shiftUnixByMonth(dateToUnix(getToday()), 1),
			        trial_from_plan = False
			    )

		else:
			changeUserCredits(email, creditsOutstanding + 1)

		url = "https://fractal-mail-server.herokuapp.com/referral"
		data = {'username': email}
		requests.post(url = url, data = data) 

