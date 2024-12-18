import socket
import datetime

HOST = "127.0.0.1"
PORT = 5000
LOG_FILE = "server_log.txt"

def start_server():
    
    # Create a socket object
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_socket:
        # Allow the socket to reuse the address if it was recently closed
        server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        # Bind the socket to the specified host and port
        server_socket.bind((HOST, PORT))

        # Start listening for incoming connections (backlog of 5)
        server_socket.listen(5)
        print(f"Server listening on {HOST}:{PORT}...")
        with open(LOG_FILE, "a") as log_file:
            while True:
                # Accept a new connection
                client_socket, client_address = server_socket.accept()
                print(f"Connection established with {client_address}")

                with client_socket:
                    while True:
                        # Receive data in chunks of 256 bytes
                        data = client_socket.recv(256)
                        if not data:
                            break  # Break if the client disconnects

                        message = data.decode('utf-8')
                        print(f"Received message: {message}")

                        # Log the message with a timestamp
                        timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                        log_file.write(f"[{timestamp}] {client_address}: {message}\n")
                        print(f"Writing log to: {LOG_FILE}")
                        log_file.flush()

                        # Optionally send an acknowledgment back to the client
                        client_socket.sendall(b"Message received")


if __name__ == "__main__":
    start_server()
