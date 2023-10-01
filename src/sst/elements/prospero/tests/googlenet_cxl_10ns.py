import sst


# Define SST core options
sst.setProgramOption("timebase", "10ps")
sst.setProgramOption("stopAtCycle", "0ms")

# Tell SST what statistics handling we want
sst.setStatisticLoadLevel(4)

clock = "2GHz"

cores = 1

#os.environ['OMP_NUM_THREADS'] = str(cores/2)


local_memory_capacity = 8192   	# Size of memory in MBs (원래: 128)
shared_memory_capacity = 16384	# 2GB ->8GB
shared_memory = 1
page_size = 4 # In KB 
num_pages = local_memory_capacity * 1024 // page_size + shared_memory_capacity*1024//page_size


# Define the simulation components
comp_cpu = sst.Component("cpu", "prospero.prosperoCPU")
comp_cpu.addParams({
    # "verbose" : "4",
    "reader" : "prospero.ProsperoTextTraceReader",
    "readerParams.file" : "/home/ydy/sst-elements/sst-elements-src/src/sst/elements/Opal/tests/googlenet_cxl_10ns_100ms-0.trace",
    "max_issue_per_cycle" : 1,
})


l1_params = {
        "cache_frequency": clock,
        "cache_size": "32KiB",
        "associativity": 8,
        "access_latency_cycles": 4,
        # "prefetcher" : "cassini.StridePrefetcher",
       	"L1": 1,
        "verbose": 30,
        # "maxRequestDelay" : "1000000",
        # "maxRequestDelay" : "2000000",
}

l2_params = {
        "cache_frequency": clock,
        "cache_size": "256KiB",
        "associativity": 8,
        "access_latency_cycles": 6,
        "mshr_num_entries" : 16,
        # "prefetcher" : "cassini.StridePrefetcher",
        # "prefetcher.reach": 4,
        # "prefetcher.detect_range":1,
}

l3_params = {
      	"access_latency_cycles" : "12",
      	"cache_frequency" : clock,
      	"associativity" : "16",
      	"cache_size" : "512KiB",
      	"mshr_num_entries" : "4096",
        # "num_cache_slices" : 1,
      	# "slice_allocation_policy" : "rr",
    #    "response_link_width" : "0B",
       "prefetcher" : "cassini.PalaPrefetcher",
    # #    "prefetcher" : "cassini.NextBlockPrefetcher",
        "prefetcher.prefetcher_mode" : 2,
        "prefetcher.prefetch_table_path": "/home/ydy/sst-elements/sst-elements-src/src/sst/elements/prospero/tests/prefetch_tables/prefetch_iplist_googlenet_10ns_512kb.txt",
       "prefetcher.reach": 1,
       "prefetcher.detect_range": 1,
}

link_params = {
	"shared_memory": shared_memory,
	"node": 0,
}

nic_params = {
	"shared_memory": shared_memory,
	"node": 0,
	"network_bw": "256GiB/s",
	"local_memory_size" : local_memory_capacity*1024*1024,
}



class Network:
    def __init__(self, name,networkId,input_latency,output_latency):
        self.name = name
        self.ports = 0
        self.rtr = sst.Component("rtr_%s"%name, "merlin.hr_router")
        self.rtr.addParams({
            "id": networkId,
            "topology": "merlin.singlerouter",
            "link_bw" : "256GiB/s",
            "xbar_bw" : "256GiB/s",
            "flit_size" : "8B",
            "input_latency" : input_latency,
            "output_latency" : output_latency,
            "input_buf_size" : "1KB",
            "output_buf_size" : "1KB",
            })

        topo = self.rtr.setSubComponent("topology", "merlin.singlerouter")

    def getNextPort(self):
        self.ports += 1
        self.rtr.addParam("num_ports", self.ports)
        return (self.ports-1)



internal_network = Network("internal_network",0,"20ps","20ps")

