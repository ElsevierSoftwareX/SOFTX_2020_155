#!/usr/bin/env python3

import argparse
import struct
import sys


class Field(object):
    def __init__(self, name, format):
        self.__name = name
        self.__format = format
        self.__value = None

    def name(self):
        return self.__name

    def format(self):
        return self.__format

    def set(self, value):
        self.__value = value

    def get(self):
        return self.__value

    def __len__(self):
        return struct.calcsize(self.__format)


class FieldArray(object):
    def __init__(self, name, count, format):
        self.__name = name
        self.__count = count
        self.__format = format
        self.__unit_size = struct.calcsize("<" + self.__format)
        self.__value = None

    def name(self):
        return self.__name

    def format(self):
        return "{0}s".format(self.__count * self.__unit_size)

    def set(self, value):
        self.__value = struct.unpack("<{0}{1}".format(self.__count,
                                                      self.__format),
                                     value)

    def get(self):
        return self.__value

    def __len__(self):
        return struct.calcsize(self.format())


class BasicStructure(object):
    def __init__(self, name, fields, blob, alias={}):
        self.__format = "<" + ''.join([x.format() for x in fields])
        self.__name = name
        self.__size = struct.calcsize(self.__format)
        self.__fields = fields
        self.__field_alias = alias
        field_names = []
        fields = struct.unpack(self.__format, blob[0: self.__size])
        try:
            assert (len(fields) == len(self.__fields))
        except:
            print(len(fields))
            print(fields)
            print(len(self.__fields))
            print(self.__fields)
            raise
        for i in range(len(fields)):
            self.__fields[i].set(fields[i])
            field_names.append(self.__fields[i].name())
        self.__remaining_data = len(blob) - self.__size

        for k in alias.keys():
            name = alias[k]
            if not name in field_names:
                raise Exception('bad field alias {0} {1}->{2}'.format(
                    self.__name,
                    k,
                    name,
                ))

    def name(self):
        return self.__name

    def get(self, field_name):
        if field_name in self.__field_alias:
            field_name = self.__field_alias[field_name]
        for field in self.__fields:
            if (field.name() == field_name):
                return field.get()
        return None

    def __len__(self):
        return self.__size

    def dump(self):
        txt = "{0}:\n".format(self.name())
        txt += "".join(["\t{0}: {1}\n".format(f.name(), f.get()) for f in self.__fields])
        txt += "\t--\n"
        txt += "\tbase struct size: {0}\n".format(self.__size)
        txt += "\tremaining data: {0}\n".format(self.__remaining_data)
        return txt


class daq_multi_cycle_data_t(BasicStructure):
    def __init__(self, blob):
        super().__init__("daq_multi_cycle_data_t",
                         [
                             Field('curCycle', 'I'),
                             Field('maxCycle', 'I'),
                             Field('maxDcuCount', 'I'),
                             Field('cycleDataSize', 'I'),
                         ],
                         blob,
                         alias={
                             'cur_cycle': 'curCycle',
                             'max_cycle': 'maxCycle',
                             'cycle_size': 'cycleDataSize',
                         })

    def offset_to_cycle(self, cycle):
        if (cycle >= self.get('maxCycle')):
            raise IndexError('Requested a non-existant cycle {0}/{1}'.format(cycle, self.get('maxCycle')))
        return len(self) + (self.get('cycle_size') * cycle)


class daq_dc_data_t(BasicStructure):
    def __init__(self, blob):
        super().__init__('daq_dc_data_t',
                         [
                             Field('dcuTotalModels', 'i'),
                         ],
                         blob,
                         alias={
                             'model_count': 'dcuTotalModels',
                         })


class daq_msg_header_t(BasicStructure):
    def __init__(self, blob):
        super().__init__('daq_msg_header_t',
                         [
                             Field('dcuId', 'I'),
                             Field('fileCrc', 'I'),
                             Field('status', 'I'),
                             Field('cycle', 'I'),
                             Field('timeSec', 'I'),
                             Field('timeNSec', 'I'),
                             Field('dataCrc', 'I'),
                             Field('dataBlockSize', 'I'),
                             Field('tpBlockSize', 'I'),
                             Field('tpCount', 'I'),
                             FieldArray('tpNum', 256, 'I'),
                         ],
                         blob)


