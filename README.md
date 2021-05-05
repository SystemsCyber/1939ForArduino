# Ard1939
Arduino compatible implementation of SAE-J1939 specs at Transport and Network Layers



## Layers

1. Physical: The transreceiver handles functions at this layer that is to get bits on the MCU facing output pins and differtial voltage on the CAN facing output pins. In the second case
2. Data-Link: The controller typically takes care of this and returns CAN frames. If not, take a look at our AddressClaimDefender repo/code. It has a controller implementation built in.
3. Transport: This is included in the data-link specs (J1939/21) but typically it comes after the network layer in ISO/OSI specs. This is where messages from the Application layer are broken into PDU or PDUs are joined back into messages based on the direction of processing i.e. transmission or recieveing.
4. Network: This layer has nothing to do with the actual PDU. It does all the network configuration jobs like address claim but for this it utilizes the transport layer functions.
5. Application: Depending on the periodicity, this layer will sample parameters, put them in groups, convert them to a data buffer and send them to the transport layer. Or it will receive messages (data buffer of >= 64 bit), parse the paramters and update in the paramter array. So basically, the network layer and application layer operate independently and communicate with the trasport layer, not with each other.

## Code layout

The header J1939.h exposes all read and write functions from all layers and networking functions such that these can be called while demonstratin the I/O from each layer. The implementations are split into \<layer>.cpp files. Internally, the read/write from upper layers call the read/write from lower layers (e.g. 5 calls 3's read()).	

## Note thats

* This is not a pipeline application because it is not implemented on a multi-core or SMT enabled setup. Simply, when its not that a layer passes info upto the layer ans gets back work. It only works when its read and write methods are called.
* At this time the application layer code is empty as we need J1939 DA parsing fuctionality to address that
* The data link layer is made library-specific e.g. data_link_flexcan. This is so other hardware-specific libraries for arduino-based platforms can be used. To switch to a different library-based data-link api simply change the read/write function calls in the data-link wrapper.
* No in-built filtering is provided at this time. This is because this is a separate challenge. While data-link libraries provide filtering support for CAN frames based on Ids, transport frames do not carry the exact ID (represented as a combination of PGN, SRC and DST) while transfering data. This needs a separate filtering mechanism and it fort sure is not constant time. Hence this is not inplemented at this time. 