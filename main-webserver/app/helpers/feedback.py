from app.utils import *
from app import *
from app.logger import *


def storeFeedback(username, feedback):
    """Saves feedback in the feedback table

    Args:
        username (str): The username of the user who submitted feedback
        feedback (str): The feedback message
    """

    if feedback:
        command = text("""
            INSERT INTO feedback("username", "feedback")
            VALUES(:email, :feedback)
            """)
        params = {'email': username, 'feedback': feedback}
        with engine.connect() as conn:
            conn.execute(command, **params)
            conn.close()
