#!/usr/bin/python3

import asyncio
from proput.device import DeviceList

async def main():
    dl = DeviceList("/dev/proput")
    await dl.connect()
    for _ in range(10):
        await dl.reload()

if __name__ == "__main__":
    asyncio.run(main())
