Note: 
1. BUP A build has Dual Configuration feature, it is the factory build (SW2 switch 2 to the ON position[0]).
2. Dual configuration feature of MAX 10 device: 
	a. Have a Dual Boot IP core in Qsys component
	b. Choose dual internal image in configuration mode in "Device and Pin Options"
3. The follows are the steps to test BUP A & BUP B.
    Preparation
   	a. Please add '*\intelFPGA\22.1std\nios2eds\bin\gnu\H-x86_64-mingw32\bin' in 'Path' of 'User Variables'.
   	b. Reprogram *\10m50daf484i6g_fpga_v22.1std.0\factory_recovery\dual_boot_image.pof into Configuration Flash Memory (CFMs) via programmer
   	c. Change SW2 switch 2 to the ON position (0)
	d. Make sure the board and the test computer are on the same network
    BUP A:
    	a. Make sure Ethernet cable is connected to port A (the lower one)
   	b. Power cycle the board or push S5 button to boot up from BUP build (BUP A)
   	c. Open NIOS command shell at *\intelFPGA\22.1std\nios2eds, e.g. double click "Nios II Command Shell.bat"
   	d. Type "nios2-terminal.exe"
   	e. Check whether it gets corresponding IP address
   	f. Launch IE and type the IP address got from step #g in the address bar (e.g. http://192.168.1.100, Change Proxy Server setting if needed)
   	g. see whether the webpage is working or not
    BUP B:
	a. Make sure Ethernet cable is connected to port B (the higher one)
	b. Power cycle the board
	c. Open NIOS command shell at *\intelFPGA\22.1std\nios2eds, e.g. double click "Nios II Command Shell.bat"
	d. Change directory to *\10m50daf484i6g_fpga_v22.1std.0\examples\board_update_portal
	e. Type “nios2-configure-sof --cable 1 --device 1 bup_ethb.sof;sleep 5;nios2-download -g -r bup_ethb.elf; nios2-terminal;”
	f. Check whether it gets corresponding IP address as expected.
	g. Launch IE and type the IP address got from step #g in the address bar (e.g. http://192.168.1.101, Change Proxy Server setting if needed)
   	h. see whether the webpage is working or not  
	


   


