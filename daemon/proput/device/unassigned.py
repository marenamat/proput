#!/usr/bin/python3

import struct

from proput.device import Device

class UnassignedPins(Device):
    def __init__(self, data=None):
        super().__init__()

        self.name = "unassigned"
        self.displayName = "Unassigned Pins"

        if data is None:
            self.pins = 0
            return

        if len(data) != 8:
            raise Exception(f"Unassigned pins data of size {len(data)}, should be 8")

        bits0, = struct.unpack("=I", data[0:4])
        bits1, = struct.unpack("=I", data[4:8])

        self.pins = bits0 | (bits1 << 32)
        print(f"Unassigned pins bitmask: {hex(self.pins)}")

    def merge(self, new):
        self.pins = new.pins

    def remove(self):
        raise Exception("UnassignedPins metadevice must never be removed")

    def toJSON(self):
        out = super().toJSON()
        out["pins"] = self.pins
        return out

UnassignedPins.register(1)
