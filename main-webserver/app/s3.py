from .utils import *
from app import engine
from .logger import *

def SendLogsToS3(content, sender, connection_id, vm_ip, version, ID = -1):
	def S3Upload(content, last_updated, sender, ID):
		bucket = 'fractal-protocol-logs'

		file_name = '{}-{}'.format(sender, last_updated.replace('/','').replace(', ','-').replace(':', ''))
		file_name = ''.join(e for e in file_name if e.isalnum())
		file_name = file_name + '.txt'

		s3 = boto3.resource(
			's3',
			region_name='us-east-1',
			aws_access_key_id = os.getenv('AWS_ACCESS_KEY'),
			aws_secret_access_key = os.getenv('AWS_SECRET_KEY')
		)
		
		try:
			s3.Object(bucket, file_name).put(Body=content, ACL='public-read', ContentType='text/plain')
			url = 'https://fractal-protocol-logs.s3.amazonaws.com/{}'.format(file_name)
			return url
		except Exception as e:
			print(str(e))
			return None

	sender = sender.upper()
	last_updated = getCurrentTime() 
	username = None

	with engine.connect() as conn:
		command = text("""
			SELECT * FROM v_ms WHERE "ip" = :ip
			""")
		params = {'ip': vm_ip}

		vm = cleanFetchedSQL(conn.execute(command, **params).fetchone())
		if vm:
			username = vm['username']

		file_name = S3Upload(content, last_updated, sender, ID)
		file_name = file_name if file_name else ''

		if sender == 'CLIENT':
			command = text("""
				SELECT * FROM logs WHERE "connection_id" = :connection_id
				""")

			params = {'connection_id': connection_id}
			logs_found = cleanFetchedSQL(conn.execute(command, **params).fetchall())

			if logs_found:
				command = text("""
					UPDATE logs 
					SET "ip" = :ip, "last_updated" = :last_updated, "client_logs" = :file_name, "username" = :username
					WHERE "connection_id" = :connection_id
					""")
				params = {'username': username, 'ip': vm_ip, 
					'last_updated': last_updated, 'file_name': file_name, 
					'connection_id': connection_id}
				conn.execute(command, **params)
			else:
				command = text("""
					INSERT INTO logs("username", "ip", "last_updated", "client_logs", "connection_id") 
					VALUES(:username, :ip, :last_updated, :file_name, :connection_id)
					""")

				params = {'username': username, 'ip': vm_ip, 'last_updated': last_updated, 
					'file_name': file_name, 'connection_id': connection_id}
				conn.execute(command, **params)

		elif sender == 'SERVER':
			command = text("""
				SELECT * FROM logs WHERE "connection_id" = :connection_id
				""")
			params = {'connection_id': connection_id}
			logs_found = cleanFetchedSQL(conn.execute(command, **params).fetchall())

			if logs_found:
				command = text("""
					UPDATE logs 
					SET "last_updated" = :last_updated, "server_logs" = :file_name, "ip" = :vm_ip, "version" = :version
					WHERE "connection_id" = :connection_id
					""")
				params = {'last_updated': last_updated, 'file_name': file_name, 'connection_id': connection_id, 'vm_ip': vm_ip, 'version': version}
				conn.execute(command, **params)      
			else:
				command = text("""
					INSERT INTO logs("last_updated", "server_logs", "connection_id", "ip", "version") 
					VALUES(:last_updated, :file_name, :connection_id, :vm_ip, :version)
					""")

				params = {'last_updated': last_updated, 'file_name': file_name, 'connection_id': connection_id, 'vm_ip': vm_ip, 'version': version}
				conn.execute(command, **params)

		conn.close()
		return 1
	return -1