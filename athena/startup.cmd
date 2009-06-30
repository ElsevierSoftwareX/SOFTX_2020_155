#dbLoadDatabase "../../dbd/athena.dbd"
#registerRecordDeviceDriver(pdbbase)
dbLoadRecords "a.db"
dbLoadTemplate "/target/pid_control.template"
#dbLoadRecords "/target/pid_control.db"
iocInit
