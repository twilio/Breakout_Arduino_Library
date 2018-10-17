# OwlModem  - a more elaborate, yet still simple modem interface.

## [`OwlModem`](OwlModem.h)

Please check the documented header file for a description of the exported functionality.


## Handlers

  This section seeks to list and document all handler function which are exported by the OwlModem. These are simple 
  `C` functions, hence no need to implement a complex C++ class to use them.

  Note: Not all handlers are useful for the application. Simply ignore those that you don't care about and pass a 
  0 (void* NULL) if a parameter must be provided to some functions.


### [`OwlModemNetwork`](OwlModemNetwork.h) 

```C
/**
 * Handler function signature for Network Registration events (CS)
 * @param stat - status
 * @param lac - local area code
 * @param ci - cell id
 * @param act - access technology
 */
void your_function_NetworkRegistrationStatusChangeHandler(at_creg_stat_e stat, uint16_t lac, uint32_t ci, 
                                                          at_creg_act_e act) {
  // Your code goes here
}

// Register with
  owlModem->network.setHandlerNetworkRegistrationURC(your_function_NetworkRegistrationStatusChangeHandler);



/**
 * Handler function signature for GPRS Registration events (2G/3G)
 * @param stat - status
 * @param lac - local area code
 * @param ci - cell id
 * @param act - access technology
 * @param rac - routing area code
 */
void your_function_GPRSRegistrationStatusChangeHandler(at_cgreg_stat_e stat, uint16_t lac, uint32_t ci,
                                                       at_cgreg_act_e act, uint8_t rac) {
  // Your code goes here
}

// Register with
  owlModem->network.setHandlerGPRSRegistrationURC(your_function_GPRSRegistrationStatusChangeHandler);



/**
 * Handler function signature for EPS Registration events (EPC - 2G/3G/LTE/WiFi/etc)
 * @param stat - status
 * @param lac - local area code
 * @param ci - cell id
 * @param act - access technology
 * @param cause_type
 * @param reject_cause
 */
void your_function_EPSRegistrationStatusChangeHandler(at_cereg_stat_e stat, uint16_t lac, uint32_t ci,
                                                      at_cereg_act_e act, at_cereg_cause_type_e cause_type,
                                                      uint32_t reject_cause) {
  // Your code goes here
}

// Register with
  owlModem->network.setHandlerEPSRegistrationURC(your_function_EPSRegistrationStatusChangeHandler);

```




### [`OwlModemSIM`](OwlModemSIM.h)

```C
/**
 * Handler function signature for SIM card not ready - use this to verify the PIN with the card.
 * @param message - the last message regarding PIN from the card
 */
void your_function_PINHandler(str message) {
  // Your code goes here
}

// Register with
  owlModem->SIM.setHandlerPIN(your_function_PINHandler);
```





### [`OwlModemSocket`](OwlModemSocket.h)

```C
/**
 * Handler for UDP data
 * @param socket - socket the data was received on
 * @param remote_ip - where the data came from - in case the socket was connected, this might be missing
 * @param remote_port - the remote port where the data came from
 * @param data - the binary data
 */
void your_function_UDPDataHandler(uint8_t socket, str remote_ip, uint16_t remote_port, str data) {
  // Your code goes here
}

// Register with either one from below
  int result = owlModem->socket.listenUDP(socket, local_port, your_function_UDPDataHandler);
  int result = owlModem->socket.openListenUDP(local_port, your_function_UDPDataHandler, &new_socket);
  int result = owlModem->socket.openConnectUDP(local_port, remote_ip, remote_port, your_function_UDPDataHandler, &new_socket);
  int result = owlModem->socket.openListenConnectUDP(local_port, remote_ip, remote_port, your_function_UDPDataHandler, &new_socket);


/**
 * Handler for TCP data
 * @param socket - the socket the data was received on
 * @param data - the binary data
 */
void your_function_TCPDataHandler(uint8_t socket, str data) {
  // Your code goes here
}

// Register with either one from below
  int result = owlModem->socket.listenTCP(socket, local_port, your_function_TCPDataHandler);
  int result = owlModem->socket.acceptTCP(socket, local_port, your_function_TCPAcceptHandler, your_function_SocketClosedHandler, your_function_TCPDataHandler);
  int result = owlModem->socket.openListenConnectTCP(local_port, remote_ip, remote_port, your_function_SocketClosedHandler, your_function_TCPDataHandler, &new_socket);
  int result = owlModem->socket.openAcceptTCP(local_port, your_function_TCPAcceptHandler, your_function_SocketClosedHandler, your_function_TCPDataHandler, &new_socket);


/**
 * Handler for TCP new connection accept
 * @param new_socket - the new socket created after accepting it on the listening_socket
 * @param remote_ip - the remote IP address from where this connection was done
 * @param remote_port - the remote port from where this connection was done
 * @param listening_socket - the listening socket where this new connection was accepted
 * @param local_ip - the local IP where this connection was accepted
 * @param local_port - the local port of this connection
 */
void your_function_TCPAcceptHandler(uint8_t new_socket, str remote_ip, uint16_t remote_port, uint8_t listening_socket, str local_ip, uint16_t local_port) {
  // Your code goes here
}

// Register with either one from below
  int result = owlModem->socket.acceptTCP(socket, local_port, your_function_TCPAcceptHandler, your_function_SocketClosedHandler, your_function_TCPDataHandler);
  int result = owlModem->socket.openAcceptTCP(local_port, your_function_TCPAcceptHandler, your_function_SocketClosedHandler, your_function_TCPDataHandler, &new_socket);


/**
 * Handler for TCP socket closed event
 * @param socket - the socket which was closed
 */
void your_function_SocketClosedHandler(uint8_t socket) {
  // Your code goes here
}

// Register with either one from below
  int result = owlModem->socket.connect(socket, remote_ip, remote_port, your_function_SocketClosedHandler);
  int result = owlModem->socket.acceptTCP(socket, local_port, your_function_TCPAcceptHandler, your_function_SocketClosedHandler, your_function_TCPDataHandler);
  int result = owlModem->socket.openListenConnectTCP(local_port, remote_ip, remote_port, your_function_SocketClosedHandler, your_function_TCPDataHandler, &new_socket);
  int result = owlModem->socket.openAcceptTCP(local_port, your_function_TCPAcceptHandler, your_function_SocketClosedHandler, your_function_TCPDataHandler, &new_socket);

```