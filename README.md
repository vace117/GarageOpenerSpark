GarageOpener - Spark Core based Garage Opener
========================

This code implements Spark Core firmware that runs a secure TCP/IP server in order to accept commands to control or query the status of the overhead garage door. The commands are sent from an Android phone.

= Building the firmware =
 $ cd core-firmware/build
 $ make

= Installation =
Upload the firmware to the Spark Core like so:
 $ dfu-util -d 1d50:607f -a 0 -s 0x08005000:leave -D core-firmware.bin


= Protocol =
The Android app connects to a TCP/IP socket exposed directly to the Internet. The available commands are:
 - OPEN
 - CLOSE
 - PRESS_BUTTON
 - GET_STATUS

The possible responses:
 - DOOR_OPEN
 - DOOR_CLOSED
 - DOOR_MOVING

= Security =
Symmetric shared-key security is used. The client Android app must have a secret key in order to connect. 

AES-128 is used for encrypting the traffic. Challenge based, timed session token approach is used to prevent Replay Attacks. Session tokens are valid for 5 seconds after the random challenge nonce is issued. The pseudo-random generator (PRGA) is initialized using any one of the 65535 pre-computed 48-bit seeds, stored in External Flash. Every time the Spark reboots, a new seed is chosen by incrementing the seed index, also stored in External Flash. The seeds are mixed with additional entropy obtained from ping times a DNS server.

The security is implemented with the following algorithm:

== AndroidRequest(COMMAND) == 
Android 1) Generate random IV_Send[16] and IV_Response[16]
Android 2) Send [IV_Send, IV_Response, AES_CBC(Key, IV_Send, COMMAND), <==== HMAC(Key)]
Spark 1) Verify that HMAC(Key, <payload>) matched the received HMAC
Spark 2) Decrypt and return payload

== SparkResponse(RESPONSE) == 
Spark 1) Send [AES_CBC(Key, IV_Response, RESPONSE), <==== HMAC(Key)]
Android 1) Verify that HMAC(Key, <payload>) matched the received HMAC
Android 2) Decrypt and return response

We already have a shared secret, so we don't need to do DH key exchange. We can establish our sessionKey on both sides, by simply mixing the shared secret with the server challenge.

== Session Establishment Sequence == 
Android 1) AndroidRequest("NEED_CHALLENGE")
Spark 1) Use PRNG to generate random Challenge[16]
Spark 2) Calculate sessionKey == HMAC(Key, Challenge[16])
Spark 3) Start 5 second timer
Spark 4) SparkResponse(Challenge[16])
Android 2) Calculate sessionKey == HMAC(Key, Challenge[16])
Android 3) AndroidRequest( [sessionKey, COMMAND] )
Spark 5a) Make sure that sessionKey matched the recieved one. If so, execute command, invalidate sessionKey and SparkResponse( [sessionKey, DOOR_STATUS] )
Spark 5b) If timer expires, invalidate sessionKey
Android 4) Make sure sessionKey in response matched, update screen and invalidate sessionKey
