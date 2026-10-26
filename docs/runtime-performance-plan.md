# Runtime performance plan

Base: game `0b015b64104f6e7ecfbd84c725d078021504508e`, engine `e353848`.

1. Trace tick/dispatch/bus/PPU/frame paths before edits; keep timing and fallback contracts.
2. Link a ROM-free harness against the actual runtime libraries, with an O1 caller and optimized runtime. Measure instruction boundaries, memory timing, EWRAM reads and dispatch hits separately.
3. Remove only measured redundant host calls in memory paths. Share canonical arithmetic rather than duplicate it. Compare baseline/candidate with identical inputs, outputs, and toolchain.
4. Exercise wait-state configurations, memory mirrors and side effects using existing bus/timer/DMA/PPU/audio/snapshot tests and a focused ROM-free timing comparison. Do not alter cycle values, event order, target FPS, pacing or generated shards.
5. Publish engine changes on an isolated branch and pin the private game branch to the reviewed engine commit. Keep both build recipes manual. Open a Draft PR against `perf/guest-shards-o1`; record limitations, especially no device FPS measurement or ROM execution.

Review focus: null active bus; live WAITCNT and memory-control writes; truncated access width; prefetch state unchanged by data-cost queries; IRQ/DMA event timing preserved.
