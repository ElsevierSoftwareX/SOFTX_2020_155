# load kosif driver
/etc/init.d/dis_kosif start

# load dx driver
/etc/init.d/dis_dx start

# load irm driver
/etc/init.d/dis_irm start

#insmod /lib/modules/3.0.8/kernel/drivers/dis/dis_kosif.ko
#insmod /lib/modules/3.0.8/kernel/drivers/dis/dis_dx.ko dx_node_id=-1 dx_msi_enable=1 dx_pio_remote_interrupt=1 dx_dma_urgent_timer=0 dx_dma_normal_timer=0 dx_repl_enable=1 dx_binding_teardown=0 dx_srom_reset=0
#insmod /lib/modules/3.0.8/kernel/drivers/dis/dis_irm.ko pcisciInsmodParams=1 dis_max_segment_size_megabytes=4 sci_page_size=1024 sci_timeout_delay=0 max-vc-number=1024 dis_report_resource_outtages=1 linkWatchdogEnabled=10 enable_session_probes_while_heartbeat_ok=0 use128BytesSciPackets=1 enableStreamErrorStatistics=1 prefetch_space_map_size=64 numberNonPrefEntries=512 numberPrefEntries=7128 number-of-megabytes-preallocated=160 minimum-size-to-allocate-from-preallocated-pool=0 try-first-to-allocate-from-preallocated-pool=1 block-size-of-preallocated-blocks=0 number-of-preallocated-blocks=0 use-sub-pools-for-preallocation=1 dis_use_iommu=0 
