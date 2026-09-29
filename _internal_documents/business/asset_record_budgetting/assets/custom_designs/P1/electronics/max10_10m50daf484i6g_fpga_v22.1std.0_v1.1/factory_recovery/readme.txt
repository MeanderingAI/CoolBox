How to restore the factory build:
a. Please add '*\intelFPGA\22.1std\nios2eds\bin\gnu\H-x86_64-mingw32\bin' in 'Path' of 'User Variables'.
b. Reprogram *\10m50daf484i6g_fpga_v22.1std.0\factory_recovery\dual_boot_image.pof into Configuration Flash Memory (CFMs) via programmer
c. Change SW2 switch 2 to the ON position (0)
d. Make sure the board and the test computer are on the same network
e. Make sure Ethernet cable is connected to port A (the lower one)
f. Power cycle the board or push S5 button to boot up from BUP build (BUP A)
g. Open NIOS command shell at *\intelFPGA\22.1std\nios2eds, e.g. double click "Nios II Command Shell.bat"
h. Change directory to *\10m50daf484i6g_fpga_v22.1std.0\factory_recovery\build_factory_source\
i. Type "nios2-download --cable n* --device 1 -g -r ./bup_etha.elf; nios2-terminal.exe;"
j. Check whether it gets corresponding IP address
k. Launch IE and type the IP address got from step #g in the address bar (e.g. http://192.168.1.100, Change Proxy Server setting if needed)
l. If it turns out be the MAX 10 FPGA development kit Webpage, please click the bottom-right "Factory Restore" link to restore factory build, 
   otherwise, it should be the raw page for factory build programming
m. Load "*\10m50daf484i6g_fpga_v22.1std.0\factory_recovery\build_factory_source\m10_fpga_html.zip" in "Webpages File Name" column
   and  "*\10m50daf484i6g_fpga_v22.1std.0\factory_recovery\build_factory_source\ext_flash_etha.flash"   in "Software File Name" column
   and then upload them
n. Check whether it uploads successfully or not
o. Power cycle the board again
p. Type "nios2-terminal.exe;" in NIOS command shell again to check whether it can get IP address as expected
q. Launch IE and type its IP address in the address bar to see whether the webpage is working or not

Note: 
a. n*: USB cable number, e.g. "cable 1"
b. Dual-boot images:
   a) Dual compressed image #0: BUP A build
   b) Dual compressed image #1: GPIO (bts_config) build
   c) Please turn to the "Selecting the Internal Configuration Scheme" section of the User Guide for Dual Configuration feature
c. Ethernet Port A is used by the factory BUP build by default