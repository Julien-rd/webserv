import asyncio
import aiohttp
import time
from collections import Counter

URL = "http://127.0.0.1:7070/login/"
CONCURRENCY = 1000
DURATION_SECONDS = 30          # How long clients stay connected
DELAY_BETWEEN_REQUESTS = 25.0   # Pause each client takes before hitting the server again
REQUEST_TIMEOUT = 10           # Per-request timeout (seconds)


async def worker(session: aiohttp.ClientSession, stop_event: asyncio.Event, status_counts: Counter):
    """A persistent worker that keeps reusing its pooled connection until stop_event fires."""
    while not stop_event.is_set():
        try:
            async with session.get(URL) as response:
                # Must drain the body, or aiohttp can't safely return the
                # connection to the pool and will close/reopen it instead.
                await response.read()
                status_counts[response.status] += 1
        except asyncio.TimeoutError:
            status_counts["timeout"] += 1
        except aiohttp.ClientError as e:
            status_counts[f"error: {type(e).__name__}: {e}"] += 1

        # Sleep, but wake up immediately if stop_event fires mid-sleep
        try:
            await asyncio.wait_for(stop_event.wait(), timeout=DELAY_BETWEEN_REQUESTS)
        except asyncio.TimeoutError:
            pass  # normal case: delay elapsed, loop again


async def main():
    connector = aiohttp.TCPConnector(limit=CONCURRENCY, limit_per_host=CONCURRENCY)
    timeout = aiohttp.ClientTimeout(total=REQUEST_TIMEOUT)

    async with aiohttp.ClientSession(connector=connector, timeout=timeout) as session:
        status_counts = Counter()
        stop_event = asyncio.Event()

        print(f"Starting {CONCURRENCY} persistent clients against {URL} for {DURATION_SECONDS}s...")
        start = time.time()

        tasks = [asyncio.create_task(worker(session, stop_event, status_counts)) for _ in range(CONCURRENCY)]

        await asyncio.sleep(DURATION_SECONDS)
        stop_event.set()
        await asyncio.gather(*tasks)

        elapsed = time.time() - start
        total_requests = sum(status_counts.values())
        success = status_counts.get(200, 0)
        errors = total_requests - success
        open_connections = len(connector._conns)  # rough signal that reuse is happening

        print(f"\nDone. Kept connections alive for {elapsed:.2f}s")
        print(f"Total Requests Sent: {total_requests} ({total_requests / elapsed:.1f} req/s)")
        print(f"Success: {success}, Errors: {errors}")
        print(f"Pooled connections still open at end: {open_connections}")
        if errors:
            print("Breakdown:")
            for status, count in status_counts.items():
                if status != 200:
                    print(f"  {status}: {count}")


if __name__ == "__main__":
    asyncio.run(main())