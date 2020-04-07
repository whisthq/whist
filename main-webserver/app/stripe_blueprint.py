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
		trial_end = 0

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

			metadata = mapCodeToUser(code)
			if metadata:
				credits += 1

			if credits == 0:
				trial_end = shiftUnixByWeek(dateToUnix(getToday()), 1)
				new_subscription = stripe.Subscription.create(
				  customer = new_customer['id'],
				  items = [{"plan": os.getenv("PLAN_ID")}],
				  trial_end = trial_end,
				  trial_from_plan = False
				)
				subscription_id = new_subscription['id']
			else:
				trial_end = shiftUnixByMonth(dateToUnix(getToday()), credits)
				new_subscription = stripe.Subscription.create(
				  customer = new_customer['id'],
				  items = [{"plan": os.getenv("PLAN_ID")}],
				  trial_end = trial_end,
				  trial_from_plan = False
				)
				changeUserCredits(email, 0)
				subscription_id = new_subscription['id']
		except Exception as e:
			return jsonify({'status': 402, 'error': str(e)}), 402

		try:
			insertCustomer(email, customer_id, subscription_id, location, trial_end, True)
		except:
			return jsonify({'status': 409}), 409

		return jsonify({'status': 200}), 200

	elif action == 'retrieve':
		body = request.get_json()

		email = body['email']
		credits = getUserCredits(email)
		customers = fetchCustomers()
		for customer in customers:
			if email == customer['email']:
				subscription = customer['subscription']
				try:
					payload = stripe.Subscription.retrieve(subscription)
					updateTrialEnd(payload['id'], payload['trial_end'])
					account_locked = not customer['paid'] and not payload['trial_end']
					return jsonify({'status': 200, 'subscription': payload, 'creditsOutstanding': credits, 'account_locked': account_locked, 'customer': customer}), 200
				except Exception as e:
					account_locked = not customer['paid'] and customer['trial_end'] < dateToUnix(getToday())
					return jsonify({'status': 402, 'creditsOutstanding': credits, 'account_locked': account_locked, 'customer': customer}), 402

		return jsonify({'status': 402, 'creditsOutstanding': credits}), 402

	elif action == 'cancel':
		body = request.get_json()

		email = body['email']
		customers = fetchCustomers()
		for customer in customers:
			if email == customer['email']:
				subscription = customer['subscription']
				try:
					payload = stripe.Subscription.delete(subscription)
				except Exception as e:
					print(e)
					pass
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
		trial_end = 0

		customers = fetchCustomers()
		for customer in customers:
			if email == customer['email']:
				has_subscription = True
				subscription_id = customer['subscription']

		if has_subscription:
			new_subscription = stripe.Subscription.retrieve(subscription_id)
			if new_subscription['trial_end']:
				if new_subscription['trial_end'] <= dateToUnix(getToday()):
					trial_end = shiftUnixByMonth(dateToUnix(getToday()), 1)
					modified_subscription = stripe.Subscription.modify(
						new_subscription['id'],
						trial_end = trial_end,
						trial_from_plan = False
					)
					updateTrialEnd(new_subscription['id'], trial_end)
				else:
					trial_end = shiftUnixByMonth(new_subscription['trial_end'], 1)
					modified_subscription = stripe.Subscription.modify(
						new_subscription['id'],
						trial_end = trial_end,
						trial_from_plan = False
					)
					updateTrialEnd(new_subscription['id'], trial_end)
			else:
				trial_end = shiftUnixByMonth(dateToUnix(getToday()), 1)
				modified_subscription = stripe.Subscription.modify(
					new_subscription['id'],
					trial_end = trial_end,
					trial_from_plan = False
				)
				updateTrialEnd(new_subscription['id'], trial_end)

		else:
			changeUserCredits(email, creditsOutstanding + 1)

		headers = {'content-type': 'application/json'}
		url = "https://fractal-mail-server.herokuapp.com/creditApplied"
		data = {'username': email}
		requests.post(url = url, data = json.dumps(data), headers = headers) 

		return jsonify({'status': 200}), 200

	elif action == 'insert':
		body = request.get_json()
		trial_end =  shiftUnixByWeek(dateToUnix(getToday()), 1)
		insertCustomer(body['email'], None, None, body['location'], trial_end, False)
		return jsonify({'status': 200}), 200

@stripe_bp.route('/referral/<action>', methods = ['POST'])
def referral(action):
	if action == 'validate':
		body = request.get_json()
		code = body['code']
		code_username = None
		username = body['username']

		metadata = mapCodeToUser(code)
		if metadata:
			code_username = metadata['email']
		if username == code_username:
			return jsonify({'status': 200, 'verified': False}), 200
		codes = fetchCodes()
		verified = code in codes
		return jsonify({'status': 200, 'verified': verified}), 200