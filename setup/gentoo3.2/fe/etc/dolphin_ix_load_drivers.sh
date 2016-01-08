insmod /lib/modules/3.2.73/kernel/drivers/disix/dis_kosif.ko
insmod /lib/modules/3.2.73/kernel/drivers/disix/dis_ix_dma.ko 
insmod /lib/modules/3.2.73/kernel/drivers/disix/dis_ix_ntb.ko 
insmod /lib/modules/3.2.73/kernel/drivers/disix/dis_irm.ko pcisciInsmodParams=1 dis_max_segment_size_megabytes=4 sci_page_size=1024 
max-vc-number=2048 dis_report_resource_outtages=1 linkWatchdogEnabled=10 min_link_width=8 min_link_speed=2 enable_session_probes_while_heartbeat_ok=0 enableStreamErrorStatistics=1 prefetch_space_map_size=64 number-of-megabytes-preallocated=0 minimum-size-to-allocate-from-preallocated-pool=0 try-first-to-allocate-from-preallocated-pool=0 block-size-of-preallocated-blocks=0 number-of-preallocated-blocks=0 use-sub-pools-for-preallocation=1 dis_use_iommu=0 
