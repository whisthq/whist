from .utils import *
from app import engine
from .logger import *

def S3Upload(content, bucket, s3_file, ID = -1):
    s3 = boto3.resource(
        's3',
        region_name='us-east-1',
        aws_access_key_id = os.getenv('AWS_ACCESS_KEY'),
        aws_secret_access_key = os.getenv('AWS_SECRET_KEY')
    )
    
    
    try:
        s3.Object(bucket, s3_file).put(Body=content, ACL='public-read', ContentType='text/plain')
        return 1
    except Exception as e:
        sendCritical(ID, str(e))
        return -1

# def SendLogs(sender, connection_id, logs, vm_ip):
# 	send
# 	if sender.upper() == 'CLIENT':
# 		ip = vm_ip
# 		command = text("""
# 			SELECT * FROM v_ms WHERE "ip" = :ip
# 			""")
# 		params = {'ip': ip}

# 		with engine.connect() as conn:
# 			vm = cleanFetchedSQL(conn.execute(command, **params).fetchone())
# 			username = vm['username']
# 			title = '[{}] Logs Received From Connection #{}'.format(sender.upper(), str(connection_id))

# 			command = text("""
# 				SELECT * FROM logs WHERE "connection_id" = :connection_id
# 				""")
# 			params = {'connection_id': connection_id}
# 			logs_found = cleanFetchedSQL(conn.execute(command, **params).fetchall())
# 			last_updated = getCurrentTime() 

# 			if logs_found:
# 				command = text("""
# 					UPDATE logs 
# 					SET "ip" = :ip, "last_updated" = :last_updated, "client_logs" = :logs, "username" = :username
# 					WHERE "connection_id" = :connection_id
# 					""")
# 				params = {'username': username, 'ip': ip, 'last_updated': last_updated, 'logs': logs, 'connection_id': connection_id}
# 				conn.execute(command, **params)
# 			else:
# 				command = text("""
# 					INSERT INTO logs("username", "ip", "last_updated", "client_logs", "connection_id") 
# 					VALUES(:username, :ip, :last_updated, :logs, :connection_id)
# 					""")

# 				params = {'username': username, 'ip': ip, 'last_updated': last_updated, 'logs': logs, 'connection_id': connection_id}
# 				conn.execute(command, **params)

# 			conn.close()
# 		return {'status': 200}
# 	elif sender.upper() == 'SERVER':
# 		command = text("""
# 			SELECT * FROM logs WHERE "connection_id" = :connection_id
# 			""")
# 		params = {'connection_id': connection_id}
# 		with engine.connect() as conn:
# 			logs_found = cleanFetchedSQL(conn.execute(command, **params).fetchall())
# 			last_updated = getCurrentTime() 

# 			connection = cleanFetchedSQL(conn.execute(command, **params).fetchone())   
# 			if connection:
# 				command = text("""
# 					UPDATE logs 
# 					SET "last_updated" = :last_updated, "server_logs" = :logs
# 					WHERE "connection_id" = :connection_id
# 					""")
# 				params = {'last_updated': last_updated, 'logs': logs, 'connection_id': connection_id}
# 				conn.execute(command, **params)      
# 			else:
# 				command = text("""
# 					INSERT INTO logs("last_updated", "server_logs", "connection_id") 
# 					VALUES(:last_updated, :logs, :connection_id)
# 					""")

# 				params = {'last_updated': last_updated, 'logs': logs, 'connection_id': connection_id}
# 				conn.execute(command, **params)
# 		return {'status': 200}
# 	return {'status': 422}