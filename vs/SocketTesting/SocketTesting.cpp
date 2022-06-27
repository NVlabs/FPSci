// SocketTesting.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <enet/enet.h>
#include <iostream>
#include <string>

#define PORT 6624


int main()
{
    // Initialize ENET
    if (enet_initialize() != 0) {
        fprintf(stderr, "An error occured while initializing ENET\n"); // print an error and terminate if Enet does not initialize successfully
        return 1;
    }
    // poll for client (sending packets) or server (receiving packets)
    std::cout << "Client or server (C/s)?\n";
    std::string option_input;
    std::getline(std::cin, option_input);
    if (option_input.c_str()[0] == 's') {   // SERVER
        std::cout << "=== server mode ===\n";
        // create inbound address
        ENetAddress address;
        address.host = ENET_HOST_ANY;
        address.port = PORT;
        ENetSocket server_socket = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM); // make the UDP Socket
        int status = -132;
        status = enet_socket_bind(server_socket, &address); // bind socket to address
        if (status != 0) { // (print error if bind fails)
            std::cout << "Failed to bind to socket";
            printf("status: %i\n", status);
            printf("bind failed with error: %d\n", WSAGetLastError());
        }

        // infinite loop - try to recieve incoming packets from socket
        while (1) {
            ENetBuffer buff; // make ENet buffer
            char bufferString[40];  // create data holder
            buff.data = &bufferString;  // assign to ENet buffer
            buff.dataLength = 40;

            ENetAddress addr_from;
            status = enet_socket_receive(server_socket, &addr_from, &buff, 1);  // receive data from socket
            if (status >= 0) {  // if we received data...
                std::string message((char*)buff.data, status); // convert to std::string
                printf("recieved packet (%i bytes): %s\n", status, message.c_str()); // print the message
            }
            else {
                // no packet this tick
            }
        }
    }
    else {
        //CLIENT
        std::cout << "=== client mode ===\n";

        ENetSocket client_socket = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM); // create UDP socket

        ENetAddress address;    //declare address
        address.host = 0; // initialize to garbage data
        address.port = PORT;
        int address_status = -1;
        // prompt user for server address:
        std::string address_input;
        std::cout << "input host name or address to connect to: ";
        std::getline(std::cin, address_input);
        address_status = enet_address_set_host(&address, address_input.c_str()); // attempt to resolve input and assign to address object
        std::cout << address_input << "\n";
        while (address_status != 0) { // if the address was not assigned the input value, prompt again:
            printf("Failed to parse address: %i", address_status);
            std::getline(std::cin, address_input);
            address_status = enet_address_set_host(&address, address_input.c_str());
        }

        ENetBuffer enet_buff; // buffer object for data to send

        while (1) {
            std::cout << "input a message to send (max 40 char): ";

            // prompt user for string to send
            std::string message_input;
            std::getline(std::cin, message_input);
            enet_buff.data = (void*)message_input.c_str();
            enet_buff.dataLength = message_input.length();// data_length;

            int status;
            status = enet_socket_send(client_socket, &address, &enet_buff, 1); // send the buffer to the address via the socket.
            if (status < 0) { // status here is the number of bytes sent
                printf("Send failed (status: %i)\n", status);
                printf("socket_send failed with error: %d\n", WSAGetLastError());
            }
        }
    }
}