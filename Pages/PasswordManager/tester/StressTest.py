import asyncio
import aiohttp
import time

URL = "http://127.0.0.1:7070/login/"
NUM_REQUESTS = 1000
CONCURRENCY = 100

async def send_request(session, i):
    try:
        async with session.get(URL) as response:
            return response.status
    except Exception as e:
        return f"error: {e}"

async def main():
    connector = aiohttp.TCPConnector(limit=CONCURRENCY)
    async with aiohttp.ClientSession(connector=connector) as session:
        tasks = [send_request(session, i) for i in range(NUM_REQUESTS)]
        start = time.time()
        results = await asyncio.gather(*tasks)
        elapsed = time.time() - start

        success = sum(1 for r in results if r == 200)
        errors = [r for r in results if r != 200]

        print(f"Done in {elapsed:.2f}s ({NUM_REQUESTS/elapsed:.0f} req/s)")
        print(f"Success: {success}, Errors: {len(errors)}")
        if errors:
            print("Sample errors:", errors[:5])

asyncio.run(main())