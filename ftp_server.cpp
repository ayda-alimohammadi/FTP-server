#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <cstring>

#define PORT 2121
#define BUFFER_SIZE 1024
#define BASE_DIR "ftp_files"

// بررسی و ایجاد دایرکتوری
void createBaseDir() {
    if (!std::filesystem::exists(BASE_DIR)) {
        std::filesystem::create_directory(BASE_DIR);
    }
}

void handleUpload(int client_socket, const std::string &filepath) {
    std::string filename = filepath.substr(filepath.find_last_of("/\\") + 1);
    std::string file_path = std::string(BASE_DIR) + "/" + filename;

    // بررسی و ایجاد نام منحصربه‌فرد برای فایل
    int count = 1;
    while (std::filesystem::exists(file_path)) {
        file_path = std::string(BASE_DIR) + "/" + filename + "(" + std::to_string(count) + ")";
        count++;
    }

    std::ofstream outfile(file_path, std::ios::binary);
    if (!outfile.is_open()) {
        send(client_socket, "Failed to open file for writing\n", 31, 0);
        return;
    }

    char buffer[BUFFER_SIZE];
    int bytes_received;
    while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        // بررسی برای دریافت کامل فایل
        if (std::string(buffer, bytes_received) == "END") {
            std::cout << "End of file received." << std::endl;
            break;
        }
        outfile.write(buffer, bytes_received);
        if (std::string(buffer, bytes_received).find("END") != std::string::npos) {
    		std::cout << "End of file received." << std::endl;
    		break;
}

        std::cout << "Bytes received and written to file: " << bytes_received << std::endl;
    }

    outfile.close();
    send(client_socket, "Upload complete\n", 16, 0);
}


void handleDownload(int client_socket, const std::string &filename) {
    // باز کردن فایل برای خواندن
    std::ifstream infile(std::string(BASE_DIR) + "/" + filename, std::ios::binary);
    
    // اگر فایل باز نشد، پیام File not found را ارسال کن
    if (!infile.is_open()) {
        send(client_socket, "File not found\n", 15, 0);
        return;
    }

    // ارسال پیام تایید به کلاینت که فایل آماده دانلود است
    send(client_socket, "OK", 2, 0);

    char buffer[BUFFER_SIZE];
    
    // خواندن و ارسال فایل به صورت بلوک‌های BUFFER_SIZE بایتی
    while (infile.read(buffer, BUFFER_SIZE)) {
        send(client_socket, buffer, infile.gcount(), 0);
    }

    // ارسال باقی‌مانده فایل در صورتی که کمتر از BUFFER_SIZE باشد
    if (infile.gcount() > 0) {
        send(client_socket, buffer, infile.gcount(), 0);
    }
    
    // ارسال علامت پایان
    send(client_socket, "END", 3, 0);

    // بستن فایل
    infile.close();
}



void handleDelete(int client_socket, const std::string &filename) {
    if (std::filesystem::remove(std::string(BASE_DIR) + "/" + filename)) {
        send(client_socket, "File deleted\n", 13, 0);
    } else {
        send(client_socket, "File not found\n", 15, 0);
    }
}

void handleSearch(int client_socket, const std::string &filename) {
    if (std::filesystem::exists(std::string(BASE_DIR) + "/" + filename)) {
        send(client_socket, "File found\n", 11, 0);
    } else {
        send(client_socket, "File not found\n", 15, 0);
    }
}

void handleList(int client_socket) {
    std::string file_list;
    for (const auto &entry : std::filesystem::directory_iterator(BASE_DIR)) {
        file_list += entry.path().filename().string() + "\n";
    }
    send(client_socket, file_list.c_str(), file_list.size(), 0);
}

void handleClient(int client_socket) {
    send(client_socket, "220 Welcome to the FTP server\n", 29, 0);


    char buffer[BUFFER_SIZE];
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            break;
        }
         std::cout << "Raw data received: " << std::string(buffer, bytes_received) << std::endl;
         

        std::string command(buffer);
        std::string filename;
        std::cout << "Received command:" << command << std::endl;
        
        if (command.rfind("UPLOAD", 0) == 0) {
            filename = command.substr(7);
            std::cout << "Upload command recived for file::" << filename << std::endl;
            handleUpload(client_socket, filename);
        } else if (command.rfind("DOWNLOAD", 0) == 0) {
            filename = command.substr(9);
            std::cout << "Download command recived for file::" << filename << std::endl;
            handleDownload(client_socket, filename);
        } else if (command.rfind("DELETE", 0) == 0) {
            filename = command.substr(7);
            std::cout << "Delete command recived for file::" << filename << std::endl;
            handleDelete(client_socket, filename);
        } else if (command.rfind("SEARCH", 0) == 0) {
            filename = command.substr(7);
            std::cout << "Search command recived for file::" << filename << std::endl;
            handleSearch(client_socket, filename);
        } else if (command == "LIST") {
            std::cout << "List command recived.\n :";        
            handleList(client_socket);
        } else if (command == "QUIT") {
            std::cout << "Quit command reciveded";
            send(client_socket, "Goodbye\n", 8, 0);
            close(client_socket);
            break;
        } else {
            send(client_socket, "Invalid command\n", 16, 0);
        }
    }
    
}

int main() {
    createBaseDir();

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        std::cerr << "Socket creation failed\n";
        return 1;
    }

    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    if (bind(server_socket, (sockaddr *)&server_address, sizeof(server_address)) < 0) {
        std::cerr << "Bind failed\n";
        return 1;
    }

    if (listen(server_socket, 5) < 0) {
        std::cerr << "Listen failed\n";
        return 1;
    }

    std::cout << "FTP server is listening on port " << PORT << "...\n";

    while (true) {
        int client_socket = accept(server_socket, nullptr, nullptr);
        if (client_socket < 0) {
            std::cerr << "Client connection failed\n";
            continue;
        }

        std::cout << "Client connected\n";

        handleClient(client_socket);
    }


    return 0;
}