for next_core in range(cores):
        # print(next_core, cores//2)
        l1 = sst.Component("l1cache_" + str(next_core), "memHierarchy.Cache")
        l1.addParams(l1_params)
        l1_cpulink = l1.setSubComponent("cpulink", "memHierarchy.MemLink")
        l1_memlink = l1.setSubComponent("memlink", "memHierarchy.MemLink")
        l1_cpulink.addParams(link_params)
        l1_memlink.addParams(link_params)

        l2 = sst.Component("l2cache_" + str(next_core), "memHierarchy.Cache")
        l2.addParams(l2_params)
        l2_cpulink = l2.setSubComponent("cpulink", "memHierarchy.MemLink")
        l2_memlink = l2.setSubComponent("memlink", "Opal.OpalMemNIC")
        l2_cpulink.addParams(link_params)
        l2_memlink.addParams(nic_params)
        l2_memlink.addParams({ "group" : 1})
        # l2.enableAllStatistics({"type":"sst.AccumulatorStatistic", "rate": "500ms"})
        # arielMMULink = sst.Link("cpu_mmu_link_" + str(next_core))
        CpuCacheLink = sst.Link("cpu_cache_link_" + str(next_core))
        # PTWMemLink = sst.Link("ptw_mem_link_" + str(next_core))
        # PTWOpalLink = sst.Link("ptw_opal_" + str(next_core))
        # ArielOpalLink = sst.Link("ariel_opal_" + str(next_core))

        # modified code
        # arielMMULink.connect((ariel, "cache_link_%d"%next_core, "300ps"), (mmu, "cpu_to_mmu%d"%next_core, "300ps"))
        # ArielOpalLink.connect((memmgr, "opal_link_%d"%next_core, "300ps"), (opal, "coreLink%d"%(next_core), "300ps"))
        # ArielOpalLink.connect((memmgr, "opal_link_%d"%next_core, "300ps"), (opal, "coreLink%d"%(next_core), "300ps"))
        CpuCacheLink.connect((comp_cpu, "cache_link", "300ps"), (l1_cpulink, "port", "300ps"))
        # PTWOpalLink.connect( (pagefaulthandler, "opal_link_%d"%next_core, "300ps"), (opal, "mmuLink%d"%(next_core), "300ps") )
        
        l2_core_link = sst.Link("l2cache_" + str(next_core) + "_link")
        l2_core_link.connect((l1_memlink, "port", "300ps"), (l2_cpulink, "port", "300ps"))				

        l2_ring_link = sst.Link("l2_ring_link_" + str(next_core))
        l2_ring_link.connect((l2_memlink, "port", "300ps"), (internal_network.rtr, "port%d"%(internal_network.getNextPort()), "300ps"))

		
l3cache = sst.Component("l3cache", "memHierarchy.Cache")
l3cache.addParams(l3_params)
l3_link = l3cache.setSubComponent("cpulink", "Opal.OpalMemNIC")
l3cache.addParams({ "slice_id" : 0 })
l3_link.addParams(nic_params)
l3_link.addParams({
        "group" : 2,
	"addr_range_start": 0,
	"addr_range_end": (local_memory_capacity*1024*1024) + (shared_memory_capacity*1024*1024) -1,
	"interleave_size": "0B",
})

l3_ring_link = sst.Link("l3_link")
l3_ring_link.connect( (l3_link, "port", "300ps"), (internal_network.rtr, "port%d"%(internal_network.getNextPort()), "300ps"))

l3cache.enableAllStatistics({"type":"sst.AccumulatorStatistic", "rate": "500ms"})

mem = sst.Component("local_memory", "memHierarchy.MemController")	
mem.addParams({
    "addr_range_start" : 0,
	"addr_range_end" : (local_memory_capacity*1024*1024)-1,
	# "interleave_size": "0B",
	"clock"   : "1.2GHz",
	"backing" : "none",
	"backend" : "memHierarchy.timingDRAM",
	"backend.id" : 0,
	"backend.addrMapper" : "memHierarchy.roundRobinAddrMapper",
	"backend.addrMapper.interleave_size" : "64B",
	"backend.addrMapper.row_size" : "1KiB",
	"backend.clock" : "1.2GHz",
	"backend.mem_size" : str(local_memory_capacity) + "MiB",
	"backend.channels" : 1,
	"backend.channel.numRanks" : 2,
	"backend.channel.rank.numBanks" : 16,
	"backend.channel.transaction_Q_size" : 32,
	"backend.channel.rank.bank.CL" : 14,
	"backend.channel.rank.bank.CL_WR" : 12,
	"backend.channel.rank.bank.RCD" : 14,
	"backend.channel.rank.bank.TRP" : 14,
	"backend.channel.rank.bank.dataCycles" : 4,
	"backend.channel.rank.bank.pagePolicy" : "memHierarchy.simplePagePolicy",
	"backend.channel.rank.bank.transactionQ" : "memHierarchy.fifoTransactionQ",
	"backend.channel.rank.bank.pagePolicy.close" : 1,
})
mem_link = mem.setSubComponent("cpulink", "memHierarchy.MemLink")
mem_link.addParams({
    "shared_memory": 1,
    "node" : 0
})

mem.enableAllStatistics({"type":"sst.AccumulatorStatistic", "rate": "500ms"})

dc = sst.Component("dc", "memHierarchy.DirectoryController")
dc.addParams({
	"entry_cache_size": 256*1024*1024, #Entry cache size of mem/blocksize
	"clock": "200MHz",
        #"debug" : 1,
        #"debug_level" : 10,
})

