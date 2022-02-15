#!/usr/bin/python3

import asyncio
import os
import sys
import logging
import struct

from proput.device import Device

log = logging.getLogger(__name__)

class KernelCommand:
    def __init__(self):
        self.finished = asyncio.get_running_loop().create_future()

    def response(self):
        return self.finished.result()

    def checkResponse(self, response):
        for t in self.responseTypes:
            if isinstance(response, t):
                return

        raise Exception(f"CheckResponse({str(type(response))}) failed for command {str(type(self))}")

    def fullfil(self, response):
        self.checkResponse(response)
        self.finished.set_result(response)

class KernelResponse:
    def checkCommand(self, cmd):
        for t in self.commandTypes:
            if isinstance(cmd, t):
                return

        raise Exception(f"CheckCommand({str(type(cmd))}) failed for response {str(type(self))}")

class KernelDeviceListCommand(KernelCommand):
    def __init__(self):
        self.cmd = "DL"
        self.responseTypes = [ KernelDeviceListResponse ]
        super().__init__()

    def rawdata(self):
        return bytes()

class KernelDeviceListResponse(KernelResponse):
    def __init__(self, data):
        self.commandTypes = [ KernelDeviceListCommand ]
        self.list = [ d for d in self.parseMsg(data) ]
        super().__init__()

    def parseMsg(self, data):
        while len(data) != 0:
            if len(data) < 4:
                raise Exception(f"Too short data remaining: {len(data)}")
            T, = struct.unpack("=H", data[0:2])
            L, = struct.unpack("=H", data[2:4])
            if L > len(data) or L < 4:
                raise Exception(f"Garbled data length: {L} in {len(data)} bytes")
            yield Device.identify(T)(data[4:L])
            data = data[L:]

class KernelConnector:
    max_recv_buf = 256

    responses = {
            "DL": KernelDeviceListResponse,
            }

    def parseResponse(self, cmd, data):
        if cmd not in self.responses:
            log.error(f"Unknown response {cmd}")

        msg = self.responses[cmd](data)

        if isinstance(msg, KernelResponse):
            msg.checkCommand(self.pendingCommand)
            self.pendingCommand.fullfil(msg)
        else:
            self.model.gotMessage(msg)
    
    def __init__(self, model, filename):
        try:
            self.device_fd = os.open(filename, os.O_RDWR | os.O_NONBLOCK)
        except Exception as e:
            log.error(f"Opening the device {filename} failed with an exception {str(e)}")
            sys.exit(1)

        self.loop = asyncio.get_running_loop()
        self.loop.add_reader(self.device_fd, self.read)
        self.cmdLock = asyncio.Lock()
        self.pendingCommand = None
        self.model = model
#        loop.add_writer(self.device_fd, self.write)

    def read(self):
        bs = os.read(self.device_fd, self.max_recv_buf)
        if len(bs) < 4:
            raise Exception(f"Read less than 4 bytes from the filedescriptor")

        cmd = bs[0:2].decode(encoding='ascii')
        sz, = struct.unpack("=H", bs[2:4])
#        log.info(f"Read a packet of type {cmd} and size {sz}")
        
        if len(bs) != sz:
            raise Exception(f"Read a packet of size {sz} with declared length {len(bs)}")

        self.parseResponse(cmd, bs[4:])

    async def sendcmd(self, cmd):
        async with self.cmdLock:
            if self.pendingCommand is not None:
                raise Exception("Command sending lock broken")

            if len(cmd.cmd) != 2:
                raise Exception("Command must be exactly 2 characters long")

            rd = cmd.rawdata()
            bs = struct.pack("=2sH", cmd.cmd.encode(encoding='ascii'), len(rd) + 4)
            bs += rd

            if len(bs) > self.max_recv_buf:
                raise Exception(f"Trying to write more than {self.max_recv_buf}")

            self.pendingCommand = cmd

            os.write(self.device_fd, bs)
            await cmd.finished

            self.pendingCommand = None

    async def getDeviceList(self):
        cmd = KernelDeviceListCommand()
        await self.sendcmd(cmd)
        return cmd.response().list
