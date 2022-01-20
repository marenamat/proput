#!/usr/bin/python3

from proput.kernel import KernelConnector

class DeviceList:
    def __init__(self, dev):
        self.dev = dev
        self.devices = {}

    async def connect(self):
        self.kernel = KernelConnector(self, self.dev)
        await self.reload()

    async def reload(self):
        for d in await self.kernel.getDeviceList():
            self.devices[d.name] = d
