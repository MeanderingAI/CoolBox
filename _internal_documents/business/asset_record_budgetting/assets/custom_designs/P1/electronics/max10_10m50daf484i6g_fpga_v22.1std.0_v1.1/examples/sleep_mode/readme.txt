Sleep Mode Reference Design: 
A reference design utilizes MAX 10’s clock tree sleep mode and selectable input buffer power off features.
Clock tree sleep and input buffer turn off can be individually selectable via switches or buttons to show unique power benefit.

Ps: the design here only utilizes clock tree sleep mode

1. Sleep mode design is included in BTS GUI package
2. Configure sleep mode design and then use sleep-mode tab to turn on/turn off clock tree sleep mode
3. The power for the MAX 10 device core supplies and VCCIO should have visible power difference in the Power Monitor when toggling between full power and sleep mode.