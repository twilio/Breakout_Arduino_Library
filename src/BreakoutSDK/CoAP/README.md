# Mini CoAP Stack

This is a small implementation of a CoAP "stack". It's far from being complete but, after-all, this is the constrained
environment that CoAP should run in. So, it's constrained in scope too and does not need everything.

## Data Structures 

### [`CoAPOption`](CoAPOption.h) - Options for the CoAP message header

The CoAPOption is a simple class with direct access to the data: option number and option value. There are 4 value
types defined in the [RFC7252](https://tools.ietf.org/html/rfc7252#section-3.2): `empty`, `opaque`, `uint` and `string`.

The value is stored in an union, hence always check the value type before reading the value. The empty value has
practically no storage. `opaque` and `string` are similar. When concerned about null characters, use `opaque` instead of
`string`.

### [`CoAPMessage`](CoAPMessage.h) - Main CoAP data structure

This is not about CoAP, hence you might want to review a tutorial on it if you are not comfortable with the concepts, otherwise, in brief, a CoAP message has the following structure

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|Ver| T |  TKL  |      Code     |          Message ID           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|   Token (if any, TKL bytes) ...
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|   Options (if any) ...
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|1 1 1 1 1 1 1 1|    Payload (if any) ...
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

```

 The main data structure is represented by the [`CoAPMessage`](CoAPMessage.h) class. There are several ways to create 
 objects of it:

- When sending, create it with the following constructor:

```C
  CoAPMessage *request = new CoAPMessage(CoAP_Type__Confirmable, CoAP_Code_Class__Request, 
                                         CoAP_Code_Detail__Request__GET, coapPeer->getNextMessageId());
```

- When receiving, an object instance is passed as decoded in a handler. If you have to do this
  yourself then initialize the class with the empty constructor and then call the `decode()` option. This is
  safe to be called also later as it does a reset on the message instance.

```C
void your_function_MessageHandler(CoAPPeer *peer, CoAPMessage *message) {
  //...
  message.log(L_INFO);
  //...
}

  // Decoding example
  bin_t buffer = { .s = input_data_ptr, .idx = 0, .max = input_data_len };
  CoAPMessage *request = new CoAPMessage();
  if (request) {
    if (!request->decode(&bin_t_buffer)) LOG(L_ERROR, "Error decoding\r\n");
    request->log(L_INFO);
  }
```

- When replying, a short-cut is provided to create a follow-up message from the one received:

```C
  CoAPMessage *ack = new CoAPMessage(con_message, CoAP_Type__Acknowledgement);
```
  or
```C
  CoAPMessage *response = new CoAPMessage(request, CoAP_Type__Non_Confirmable, CoAP_Code_Class__Response, 
                                          CoAP_Code_Detail__Response__Content);
```

Adding options can be done through built-in methods. Two types of methods are available to manipulate options:

 - Directly, by using one of the known CoAP options. E.g.:

```C
  if (!message.addOptionUriHost("host")) LOG(L_ERROR, "Error adding UriHost option\r\n");
  if (!message.addOptionContentFormat(CoAP_Content_Format__application_json)) 
    LOG(L_ERROR, "Error adding ContentFormat option\r\n");
```
```C
  CoAPOption *i = 0;
  str proxyScheme;
  while (message->getNextOptionProxyScheme(&proxy_scheme, &i)
    LOG(L_INFO, "ProxyScheme is [%.*s]\r\n", proxy_scheme.len, proxy_scheme.s);
```

 - Indirectly, in case you need to add a new or proprietary option. Methods give access to the four defined data types: 
   `empty`, `opaque`, `uint`, and `string`.

```C
  CoAPOption *o;
  if (!(o = message.addOptionUint(42, 43))) LOG(L_ERROR, "Error adding option 42 with value 43\r\n");
```
```C
  CoAPOption *i = 0;
  uint64_t uint;
  while ((i = message->getNextOptionUint(42, &i)) ! = 0)
    LOG(L_INFO, "Option 42 is [%llu]\r\n", i->value.uint);
```

Setting the payload is very simple, but *do check the next note on memory and storage*:

```C
  message->payload.s = "Sample payload";
  message->payload.len = strlen(message->payload.s);
```


**Notes about memory and storage:** 
 - When decoding, the `CoAPMessage::decode(bin_t_buffer)` method builds a shallow structure in the sense that it does
   not duplicate any data from the original `bin_t_buffer`. This is faster and does not consume extra memory. However,
   the buffer must be available as long as you are processing the message. When handling a message, contents must be
   duplicated as they won't be available after returning from the handler.
 - Similarly for creating a message, neither the options (`opaque` or `string`) or the payload are duplicated.
   Hence they must be available as long as the CoAPMessge instance is in use. 
 - Freeing the instances only delets the lists used to reference the options (but not deep!) and resets values. The
   Option contents (for string, opaque) and the payload is not freed.




## [`CoAPPeer`](CoAPPeer.h) - Transport and mini-stack features

The `CoAPPeer` represents a connection to a single remote peer. Hence the remote IP and remote port parameters are
fixed.

Selecting the constructor also selects the transport:

```C
  str remote_ip = STRDECL("10.0.0.1");
  // Local port is dynamic, remote is default 5683.
  CoAPPeer *peerPlaintext = new CoAPPeer(owlModem, 0, remote_ip);

  str psk_id = STRDEF("username");
  str psk_key = STRDEF("password");
  // Local port is dynamic, remote is default 5684.
  CoAPPeer *peerDTLS = new CoAPPeer(owlModem, psk_id, psk_key, 0, remote_ip);
```


### Initialization

Next, as DTLS has to perform the handshake, but also for plaintext, several operations need to happen:

 - Step 0: Set first handlers, to make sure all the data will be received. Pay attention that the 
   `your_function_StatelessMessageHandler()` will be called for all incoming messages, even if when identified as
   retransmissions. The `your_function_RequestHandler()` and the `your_function_ResponseHandler()` will only be called
   for requests, respectively responses, and only once. This also means that each message will trigger at least one
   handler, but some good be triggering multiple ones. Not all all handlers must be provided, in case not all options
   are useful in the application.
```C
  peer->setHandlers(your_function_StatelessMessageHandler, your_function_DTLSEventHandler,
                    your_function_RequestHandler, your_function_ResponseHandler);
```

 - Step 1: Initialize the transport.
```C
  if (!peer->reinitializeTransport()) LOG(L_ERROR, "Error opening connection\r\n");
```

 - Step 2: Be patient until the DTLS handshake completes. It's not a good idea to spin here, as control shall be
   returned such that the incoming data will be received, retransmissions will be sent, etc. This, unfortunately
   entails that the design of the application would be more difficult with re-entrant functions, yet there is no other
   easy way around this, in the single-thread Arduino environment.
```C
  if (!peer->transportIsReady()) 
    LOG(L_INFO, "Do other things, like handling incoming data, until this is ready\r\n");
  else
    LOG(L_INFO, "Messages can be sent out now\r\n);
```


### Sending Messages

Once the `CoAPPeer` is initialize and the transport is ready, messages can be sent out in one of 2 manners:

 - Unreliably (for Non-confirmable messages (NON), Acknowledgements (ACK) and Resets (RST):
 
```C
  // Send once
  if (!peer->sendUnreliably(message_once)) LOG(L_ERROR, "Error sending NON/ACK/RST message\r\n");
  // Or send multiple times, with probing_rate = 2 bytes / second and a MAX_TRANSMIT_SPAN of 60 seconds
  if (!peer->sendUnreliably(message_repeated, 2, 60)) LOG(L_ERROR, "Error sending NON message\r\n"); 
```
**Note:** Non-confirmable messages (NON) do not have a corresponding ACK to stop the retransmissions. Hence in case the
retransmission is enabled, this will continue regardless of replies. This mechanism then should be used for messages
without a follow-up, which need a better reliability model than fire-once-and-forget.

 - Reliably:

```C
  // Send with default retransmission parameters
  if (!peer->sendReliably(message, your_function_ClientTrasactionCallback, your_param_ptr)) 
    LOG(L_ERROR, "Error sending CON message\r\n");
  // Send without a callback, but with custom retransmission parameters: MAX RETRANSMIT 3, MAX_TRANSMIT_SPAN 90 sec
  if (!peer->sendReliably(message, 0, 0, 3, 90)) 
    LOG(L_ERROR, "Error sending CON message\r\n");
```

   The Client Transaction callback allows the CoAP peer to notify the application when this particular message was 
   acknowledged, rejected, a timeout occured and delivery then seemingly failed, or your application canceled 
   retransmissions (by using the `peer->stopRetransmissions(message->message_id)` method).

```C
void your_function_ClientTransactionCallback(CoAPPeer *peer, coap_message_id_t message_id, void *cb_param,
                                             coap_client_transaction_event_e event, CoAPMessage *message) {
  switch (event) {
    case CoAP_Client_Transaction_Event__ACK:
      LOG(L_INFO, "Message delivered successfully\r\n");
      break;
    case CoAP_Client_Transaction_Event__RST:
      LOG(L_INFO, "Message rejected\r\n");
      break;
    case CoAP_Client_Transaction_Event__Timeout:
      LOG(L_INFO, "Message delivery failed\r\n");
      break;
    case CoAP_Client_Transaction_Event__Canceled:
      LOG(L_INFO, "Message retransmissions stopped\r\n");
      break;
    default:
      LOG(L_ERROR, "Not handled event %d\r\n", event);
  }
}
```


### Receiving Messages 

Receiving messages is done through one of the handler functions. 

The stateless message is probably of little use, other than for debugging purposes, as it is triggered also on 
retransmissions. 

```C
void your_function_RequestHandler(CoAPPeer *peer, CoAPMessage *message) {
  LOG(L_INFO, "Incoming message received\r\n");
  message->log(L_INFO);
}
```

The request and response handlers are the most important ones and they feature message deduplication, meaning that 
retransmissions are identified by `message_id` and filtered out (see the Server Transaction section).

```C
coap_handler_follow_up_e your_function_RequestHandler(CoAPPeer *peer, CoAPMessage *request) {
  LOG(L_INFO, "Received a request\r\n");
  request->log(L_INFO);
  switch (request->code_detail) {
    case CoAP_Code_Detail__Request__GET:
      LOG(L_INFO, "Handling GET\r\n);
      return CoAP__Handler_Followup__Send_Acknowledgement;
    case CoAP_Code_Detail__Request__POST:
      LOG(L_INFO, "Handling POST - will ack after we're fully finished\r\n");
      // Note that this won't be called again, as the Server Transactions will mask retransmisssions
      return CoAP__Handler_Followup__Do_Nothing;
    default:
      LOG(L_INFO, "Not handling other requests\r\n");
      return CoAP__Handler_Followup__Send_Reset;
  }
}

 // Sending the ACK later 
 peer->sendUnreliably(ack);
```

Returning a follow-up different than `CoAP__Handler_Followup__Do_Nothing` on NON messages is fine, as the `CoAPPeer` 
will only send ACK/RST in case the handled message is CON. When following up later, make sure that you save at least the
message_id, to be able to create the ACK or RST message; also always use for ACK and RST the `peer->sendUnreliably()`
method.

Handling responses is done similarly:

```C
coap_handler_follow_up_e your_function_ResponseHandler(CoAPPeer *peer, CoAPMessage *Response) {
  LOG(L_INFO, "Received a response\r\n");
  request->log(L_INFO);
  switch (request->code_detail) {
    case CoAP_Code_Detail__Error__Bad_Request:
      LOG(L_INFO, "We send something wrong\r\n);
      return CoAP__Handler_Followup__Send_Acknowledgement;
    case CoAP_Code_Detail__Server_Error__Internal_Server_Error:
      LOG(L_INFO, "The server messed up\r\n);
      return CoAP__Handler_Followup__Send_Acknowledgement;
    default:
      LOG(L_INFO, "Something else happened %d - %s\r\n", request->code_detail);
      CoAP__Handler_Followup__Do_Nothing;
  }
  return CoAP__Handler_Followup__Send_Reset;
}
```


### Client Transactions

Client transactions allow for retransmission of messages in one of 2 ways:
 - Confirmable (CON) messages retransmitted at increasing intervals until either an Acknowledgement (ACK) or a Rejection
   (RST) is received.
 - Non-confirmable (NON) messages retransmitted at constant intervals, to increase their chance of delivery.

For CON messages, the client transaction also provides corelation, by calling the application callback with the
particular per transaction application context callback (the `cb_param` parameter).

For this to function properly, the `message_id`s should be unique per peer. Use the `peer->getNextMessageId()` for good
values.

If the answer to message is no longer relevant, the client transaction can be stopped (a.i. canceled) with the 
`peer->stopRetransmissions(message_id);` method.

To see or debug the client transactions, use the `peer->logClientTransaction(L_INFO);` method.

**Important!** - to enable actually retransmissions, the `CoAPPeer::triggerPeriodicRetransmit()` static method must
be called every once in a while. This also triggers retransmission in DTLS, if needed.


### Server Transactions

Server transactions allow for deduplication of incoming requests or responses. A list of incoming `message_ids` is kept,
for EXCHANGE_LIFETIME in case of CON, or NON_LIFETIME in case of NON messages. Only the first message triggers
the request or reponse handlers, while duplicates are ignored.

When an ACK or RST is sent for that message, the message is storred and re-sent automatically on every subsequent 
retransmission received.

To see or debug the server transactions, use the `peer->logServerTransaction(L_INFO);` method.



## Timer Values and Adjusting for your Radio Latency, RTT and Packet-Loss

 To change timer values, please consult first [RFC7252](https://tools.ietf.org/html/rfc7252#section-4.8). Then proceed
 to change the Retransmit Parameters `#define` lines at the beginning of [`CoAPPeer.h`](CoAPPeer.h).



## Other Handlers

See above for the regular CoAP message handlers.

For DTLS events, a special handler can be registered:

```C
void your_function_DTLSEventHandler(CoAPPeer *peer, dtls_alert_level_e level, dtls_alert_description_e code) {
  LOG(L_INFO, "Received on peer %.*s:%u DTLS event level %d (%s) and description %d (%s)\r\n", 
      peer->remote_ip.len, peer->remote_ip.s, peer->remote_port, 
      level, dtls_alert_level_text(level),
      code, dtls_alert_description_text(code));
}
```


## Tokens and Context between Requests and Responses

Currently, tokens are available to be set and read from the `CoAPMessage` class. The `CoAPPeer` does not use them though
in any manner at the moment. This could be extended in the future, if useful for simplifying the application
development. Here are some of the possible future features:
  - Generation of unique tokens
  - Dialog context (not naming this "transaction", in order to avoid confusion with the Client/Server Transactions)
  - Generic parameters set per dialog context, passed as parameters to the request/reponse handlers
  - Additional callbacks for dialogs.