dc_cpulink = dc.setSubComponent("cpulink", "Opal.OpalMemNIC")
dc_memlink = dc.setSubComponent("memlink", "memHierarchy.MemLink")
dc_memlink.addParams(link_params)
dc_cpulink.addParams(nic_params)
dc_cpulink.addParams({
        "group" : 3,
	"addr_range_start" : 0,
	"addr_range_end" : (local_memory_capacity*1024*1024)-1,
	"interleave_size": "0B",
	"shared_memory": shared_memory,
	"node": 0,
        #"debug" : 1,
        #"debug_level" : 10,
})

memLink = sst.Link("mem_link")
memLink.connect((mem_link, "port", "500ps"), (dc_memlink, "port", "500ps"))

netLink = sst.Link("dc_link")
netLink.connect((dc_cpulink, "port", "10000ps"), (internal_network.rtr, "port%d"%(internal_network.getNextPort()), "10000ps"))

# External memory configuration

# external_network = Network("Ext_Mem_Net",1,"100ns","100ns") # 이거 latency 조절?
port = internal_network.getNextPort()

# ext_mem = sst.Component("ExternalNVMmemContr", "memHierarchy.MemController")
ext_mem = sst.Component("shared_memory", "memHierarchy.MemController")
# ext_mem.addParams({
#     "memory_size" : str(shared_memory_capacity) + "MB",
# 	"max_requests_per_cycle"    : 4,
# 	"backing" : "none",
#     "clock" : clock,
# })
ext_mem.addParams({
    "addr_range_start" : (local_memory_capacity*1024*1024),
	"addr_range_end" : (local_memory_capacity*1024*1024) + (shared_memory_capacity*1024*1024) -1,
	"clock"   : "1.2GHz",
	"backing" : "none",
	"backend" : "memHierarchy.timingDRAM",
	"backend.id" : 0,
	"backend.addrMapper" : "memHierarchy.roundRobinAddrMapper",
	"backend.addrMapper.interleave_size" : "64B",
	"backend.addrMapper.row_size" : "1KiB",
	"backend.clock" : "1.2GHz",
    # "backend.access_time" : "200 ns",
	"backend.mem_size" : str(shared_memory_capacity) + "MiB",
	"backend.channels" : 1,
	"backend.channel.numRanks" : 2,
	"backend.channel.rank.numBanks" : 16,
	"backend.channel.transaction_Q_size" : 32,
	"backend.channel.rank.bank.CL" : 14,
	"backend.channel.rank.bank.CL_WR" : 12,
	"backend.channel.rank.bank.RCD" : 14,
	"backend.channel.rank.bank.TRP" : 14,
	"backend.channel.rank.bank.dataCycles" : 4,
	"backend.channel.rank.bank.pagePolicy" : "memHierarchy.simplePagePolicy",
	"backend.channel.rank.bank.transactionQ" : "memHierarchy.fifoTransactionQ",
	"backend.channel.rank.bank.pagePolicy.close" : 1,
})

ext_mem_link = ext_mem.setSubComponent("cpulink", "memHierarchy.MemLink")
ext_mem_link.addParams({
    "node" : 9999,
})

ext_dc = sst.Component("ExtMemDc", "memHierarchy.DirectoryController")
ext_dc.addParams({
	"entry_cache_size": 256*1024*1024, #Entry cache size of mem/blocksize
	# "clock": "1GHz",
    "clock": "200MHz",
})
ext_dc_cpulink = ext_dc.setSubComponent("cpulink", "Opal.OpalMemNIC")
ext_dc_memlink = ext_dc.setSubComponent("memlink", "memHierarchy.MemLink")

ext_dc_cpulink.addParams({
    "interleave_size": "0B", # 이거 필요한지?
	"network_bw": "256GiB/s",
	"addr_range_start" : (local_memory_capacity*1024*1024),
	"addr_range_end" : (local_memory_capacity*1024*1024) + (shared_memory_capacity*1024*1024) -1,
	"node": 9999,
    "group" : 3, # TODO is this the right routing group? means sources are all components in group 2 and dests are all components in group 4
})

extmemLink = sst.Link("External_mem_dc_link")
extmemLink.connect( (ext_dc_memlink, "port", "500ps"), (ext_mem_link, "port", "500ps") )

ext_dcLink = sst.Link("External_mem_link")
ext_dcLink.connect( (ext_dc_cpulink, "port", "500ns"), (internal_network.rtr, "port%d"%port, "500ns") )

sst.setStatisticOutput("sst.statOutputCSV", {"filepath" : "./final_csv_results/512KB_Googlenet_10ns100ms_500ns_ibsp+nb.csv", "separator" : ", " } )