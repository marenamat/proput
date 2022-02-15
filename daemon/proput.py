#!/usr/bin/python3

import asyncio
import websockets
from proput.device import DeviceList

async def main():
    dl = DeviceList("/dev/proput")
    await dl.connect()
    await websockets.serve(dl.websocket, host="", port=8099)

if __name__ == "__main__":
    asyncio.get_event_loop().run_until_complete(main())
    asyncio.get_event_loop().run_forever()
