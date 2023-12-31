# Automatically generated SST Python input
import sst

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stop-at", "10000s")

# Define the simulation components
comp_clocker0 = sst.Component("clocker0", "simpleSimulation.simpleCarWash")
comp_clocker0.addParams({
      "clockcount" : """100000000""",
      "clock" : """1MHz"""
})


# Define the simulation links
# End of generated output.
