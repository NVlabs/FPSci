// SocketTesting.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <enet/enet.h>
#include <iostream>
#include <string>
#include <G3D/G3D.h>
#include <Windows.h>

#define PORT 6624
#define FLOOD_TEST true

int run_flood_test() {
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
        int socket_n = 0;
        int peer_n = 0;
        int socket_l = 0;
        int peer_l = 0;
        bool running = false;
        // SOCKETS STUFF
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
        enet_socket_set_option(server_socket, ENET_SOCKOPT_NONBLOCK, 1);

        // PEER STUFF

        ENetAddress address_peer;
        address_peer.host = ENET_HOST_ANY;
        address_peer.port = PORT + 1;
        ENetHost* serverHost = enet_host_create(&address_peer, 32, 2, 0, 0);
        if (serverHost == NULL) {
            throw std::runtime_error("Could not create a local host for the clients to connect to");
        }

        // infinite loop - try to recieve incoming packets from socket
        while (1) {
            ENetBuffer buff; // make ENet buffer
            char bufferString[40];  // create data holder
            buff.data = &bufferString;  // assign to ENet buffer
            buff.dataLength = 40;

            ENetAddress addr_from;
            // receive data from socket
            while (enet_socket_receive(server_socket, &addr_from, &buff, 1) > 0) {  // if we received data...
                socket_n++;
                //std::string message((char*)buff.data, status); // convert to std::string
                //printf("recieved packet (%i bytes): %s\n", status, message.c_str()); // print the message
            }

            ENetEvent event;
            while (enet_host_service(serverHost, &event, 0) > 0)
            {
                switch (event.type)
                {
                case ENET_EVENT_TYPE_CONNECT:
                    socket_n = 0;
                    peer_n = 0;
                    socket_l = 0;
                    peer_l = 0;
                    running = true;
                    break;
                case ENET_EVENT_TYPE_RECEIVE:
                    peer_n++;
                    enet_packet_destroy(event.packet);
                    break;
                }
            }
            if (running) {
                Sleep(1);
                printf("Socket: %i\t\tPeer:%i\t\tSocket_l: %i\t\tPeer_l: %i\r", socket_n, peer_n, socket_l, peer_l);
                fflush(stdout);
            }
        }
    }
    //=================================================================================================================================================
    else {
        //CLIENT
        std::cout << "=== client mode ===\n";

        ENetSocket client_socket = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM); // create UDP socket
        enet_socket_set_option(client_socket, ENET_SOCKOPT_NONBLOCK, 1);

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

        // PEER
        ENetAddress localAddress;
        ENetHost* localHost = enet_host_create(NULL, 1, 2, 0, 0);
        if (localHost == NULL)
        {
            throw std::runtime_error("Could not create a local host for the server to connect to");
        }
        ENetAddress serverAddress;
        serverAddress.port = PORT + 1;
        enet_address_set_host(&serverAddress, address_input.c_str());
        ENetPeer* serverPeer = enet_host_connect(localHost, &serverAddress, 2, 0);

        //**//
        ENetBuffer enet_buff; // buffer object for data to send

        while (1) {
            //std::cout << "input a message to send (max 40 char): ";

            // prompt user for string to send
            //std::string message_input;
            //std::getline(std::cin, message_input);
            enet_buff.data = (void*)"test";// (void*)message_input.c_str();
            enet_buff.dataLength = 5;//message_input.length();// data_length;

            int status;
            status = enet_socket_send(client_socket, &address, &enet_buff, 1); // send the buffer to the address via the socket.
            if (status < 0) { // status here is the number of bytes sent
                printf("Send failed (status: %i)\n", status);
                printf("socket_send failed with error: %d\n", WSAGetLastError());
            }
            ENetPacket* packet = enet_packet_create("test", 4 + 1, ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(serverPeer, 0, packet);
            ENetEvent event;
            enet_host_service(localHost, &event, 0);
            //Sleep(1);
        }
    }
}

int run_socket_test()
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

int main()
{
    if (FLOOD_TEST) {
        run_flood_test();
    }
    else {
        run_socket_test();
    }

}