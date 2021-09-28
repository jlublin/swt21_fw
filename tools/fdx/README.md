To use FDX in your CANoe project you'll need to folllow these steps.

1. In the CANoe menu go to "File" -> "Options" -> "Extensions" ->
   "XIL API & FDX Protocol".

2. Choose "Enable FDX" and make sure "Transport Layer" is "UDP IPv4" and
   "Port number" is 2809.

3. Under "FDX Description Files" click "Add" and choose the "labkit.xml".
   You can study this file in the FDX editor.

4. In the CANoe menu go to "Environment" -> "System Variables".

5. Under "User-Defined" add two new variables with the following settings:

   Namespace: FDX
   Name: CanFrameSend
   Data type: Data

   Namespace: FDX
   Name: CanFrameRecv
   Data type: Data

6. In the "Simulation Setup" add a new "Network Node" and in its configuration
   set it to use "labkit.can".

7. Start "client.py" with the options for your setup (COM-port, BRP, TSEGx etc).
   Usage: ./client.py <COM-port> <brp> <tseg1> <tseg2> [<sjw>] [<FDX addr>] [<FDX port>]

8. Now you can start your simulation in "Online Mode" with "Simulated Bus" and
   it will communicate using the labkit.
