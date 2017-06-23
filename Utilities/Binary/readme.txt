To enable user deployment and testing of end-to-end solutions between wireless sensor nodes and internet servers, sample node applications are provided in the form of pre-compiled and ready-to-use binaries for STM32 Nucleo platforms equipped with expansion boards. The examples export different resources, based on the type of expansion boards stacked on the NUCLEO-F401RE. 

The supported configurations are:

1. NUCLEO-F401RE + X-NUCLEO-IDS01A4 or X-NUCLEO-IDS01A5 (/Utilities/Binary/<SAMPLE_APPLICATION>/Ipso_Example_Nosensors/)

2. NUCLEO-F401RE + X-NUCLEO-IDS01A4 or X-NUCLEO-IDS01A5 + X-NUCLEO-IKS01A1 (/Utilities/Binary/<SAMPLE_APPLICATION>/Ipso_Example_Mems/)

3. NUCLEO-F401RE + X-NUCLEO-IDS01A4 or X-NUCLEO-IDS01A5 + X-NUCLEO-6180XA1 (/Utilities/Binary/<SAMPLE_APPLICATION>/Ipso_Example_Proximity/)

Before proceeding be sure to:
- Install ST-LINK drivers: http://www.st.com/content/st_com/en/products/development-tools/hardware-development-tools/development-tool-hardware-for-mcus/debug-hardware-for-mcus/debug-hardware-for-stm32-mcus/st-link-v2.html   
- On the Nucleo board put JP5 jumper in U5V position

The binaries are provided in both .bin and .hex format and can be flashed into the micro-controller flash memory using one of the procedures described below. 

- Procedure 1 (.bin only) -

 1 - Plug the Nucleo Board to the host PC using a mini USB cable. If the ST-LINK driver is correctly installed, it will be recognized as an external memory device called "NUCLEO"
 2 - Drag and drop or copy the binary file into the "NUCLEO" device you see in Computer
 3 - Wait until flashing is complete.

- Procedure 2 (.hex and .bin) -

 1 - Install ST-LINK Utility: http://www.st.com/content/st_com/en/products/development-tools/hardware-development-tools/development-tool-hardware-for-mcus/debug-hardware-for-mcus/debug-hardware-for-stm32-mcus/st-link-v2.html
 2 - Plug the Nucleo Board to the host PC using a micro USB cable.
 3 - Open the ST-LINK utility.
 4 - Set the "Connect Under Reset" Mode selecting "Target -> Settings" from the menu.
 5 - Connect to the board selecting "Target -> Connect" from the menu or pressing the corresponding button.
 6 - Open the binary file selecting "File -> Open File..." and choose the one you want to flash.
 7 - From the menu choose: "Target -> Program"
 8 - Check the Start address, it must be equal to 0x08000000.
 9 - Check the box "Reset after programming"
10 - Click Start.
11 - Wait until flashing is complete.
