#!/usr/bin/python3

class DeviceList:
    def __init__(self, dev):
        self.dev = dev
        unas = UnassignedPins()
        self.devices = { unas.name: unas }

    async def connect(self):
        self.kernel = KernelConnector(self, self.dev)
        await self.reload()

    async def reload(self):
        updated = set()
        for d in await self.kernel.getDeviceList():
            if d.name in self.devices:
                self.devices[d.name].merge(d)
            else:
                self.devices[d.name] = d

            updated |= { d.name }

        for name in self.devices:
            if name in updated:
                continue
            print(f"Removing device {name}")
            self.devices[name].remove()
            del self.devices[name]

class Device:
    def merge(self, new):
        raise NotImplementedError("Devices must know how to merge themselves")

    def remove(self):
        raise NotImplementedError("Devices must know how to remove themselves")

    typemap = {}
    @classmethod
    def register(self, _id):
        if _id is None:
            raise NotImplementedError("Devices must declare their typeID")
        if _id in self.typemap and self.typemap[_id] is not self:
            raise Exception("Clashing typeID {_id} of {self} and {self.typemap[_id]}")

        self.typemap[_id] = self

    @classmethod
    def identify(self, _id):
        if _id in self.typemap:
            return self.typemap[_id]

        raise Exception("TypeID {_id} not registered")

# Deferred imports to avoid circular dependency hell
from proput.kernel import KernelConnector
from proput.device.unassigned import UnassignedPins
