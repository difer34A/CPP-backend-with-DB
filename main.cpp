#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include <cstring> // Add this line to include <cstring> for std::strlen

std::string getRequestMethod(const std::string& request) {
    std::string method;
    size_t methodEnd = request.find(' ');
    if (methodEnd != std::string::npos) {
        method = request.substr(0, methodEnd);
    }
    return method;
}

std::string getRequestPath(const std::string& request) {
    std::string path;
    size_t pathStart = request.find(' ');
    if (pathStart != std::string::npos) {
        pathStart++;
        size_t pathEnd = request.find(' ', pathStart);
        if (pathEnd != std::string::npos) {
            path = request.substr(pathStart, pathEnd - pathStart);
        }
    }
    return path;
}

std::string extractRequestBody(const std::string& request) {
    std::string requestBody;
    size_t bodyStart = request.find("\r\n\r\n");
    if (bodyStart != std::string::npos) {
        bodyStart += 4;  // Move past the "\r\n\r\n" delimiter
        requestBody = request.substr(bodyStart);
    }
    return requestBody;
}

std::string generateResponse(const std::string& content) {
    std::string response;
    response += "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/plain\r\n";
    response += "Content-Length: " + std::to_string(content.size()) + "\r\n";
    response += "\r\n";
    response += content;
    return response;
}

struct Record {
    int id;
    std::string name;
};
std::vector<Record> database;

void createRecord(const Record& record) {
    database.push_back(record);
}

Record readRecord(int id) {
    for (const auto& record : database) {
        if (record.id == id) {
            return record;
        }
    }
    // Return a default-constructed record if not found
    return Record();
}

void updateRecord(int id, const Record& newData) {
    for (auto& record : database) {
        if (record.id == id) {
            record = newData;
            break;
        }
    }
}
std::string seeAll() {
    std::string allRecords;
    for (const auto& record : database) {
        allRecords += "ID: " + std::to_string(record.id) + ", Name: " + record.name + "\n";
    }
    return allRecords;
}
void put(int id, const std::string& name) {
    Record newRecord;
    newRecord.id = id;
    newRecord.name = name;
    // Set other fields if needed

    createRecord(newRecord);
    std::cout << "Record added successfully." << std::endl;
}

void deleteRecord(int id) {
    auto it = std::remove_if(database.begin(), database.end(),
                            [id](const Record& record) { return record.id == id; });

    if (it != database.end()) {
        database.erase(it, database.end());
        std::cout << "Record deleted successfully." << std::endl;
    } else {
        std::cout << "Record not found." << std::endl;
    }
}


int main() {
    int serverSocket, newSocket;
    int port = 8080;  // Change this to the desired port number
    // Sample data
    Record record1;
    record1.id = 1;
    record1.name = "John Doe";
    database.push_back(record1);

    Record record2;
    record2.id = 2;
    record2.name = "Jane Smith";
    database.push_back(record2);

    // Create a socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Failed to create socket." << std::endl;
        return 1;
    }

    // Bind the socket to localhost and the specified port
    struct sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        std::cerr << "Failed to bind socket." << std::endl;
        return 1;
    }

    // Start listening for incoming connections
    if (listen(serverSocket, 3) < 0) {
        std::cerr << "Failed to listen." << std::endl;
        return 1;
    }

    std::cout << "Server listening on port " << port << std::endl;

    while (true) {
        // Accept incoming connections
        struct sockaddr_in clientAddress{};
        int clientAddressSize = sizeof(clientAddress);

        newSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, (socklen_t*)&clientAddressSize);
        if (newSocket < 0) {
            std::cerr << "Failed to accept connection." << std::endl;
            return 1;
        }

        std::cout << "Connection accepted." << std::endl;

        // Receive and parse the incoming request
        const int bufferSize = 4096;  // Adjust buffer size as needed
        char buffer[bufferSize];

        int bytesRead = read(newSocket, buffer, bufferSize - 1);
        if (bytesRead < 0) {
            std::cerr << "Failed to read data from socket." << std::endl;
            return 1;
        }
        buffer[bytesRead] = '\0';  // Null-terminate the buffer

        std::string request(buffer);
        std::string method = getRequestMethod(request);
        std::string path = getRequestPath(request);
        std::string requestBody = extractRequestBody(request);

        // Handle the request based on the method
        std::string responseBody;
        if (method == "GET") {
            std::cout << "Received GET request for path: " << path << std::endl;
            // Handle GET request logic and set the response body
            responseBody = seeAll();
        } else if (method == "POST") {
            // Extract id and name from the request body
            int id = 0;  // Default value, modify as needed
            std::string name;  // Default value, modify as needed

            // Parse the requestBody and extract the id and name fields
            size_t idStart = requestBody.find("\"id\":");
            size_t idEnd = requestBody.find(",", idStart);
            if (idStart != std::string::npos && idEnd != std::string::npos) {
                idStart += 6; // Move past "\"id\":"
                std::string idString = requestBody.substr(idStart, idEnd - idStart);
                id = std::stoi(idString);
            }

            size_t nameStart = requestBody.find("\"name\":");
            size_t nameEnd = requestBody.find("}", nameStart);
            if (nameStart != std::string::npos && nameEnd != std::string::npos) {
                nameStart += 8; // Move past "\"name\":"
                std::string nameString = requestBody.substr(nameStart, nameEnd - nameStart);
                // Remove surrounding quotes if present
                if (nameString.front() == '\"' && nameString.back() == '\"') {
                    nameString = nameString.substr(1, nameString.size() - 2);
                }
                name = nameString;
            }

            put(id, name); // Call the put function with the extracted id and name
            responseBody = "Record added successfully!";

        } else {
            std::cout << "Received unsupported request method: " << method << std::endl;
            responseBody = "Unsupported request method.";
        }

        // Generate the HTTP response
        std::string response = generateResponse(responseBody);

        // Send the response to the client
        if (send(newSocket, response.c_str(), response.size(), 0) < 0) {
            std::cerr << "Failed to send response." << std::endl;
            return 1;
        }

        // Close the current connection socket
        close(newSocket);
    }

    // Close the server socket (this won't be reached in this example)
    close(serverSocket);

    return 0;
}

