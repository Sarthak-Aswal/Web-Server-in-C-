#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "9090" // Port number
#define DEFAULT_BUFLEN 512

// Function to read the contents of a file into a string
std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Function to insert content into the <body> tag
std::string insertIntoBody(const std::string& htmlContent, const std::string& newBodyContent) {
    std::string result;
    std::string::size_type bodyStart = htmlContent.find("<body>");
    std::string::size_type bodyEnd = htmlContent.find("</body>", bodyStart);

    if (bodyStart != std::string::npos && bodyEnd != std::string::npos) {
        // Copy content before <body>
        result += htmlContent.substr(0, bodyEnd + 7);
        // Insert new content into <body>
        result += newBodyContent;
        // Copy content after </body>
        result += htmlContent.substr(bodyEnd + 7);
    } else {
        // If <body> tags are not found, just return the original content
        result = htmlContent;
    }

    return result;
}

int main() {
    WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo *result = NULL;
    struct addrinfo hints;

    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed with error: " << iResult << std::endl;
        return 1;
    }
    std::cout << "Winsock initialized\n";

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        std::cerr << "getaddrinfo failed with error: " << iResult << std::endl;
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for the server to listen for client connections
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        std::cerr << "socket failed with error: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }
    std::cout << "Socket created\n";

    // Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        std::cerr << "bind failed with error: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }
    std::cout << "Socket bound successfully\n";

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        std::cerr << "listen failed with error: " << WSAGetLastError() << std::endl;
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }
    std::cout << "Socket is listening on port 9090\n";

    // Accept a client socket
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        std::cerr << "accept failed with error: " << WSAGetLastError() << std::endl;
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }
    std::cout << "Client connected\n";

    // No longer need server socket
    closesocket(ListenSocket);

    // Read HTML and CSS files
    std::string htmlContent = readFile("index.html");
    std::string cssContent = readFile("style.css");

    if (htmlContent.empty() || cssContent.empty()) {
        std::cerr << "Error reading files. Please check file paths and contents." << std::endl;
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    // Create the body content
    std::stringstream bodyContentStream;
    bodyContentStream << "<style>" << cssContent << "</style>"
                      << "<h1>Welcome to the Test Page</h1>"
                      << "<p>This is a dynamically inserted content.</p>";

    std::string bodyContent = bodyContentStream.str();

    // Insert the new content into the <body> tag
    std::string finalHtmlContent = insertIntoBody(htmlContent, bodyContent);

    // Form the complete HTTP response
    std::stringstream responseStream;
    responseStream << "HTTP/1.1 200 OK\r\n"
                   << "Content-Type: text/html\r\n\r\n"
                   << finalHtmlContent;

    std::string response = responseStream.str();

    // Send the HTTP response
    iResult = send(ClientSocket, response.c_str(), response.length(), 0);
    if (iResult == SOCKET_ERROR) {
        std::cerr << "send failed with error: " << WSAGetLastError() << std::endl;
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Response sent successfully.\n";

    // Shutdown the connection since we're done
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        std::cerr << "shutdown failed with error: " << WSAGetLastError() << std::endl;
    }

    // Cleanup
    closesocket(ClientSocket);
    WSACleanup();

    return 0;
}
