#!/usr/bin/python3

import json

class DeviceList:
    def __init__(self, dev):
        self.dev = dev
        unas = UnassignedPins()
        self.devices = { unas.name: unas }
        self.sockets = set()
        self.ws_handlers = {
                "devicelist": self.sendDeviceList
                }

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

    async def websocket(self, socket, _):
        self.sockets.add(socket)
        async for message in socket:
            obj = json.loads(message)
            if obj["msgID"] > 0 and obj["request"] in self.ws_handlers:
                await self.ws_handlers[obj["request"]](socket, obj)
            else:
                break
        self.sockets.remove(socket)

    async def sendDeviceList(self, socket, obj):
        await socket.send(json.dumps({
            "msgID": obj["msgID"],
            "devices": dict(self.devices),
            }, cls=DeviceEncoder))


class Device:
    def merge(self, new):
        raise NotImplementedError("Devices must know how to merge themselves")

    def remove(self):
        raise NotImplementedError("Devices must know how to remove themselves")

    def toJSON(self):
        return { "name": self.name, "displayName": self.displayName }

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


class DeviceEncoder(json.JSONEncoder):
    def default(self, obj):
        return obj.toJSON() if isinstance(obj, Device) else super().default(obj)

# Deferred imports to avoid circular dependency hell
from proput.kernel import KernelConnector
from proput.device.unassigned import UnassignedPins