class blockProp(BasicStructure):
    def __init__(self, blob):
        super().__init__('blockProp',
                         [
                             Field('status', 'I'),
                             Field('timeSec', 'I'),
                             Field('timeNSec', 'I'),
                             Field('run', 'I'),
                             Field('cycle', 'I'),
                             Field('crc', 'I'),
                         ],
                         blob)


class rmIpcStr(BasicStructure):
    def __init__(self, blob):
        super().__init__('rmIpcStr',
                         [
                             Field('cycle', 'I'),
                             Field('dcuId', 'I'),
                             Field('crc', 'I'),
                             Field('command', 'I'),
                             Field('cmdAck', 'I'),
                             Field('request', 'I'),
                             Field('reqAck', 'I'),
                             Field('status', 'I'),
                             Field('channelCount', 'I'),
                             Field('dataBlockSize', 'I'),
                         ],
                         blob,
                         alias={
                             'cur_cycle': 'cycle',
                             'cycle_size': 'dataBlockSize',
                         })
        self.max_cycle = 16
        self.block_props = []
        tp_offset = 0x1000
        tp_count = struct.unpack("=I", blob[tp_offset:tp_offset + 4])[0]
        tp_offset += 4
        self.tp_table = struct.unpack("={0}I".format(tp_count),
                                      blob[tp_offset:tp_offset + 4 * tp_count])

        cur_offset = len(self)
        for i in range(self.max_cycle):
            block_prop = blockProp(blob[cur_offset:])
            self.block_props.append(block_prop)
            cur_offset += len(block_prop)

    def get_tp_table(self):
        return self.tp_table

    def get_block_prop(self, cycle):
        return self.block_props[cycle]

    def offset_to_cycle(self, cycle):
        if cycle >= self.max_cycle:
            raise IndexError('Requested a non-existant cycle {0}/{1}'.format(
                cycle, self.max_cycle
            ))
        DAQ_DCU_SIZE = 0x400000
        DAQ_NUM_DATA_BLOCK = 16
        DAQ_DCU_BLOCK_SIZE = DAQ_DCU_SIZE // DAQ_NUM_DATA_BLOCK
        return 0x2000 + (DAQ_DCU_BLOCK_SIZE * 2 * cycle)


class DataDumper(object):
    def __init__(self, descriptor):
        def usage():
            raise RuntimeError("Data range specification is 'offset:length[:formatchar]'")

        self.__is_null = True
        if descriptor is None or descriptor == "":
            return
        self.__is_null = False
        parts = descriptor.split(":", 2)
        if not len(parts) in set([2, 3, ]):
            usage()
        self.__offset = int(parts[0])
        self.__length = int(parts[1])
        self.__format = None
        if len(parts) > 2:
            self.__format = parts[2][0]
        if self.__offset < 0 or self.__length < 1:
            usage()
        self.__stride = 1
        if not self.__format is None:
            self.__stride = struct.calcsize('<' + self.__format)
            if self.__length % self.__stride != 0:
                print('Requested length is not a multiple of the base type size')
                usage()

    def should_dump(self):
        return not self.__is_null

    def dump(self, buffer):
        if self.__is_null:
            return
        if len(buffer) < self.__offset + self.__length:
            raise IndexError('Data buffer too small for data request {0}/{1}'.format(
                self.__offset + self.__length,
                len(buffer),
            ))
        if self.__format is None:
            print(" ".join([hex(int.from_bytes(o, byteorder='little'))[2:] for o in
                            buffer[self.__offset: self.__offset + self.__length]]))
        else:
            format = "<" + self.__format
            offset = self.__offset
            remaining = self.__length
            stride = self.__stride
            pre = ""
            out = ""
            while remaining > 0:
                data = struct.unpack(format, buffer[offset: offset + stride])[0]
                out += "{0}{1}".format(pre, data)
                pre = " "
                offset += stride
                remaining -= stride
            print(out)

    def __str__(self):
        return "{0} {1} {2} {3}".format(
            self.__is_null,
            self.__offset,
            self.__length,
            self.__format,
        )


def read_blob(fname):
    with open(fname, 'rb') as f:
        return f.read()


