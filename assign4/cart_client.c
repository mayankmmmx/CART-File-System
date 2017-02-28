////////////////////////////////////////////////////////////////////////////////
//
//  File          : cart_client.c
//  Description   : This is the client side of the CART communication protocol.
//
//   Author       : ????
//  Last Modified : ????
//

// Include Files
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

// Project Include Files
#include <cart_network.h>
#include <cart_controller.h>
#include <cmpsc311_util.h> 

//
//  Global data
int client_socket = -1;
int                cart_network_shutdown = 0;   // Flag indicating shutdown
unsigned char     *cart_network_address = NULL; // Address of CART server
unsigned short     cart_network_port = 0;       // Port of CART server
unsigned long      CartControllerLLevel = 0; // Controller log level (global)
unsigned long      CartDriverLLevel = 0;     // Driver log level (global)
unsigned long      CartSimulatorLLevel = 0;  // Driver log level (global)

//
// Functions

////////////////////////////////////////////////////////////////////////////////
//
// Function     : client_cart_bus_request
// Description  : This the client operation that sends a request to the CART
//                server process.   It will:
//
//                1) if INIT make a connection to the server
//                2) send any request to the server, returning results
//                3) if CLOSE, will close the connection
//
// Inputs       : reg - the request reqisters for the command
//                buf - the block to be read/written from (READ/WRITE)
// Outputs      : the response structure encoded as needed

CartXferRegister client_cart_bus_request(CartXferRegister reg, void *buf) {
    struct sockaddr_in caddr; //socket info
    char *char_buf = (char *) buf; //converts void buffer into character buffer
    char *read_in = (char *) malloc(sizeof(char) * CART_FRAME_SIZE); //buffer to store data copied from network
    CartXferRegister opcode; //stores opcode from reg
    CartXferRegister converted_reg; //stores reg converted to network byte addressing
    CartXferRegister response; //stores network response
    cart_network_address = (cart_network_address == NULL) ? (unsigned char *) CART_DEFAULT_IP : cart_network_address; //cmd line ip or default
    cart_network_port = (cart_network_port == 0) ? (unsigned short) CART_DEFAULT_PORT : cart_network_port; //cmd line port or default

    if(client_socket == -1) { //if socket connection had not been opened
        caddr.sin_family = AF_INET; //sin family
        caddr.sin_port = htons(cart_network_port); //convert port to network bytes
        if(inet_aton((char *)cart_network_address, &caddr.sin_addr) == 0) { return(-1); } //generate connection address

        client_socket = socket(PF_INET, SOCK_STREAM, 0); //create the socket
        if(client_socket == -1) { return(-1); } //if creating socket fails, fail

        if (connect(client_socket, (const struct sockaddr *)&caddr, sizeof(caddr)) == -1 ) { return(-1); } //connect to socket
    }
   
    opcode = reg >> 56; //get the opcode from the reg
    converted_reg = htonll64(reg); //convert the reg into network bytes
    if(write(client_socket, &converted_reg, sizeof(converted_reg)) != sizeof(converted_reg)) { return(-1); } //write reg to network or fail

    switch(opcode) { //determine opcode instruction and execute specific instructions for different opcodes
        case CART_OP_RDFRME: //if read frame
            if(read(client_socket, &response, sizeof(response)) != sizeof(response)) { return(-1); } //read response from network
            response = ntohll64(response); //convert response to host bytes
            
            if(read(client_socket, read_in, CART_FRAME_SIZE) != CART_FRAME_SIZE) { return(-1); } //read buffer from network
            memcpy(char_buf, read_in, CART_FRAME_SIZE); //copy network buffer into char buf
            break;
        
        case CART_OP_WRFRME: //if write frame
            if(write(client_socket, char_buf, CART_FRAME_SIZE) != CART_FRAME_SIZE) { return(-1); } //write the buffer to the network 
            if(read(client_socket, &response, sizeof(response)) != sizeof(response)) { return(-1); } //read response from network
            response = ntohll64(response); //convert response to host bytes
            break;

        case CART_OP_POWOFF: //if power off
            if(read(client_socket, &response, sizeof(response)) != sizeof(response)) { return(-1); } //read response from network
            response = ntohl(response); //convert response to host bytes
            close(client_socket); //close the socket
            break;

        default: // any other instructions
            if(read(client_socket, &response, sizeof(response)) != sizeof(response)) { return(-1); } //read response from network
            response = ntohll64(response); // convert response to host bytes
            break;
    }

    return response; //return response
}
