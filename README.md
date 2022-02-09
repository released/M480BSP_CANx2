# M480BSP_CANx2
 M480BSP_CANx2

update @ 2022/02/09

1. Test on M487 EVB , by using CAN0 RXD(PA.4) and TXD(PA.5) and CAN1 TXD(PE.7) and RXD(PE.6)

2. Scenario notice:

	- set RX by DEVICE A (CAN0) and DEVICE B (CAN1) , with data frame receive and remote frame receive
	
	- set DEVICE A data frame TX
	
	- set DEVICE B data frame TX
	
	- set DEVICE A remote frame TX
	
	- set DEVICE B remote frame TX
			

3. Notice : can.c is modified , base on FAE suggestion (compare with can.c in BSP to check difference )

4. below is log message catpure ( data frame and remote frame)

![image](https://github.com/released/M480BSP_CANx2/blob/main/TX.jpg)

![image](https://github.com/released/M480BSP_CANx2/blob/main/TX_remote.jpg)