def handle_daq_multi_cycle(args, buffer):
    dumper = DataDumper(args.data)
    if dumper.should_dump() and args.dcu < 0:
        print("When requesting a data dump, please select a specific dcu")
        sys.exit(1)

    multi_cycle_hdr = daq_multi_cycle_data_t(buffer)
    cur_cycle = multi_cycle_hdr.get('cur_cycle')
    if args.cycle >= 0:
        cur_cycle = args.cycle
    dcu_text = "all"
    cur_dcu = args.dcu
    if args.dcu >= 0:
        dcu_text = str(args.dcu)

    print('read in {0} bytes of data'.format(len(buffer)))
    print('dumping cycle {0} and dcu {1}\n'.format(cur_cycle, dcu_text))

    print("{0}".format(multi_cycle_hdr.dump()))

    cycle_offset = multi_cycle_hdr.offset_to_cycle(cur_cycle)

    cycle_header = daq_dc_data_t(buffer[cycle_offset:cycle_offset + multi_cycle_hdr.get('cycle_size')])
    print("{0}".format(cycle_header.dump()))

    model_count = cycle_header.get('model_count')
    if cur_dcu >= 0 and model_count <= cur_dcu:
        raise IndexError('Requested and invalid dcu/model {0}/{1}'.format(
            cur_dcu,
            model_count,
        ))

    dcu_offset = cycle_offset + len(cycle_header)
    cur_dcu_data_offset = 0
    dcu_struct_size = 0
    for i in range(model_count):
        dcu = daq_msg_header_t(buffer[dcu_offset:])
        dcu_struct_size = len(dcu)
        if cur_dcu < 0 or cur_dcu == i:
            print("dcu index {0}".format(i))
            print(dcu.dump())
            if cur_dcu == i:
                break
        cur_dcu_data_offset += dcu.get('dataBlockSize') + dcu.get('tpBlockSize')
        dcu_offset += len(dcu)

    if not dumper.should_dump():
        return

    data_block_offset = cycle_offset + len(cycle_header) + dcu_struct_size * 128 + cur_dcu_data_offset
    print("Offset {0}:".format(data_block_offset))
    dumper.dump(buffer[data_block_offset:])


def handle_rmipc(args, buffer):
    dumper = DataDumper(args.data)

    rmipc = rmIpcStr(buffer);
    cur_cycle = rmipc.get('cur_cycle')
    if args.cycle >= 0:
        cur_cycle = args.cycle

    print('read in {0} bytes of data'.format(len(buffer)))
    print('dumping cycle {0}\n'.format(cur_cycle))

    print("{0}".format(rmipc.dump()))

    tp_table = rmipc.get_tp_table()
    print("Test Point Table:\n\tCount: {0}\n\t{1}\n".format(len(tp_table), tp_table))

    print("{0}".format(rmipc.get_block_prop(cur_cycle).dump()))

    data_block_offset = rmipc.offset_to_cycle(cur_cycle)
    print("offset = {0}".format(data_block_offset))
    if dumper.should_dump():
        dumper.dump(buffer[data_block_offset:])


def main(argv):
    struct_dispatch = {
        'daq_multi_cycle_data_t': handle_daq_multi_cycle,
        'rmIpcStr': handle_rmipc,
    }
    allowed_structs = struct_dispatch.keys()
    parser = argparse.ArgumentParser(description="Dump data from zmq_daq_core.h structures")
    parser.add_argument('-i', '--input', default='probe_out.bin', dest='input_file',
                        help="Dump file to process")
    parser.add_argument('-c', '--cycle', default=-1, type=int, help="data cycle to dump from")
    parser.add_argument('-d', '--dcu', default=-1, type=int, help="which dcu block to dump. "
                                                                  "Defaults to all dcus. "
                                                                  "Does not apply to rmIpcStr.")
    parser.add_argument('-D', '--data', default='',
                        help="Print data from a dcu 'offset:length[:formatchar]'. "
                             "Offset is from the start of the individual dcu.  Formatchar "
                             "is optional and is a single char describing the python struct "
                             "formatting [little endian only], prints in hex otherwise. "
                             "A specific dcu must be specified (or struct = rmIpcStr.")
    parser.add_argument('-s', '--struct', default='daq_multi_cycle_data_t',
                        help="Select the structure type to interpret. "
                             "Must be one of {0} "
                             "Defaults to daq_multi_cycle_data_t".format(allowed_structs))

    args = parser.parse_args(argv[1:])
    if not args.struct in allowed_structs:
        parser.print_help()
        sys.exit(1)

    buffer = read_blob(args.input_file)

    struct_dispatch[args.struct](args, buffer)


main(sys.argv)
