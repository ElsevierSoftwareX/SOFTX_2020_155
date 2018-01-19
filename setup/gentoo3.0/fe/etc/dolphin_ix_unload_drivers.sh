#!/bin/sh

# unload drivers for shutdown

# unload sisci driver
/etc/init.d/dis_sisci stop
# unload irm driver
/etc/init.d/dis_irm stop
# unload ix driver
/etc/init.d/dis_ix stop
# unload kosif driver
/etc/init.d/dis_kosif stop

