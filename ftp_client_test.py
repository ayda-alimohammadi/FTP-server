import socket
import os
import time

SERVER_HOST = "localhost"
SERVER_PORT = 2121
BUFFER_SIZE = 1024
client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client_socket.connect((SERVER_HOST, SERVER_PORT))

welcome_message = client_socket.recv(BUFFER_SIZE).decode()  # دریافت پیام خوش‌آمدگویی
print("Server response:", welcome_message)

while True:
    command = input("Enter command (UPLOAD <filename>/DOWNLOAD <filename>/LIST/DELETE <filename>/SEARCH <filename>/QUIT): ").strip()
    client_socket.send(command.encode())

    if command.startswith("UPLOAD"):
        filename = command.split(" ", 1)[1]
        if os.path.exists(filename):
            with open(filename, "rb") as file:
                while (chunk := file.read(BUFFER_SIZE)):
                    client_socket.send(chunk)
            client_socket.send(b"END")
            time.sleep(0.1)
        else:
            print(f"File {filename} not found.")
            continue

        # Receive server confirmation after upload ends
        response = client_socket.recv(BUFFER_SIZE).decode()
        print("Server response:", response)

    elif command.startswith("DOWNLOAD"):
        filename = command.split(" ", 1)[1]
        response = client_socket.recv(BUFFER_SIZE).decode()

        if response == "OK":
            with open(filename, "wb") as file:
                while True:
                    chunk = client_socket.recv(BUFFER_SIZE)
                    if chunk == b"END":
                        break
                    file.write(chunk)
            print(f"{filename} downloaded successfully.")
        else:
            print("Server response:", response)

    elif command.startswith("DELETE"):
        filename = command.split(" ", 1)[1]
        response = client_socket.recv(BUFFER_SIZE).decode()
        print("Server response:", response)

    elif command.startswith("SEARCH"):
        filename = command.split(" ", 1)[1]
        response = client_socket.recv(BUFFER_SIZE).decode()
        print("Server response:", response)

    elif command == "LIST" or command == "QUIT":
        response = client_socket.recv(BUFFER_SIZE).decode()
        print("Server response:", response)
        if command == "QUIT":
            break

    else:
        print("Invalid command. Please try again.")

client_socket.close()

