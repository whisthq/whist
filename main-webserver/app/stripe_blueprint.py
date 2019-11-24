from .imports import *
from .helperFuncs import *
from app import conn

stripe_bp = Blueprint('stripe_bp', __name__)

@stripe_bp.route('/stripe', methods = ['POST'])
def charge():
	stripe.api_key = os.getenv('STRIPE_SECRET') 
	body = request.get_json()
	token = body['token']
	amount = body['amount']
	charge = stripe.Charge.create(
		amount = amount,
		currency = 'usd',
		description = 'Fractal Instance',
		source = token
	)
	return jsonify({'status': 200}), 200