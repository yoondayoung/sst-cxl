# Automatically generated SST Python input
import sst
import os

# Define SST core options
sst.setProgramOption("timebase", "1ps")
sst.setProgramOption("stop-at", "5us")

# Define the simulation components
comp_cpu = sst.Component("cpu", "prospero.prosperoCPU")
comp_cpu.addParams({
    "verbose" : "16",
    "reader" : "prospero.ProsperoTextTraceReader",
    "readerParams.file" : "/home/moon/model/VGG16/small_vgg16_trace",
    "max_issue_per_cycle" : 1,
    "act_mem_size" : 128 * 1024 * 1024, # 
})


comp_l1cache = sst.Component("l1cache", "memHierarchy.Cache")
comp_l1cache.addParams({
      "access_latency_cycles" : "1",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "L1" : "1",
      "cache_size" : "64 KB"
})
comp_memctrl = sst.Component("memory", "memHierarchy.MemController")
comp_memctrl.addParams({
      "clock" : "1GHz"
})


#changed main memory simpleMem -> timingDRAM
memory = comp_memctrl.setSubComponent("backend", "memHierarchy.timingDRAM")
memory.addParams({
    "id" : 0,
	"addrMapper" : "memHierarchy.roundRobinAddrMapper",
	"addrMapper.interleave_size" : "64B",
	"addrMapper.row_size" : "1KiB",
	"clock" : "1.2GHz",
	"mem_size" : "1024MiB",
	"channels" : 2,
	"channel.numRanks" : 2,
	"channel.rank.numBanks" : 16,
	"channel.transaction_Q_size" : 32,
	"channel.rank.bank.CL" : 14,
	"channel.rank.bank.CL_WR" : 12,
	"channel.rank.bank.RCD" : 14,
	"channel.rank.bank.TRP" : 14,
	"channel.rank.bank.dataCycles" : 2,
	"channel.rank.bank.pagePolicy" : "memHierarchy.simplePagePolicy",
	"channel.rank.bank.transactionQ" : "memHierarchy.fifoTransactionQ",
	"channel.rank.bank.pagePolicy.close" : 1,
})



# Define the simulation links
link_cpu_cache_link = sst.Link("link_cpu_cache_link")
link_cpu_cache_link.connect( (comp_cpu, "cache_link", "1000ps"), (comp_l1cache, "high_network_0", "1000ps") )
link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (comp_l1cache, "low_network_0", "50ps"), (comp_memctrl, "direct_link", "50ps") )
# End of generated output.
