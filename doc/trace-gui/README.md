# A native trace viewer

**Status:** design note. The *back end described here already exists and was verified
working on 2026-10-10.* What is missing is a client, one emitter, and three bug fixes.
Proposed 2026-10-10.

> I always wanted a graphical user interface for the visualization of an execution trace
> (static traces but also live ones). The GUI should use raylib and run natively.

The headline finding of this note is that the expensive half of that wish was built in
March and April 2024 and then left alone. There is a record vocabulary, a binary wire
format with nanosecond timestamps and monotonic ids, a TCP server with an incremental
pull protocol, a state-name dictionary, and a working reference client. All of it is in
the build. All of it runs today.

So this is not a proposal to build a tracing system and a GUI. It is a proposal to
**write a client.**

<a name="exists"></a>

## What already exists

Every row below was checked against the source on 2026-10-10, and the live path was
additionally checked by running it (see [Verification](#verification)).

| Piece | Location | State |
|---|---|---|
| Record vocabulary | `core/include/sm_livelog_storage_ids.hpp:10-17` | 8 record kinds declared |
| Ring-buffer storage, ns timestamps, monotonic ids | `core/src/livelog/livelogger.cpp:181` | works |
| TCP server | `core/src/livelog/livelogger.cpp:247,253` | works |
| Incremental pull protocol with resume cursor | `core/src/livelog/livelogger.cpp:339,358` | works |
| State-id interning and `IDX2FQS` dictionary | `core/src/state_machine_simulation_core_buildsms.cpp:1600-1605` | works |
| Reference client | `sm4ceps::Livelogger_sink`, `core/src/sm_livelog_storage_utils.cpp:220` | ~150 lines, copyable |
| Call sites in the simulation loop | `core/src/sm_sim_core_simulation_loop.cpp:861,918,922` | wired |
| Makefile targets | `Makefile:64-65,159-162` | built into `bin/ceps` |
| Command-line switch | `core/src/cmdline_utils.cpp:235` | `--live_log` |

The last commit touching this subtree is `4561bf5` *[FEATURE] html/jscript/websocket
interplay*. The code has been sitting there since.

### The record vocabulary

From `core/include/sm_livelog_storage_ids.hpp`:

| constant | value | payload |
|---|---|---|
| `STORAGE_WHAT_EVENT` | 3 | event name plus a `Variant` parameter list |
| `STORAGE_WHAT_CURRENT_STATES` | 4 | array of `int32` state indices |
| `STORAGE_WHAT_CONSOLE` | 5 | string |
| `STORAGE_WHAT_INFO` | 6 | string |
| `STORAGE_WHAT_WARNING` | 7 | string |
| `STORAGE_WHAT_ERROR` | 8 | string |
| `STORAGE_WHAT_TRANSITION` | 9 | **declared, never written — see [the gap](#gap)** |
| `STORAGE_WHAT_INT32_TO_STRING_MAP` | 10 | dictionary record |

Plus two storage ids: `STORAGE_IDX2FQS` and `STORAGE_IDX2FQS_FLUSH`.

### The wire format

Each record is a packed `chunk` header (`core/include/livelog/livelogger.hpp:40`)
followed by its payload:

```c
struct chunk {
    size_t   len_;             // payload length
    ssize_t  id_;              // monotonically increasing
    int32_t  what_;            // one of STORAGE_WHAT_*
    uint64_t timestamp_secs;
    uint64_t timestamp_nsecs;
} __attribute__((packed));
```

Monotonic ids *and* nanosecond timestamps, per record. That is precisely the pair a
timeline view needs: the id gives a total order that survives clock weirdness, the
timestamp gives the x-axis.

### The protocol

Better than it had any right to be. It is a **pull protocol with a resume cursor**:

1. Client connects to the port (default `3000`).
2. Client writes a `uint32` command.
3. Server replies with a sequence of `[len][chunk header][payload]` triples, terminated
   by a zero-length sentinel (`livelogger.cpp:339-356`).

The command is interpreted as follows (`livelogger.cpp:358`):

| command | meaning |
|---|---|
| `CMD_FLUSH_MAIN_LOG_STORAGE` = 0 | flush |
| `CMD_GET_NEW_LOG_ENTRIES` = 1 | every chunk with `id > last_transmitted_id` |
| *any registered storage id* | the whole of that storage |

The third row is the interesting one and is not obvious from the code. The server keeps
a registry of named storages; sending a storage id as the command dumps it. That is how
a client fetches the state-name dictionary: send `STORAGE_IDX2FQS`.

The server tracks `last_transmitted_id` **per connection**, so repeated
`CMD_GET_NEW_LOG_ENTRIES` polling yields exactly the new records and nothing else. No
deduplication is needed on the client side.

### The dictionary is the hierarchy

`state_machine_simulation_core_buildsms.cpp:1602-1605` registers a map from state index
to fully qualified state name, built from `executionloop_context().state_id_to_idx`.
The names are **dotted paths**:

```
partition_sm_vp_motor_temperatur
partition_sm_vp_motor_temperatur.Initial
partition_sm_vp_motor_temperatur.niedrig
partition_sm_vp_motor_temperatur.mittel
```

So the state *hierarchy* is recoverable from the dictionary alone, by splitting on `.`.
The GUI needs no separate model export in order to draw a state tree. This is worth
saying out loud, because the obvious alternative — teaching the viewer to parse `.ceps`,
or adding a model-export format alongside the trace format — is a large amount of work
that turns out to be unnecessary.

<a name="verification"></a>

## Verification

The standing rule in this repository is that a measurement is only citable alongside a
stated attempt to produce the opposite result.

**Claim:** `--live_log` opens a listening socket, and the simulator otherwise runs
normally.

**Attempt to falsify.** The first attempt *appeared* to refute it: a run with
`--live_log` followed by `ss -ltn` four seconds later showed nothing on port 3000 and no
`ceps` process. That is a false negative — the Lüftersteuerung example completes in well
under a second, so the socket had already closed. The second attempt polled every 20 ms
for the lifetime of the process:

```sh
cd examples/doing_specs/lueftersteuerung
( for i in $(seq 1 200); do
    ss -ltn 2>/dev/null | grep -q ':3000' && { echo "PORT 3000 OPEN at poll $i"; break; }
    sleep 0.02
  done ) &
bin/ceps .ceps/prelude.ceps spec/main.ceps \
         tests/monotonically_increasing_temperature_1.ceps --live_log
```

Result: `PORT 3000 OPEN at poll 2`, `ceps EXIT=0`, and no `fatal`/`getaddrinfo`/`bind`/
`listen` diagnostics in the output. The socket is up roughly 40 ms after start.

**What this does and does not establish.** It establishes that the dispatcher thread
starts, binds and listens, and that enabling it does not disturb the simulation. It does
**not** establish that a client can complete a transaction, because no client was
connected. That is milestone zero below, and until someone has pulled bytes the protocol
should be treated as unproven.

Also recorded, because the negative result is useful: the short-lived process means that
**any** interactive testing of the live path needs a model that runs long enough to
connect to. Either a timer-driven model, or a `--wait`-style switch that holds the
process open after the simulation ends. The latter does not currently exist.

<a name="gap"></a>

## The one real gap: transitions are never emitted

`STORAGE_WHAT_TRANSITION = 9` is declared, and nothing in the tree writes it:

```
$ grep -rn "STORAGE_WHAT_TRANSITION" --include=*.cpp --include=*.hpp .
./core/include/sm_livelog_storage_ids.hpp:16:  ... STORAGE_WHAT_TRANSITION = 9;
```

One hit: the declaration. What is logged instead is a **snapshot of the current-state
set** on every step (`sm_sim_core_simulation_loop.cpp:861,918`) and the event that was
read (`:922`).

From snapshots you can compute that the configuration changed, and by differencing
consecutive snapshots you can even infer which states were entered and left. What cannot
be recovered is:

- **which** transition fired, when several could have
- the guard that admitted it, or the guards that did not
- the action sequence it ran
- in an orthogonal composition, which region the change belongs to

That is the difference between a viewer that shows *what happened* and one that shows
*why*, and the second is the entire reason to build the thing. This is the single piece
of C++ work the proposal requires.

The information is available at the point of logging: the execution-loop context already
carries `transitions` and `shadow_transitions`
(`state_machine_simulation_core_buildsms.cpp:1595-1597`). The textual output already
prints enter/exit in the `+`/`-` notation:

```
partition_sm_vp_motor_temperatur.Initial- partition_sm_vp_motor_temperatur.niedrig+
```

The livelog should carry the same information structurally rather than as text.

<a name="one-stream"></a>

## Static and live are the same problem

This is the structural gift in the existing design, and the thing to exploit hardest.

A chunk stream is a chunk stream. The server hands it over a socket;
`core/src/trace.cpp` shows the same family of records persisted to a memory-mapped file
(`log4kmw::persistence::memory_mapped_file("trace.bin", 8*1024*1024, false)`). The
format does not change with the transport.

**Therefore: one iterator over chunks, two sources.** Static playback is live playback
with the socket swapped for an `mmap`. There is no reason to design, test or maintain two
viewers.

Push it one step further than seems necessary: have the GUI **always append incoming
chunks into an arena, and always render from the arena**, never directly from the socket.
Live then means nothing more than "the arena is still growing". Scrubbing backwards
through a running trace falls out for free, and so does saving a live session to a file,
because the arena *is* the file format. That is the feature people want from a live
viewer and rarely get, and here it costs nothing, because the arena has to exist anyway.

### The constraint that makes this mandatory

`Storage::push_back` (`core/src/livelog/livelogger.cpp:181-185`):

```c
auto tot_len = len+sizeof(chunk);
if (tot_len + 1 > len_) return std::make_pair(false,0);
while (tot_len > available_space()) pop();        // <-- silently drops the oldest
```

The server's buffer is a **ring that discards history to make room**. There is no
back-pressure and no notification. A long run will quietly lose its beginning.

So the arena is not an optimisation, it is a correctness requirement: **whatever the
client does not keep is gone, and the client cannot ask for it again.** This is the
easiest thing in the whole design to get wrong, because it works perfectly in every short
test.

<a name="raylib"></a>

## raylib

The right call, for reasons that are specific rather than general.

**In favour.** A single static binary with no toolkit dependency matches what
[`POSITIONING.md`](../../POSITIONING.md) already claims as an advantage — *"no setup, no
boilerplate, a single binary"* — and a viewer that needed a GTK or Qt installation would
undercut it. Immediate mode is the natural fit for the actual drawing problem, which is
"render the current viewport from an array"; a retained-mode widget tree would be fought
rather than used. And it is one `.a`.

**Three caveats, in increasing order of how much time they will cost.**

1. **There is no widget toolkit.** raygui is a single header and is basic. A scrollable
   tree of several thousand states, text selection, a usable text input, a splitter —
   those get written by hand.

2. **Text is most of the pixels.** A trace viewer is small monospaced labels everywhere.
   Decide the font strategy early (a fixed-size bitmap atlas, or SDF if arbitrary zoom is
   wanted). Retrofitting text rendering after the layout is built is miserable.

3. **The work is not raylib, it is culling.** Immediate mode redraws everything every
   frame, so a hundred thousand chunks means binning and level-of-detail against the
   viewport, recomputed per frame. This is where the time will actually go, and it is
   independent of which library draws the rectangles.

**The alternative, stated honestly.** Dear ImGui is the conventional answer for exactly
this class of tool, and brings mature widgets, docking and ImPlot. It needs a rendering
backend and is more template-heavy than the house style. `rlImGui` exists if both are
wanted later. raylib plus hand-rolled is the better match for how this repository is
written; the point of recording the alternative is that caveat 1 above is precisely the
thing ImGui would have solved.

<a name="draw"></a>

## What to draw

Three panes, of which one is the product.

### Left: the state tree

Built from the `IDX2FQS` dictionary by splitting fully qualified names on `.`. Active
states highlighted. This answers *where am I*.

### Centre: a swimlane timeline

One lane per state, a bar for each interval during which that state is active, events as
vertical ticks crossing all lanes, transitions as arrows between lanes once
[the gap](#gap) is closed.

A Gantt chart, essentially — and it is the correct shape for *hierarchical* machines
specifically, because orthogonal regions become parallel lanes. Concurrency becomes
something seen rather than inferred. A single "current state" readout, which is the
obvious first idea, cannot represent an orthogonal composition at all.

**Offer both x-axes, and make switching cheap:**

| axis | source | answers |
|---|---|---|
| step index | chunk `id_` | logical questions: ordering, causality, which step |
| wall-clock | `timestamp_secs/nsecs` | timing questions: latency, jitter, which step was slow |

These are different questions, and conflating them is the classic mistake in trace
tooling. The data supports both already; nothing needs to be added.

### Bottom: the inspector

The selected chunk, in full: event name and decoded `Variant` parameters, with console,
info, warning and error records interleaved in timestamp order.

### Live affordances

A **follow** toggle that auto-scrolls to the newest record and *disengages automatically
when the user scrubs backwards* — the `tail -f` affordance. Re-engaging should jump back
to now rather than animating there.

<a name="bugs"></a>

## Three bugs found while reading

These are pre-existing, in code that no current test exercises. They are recorded here
rather than in `DEFECTS.md` because they are latent in a subsystem the rest of the tree
does not use — but any client will exercise all three.

### 1. Race in `publish()` — fails silently

`core/src/livelog/livelogger.cpp:247-251`:

```c
void livelog::Livelogger::publish(std::string port){
  if (comm_stream_dispatcher_thread_ != nullptr) return;
  comm_stream_dispatcher_thread_ = new std::thread(&Livelogger::comm_stream_dispatcher_fn,this);
  port_ = port;                                  // <-- after the thread is launched
}
```

`comm_stream_dispatcher_fn` reads `port_` and calls
`getaddrinfo(nullptr, port_.c_str(), ...)`. If the new thread is scheduled before line
250 lands, it resolves the empty string, `getaddrinfo` fails, `fatal()` is called, and the
dispatcher **returns without listening**. The simulation continues normally and the
socket simply never appears.

It lost the race in testing — thread creation is slower than a string assignment on this
machine — which is exactly why it is dangerous. Swap the two lines.

This belongs to the same family as the silent failures catalogued in `DEFECTS.md`: the
failure mode is *nothing visible happens*.

### 2. Inverted null check leaks the storage buffer

`core/include/livelog/livelogger.hpp:68`:

```c
~Storage() {if (data_ == nullptr) delete[] data_;data_=nullptr;}
```

Deletes only when there is nothing to delete; leaks whenever there is. The condition is
inverted. Note that `Storage` is movable, so the fix has to handle the moved-from state at
the same time.

### 3. `CLOCK_REALTIME` for interval measurement

`core/src/livelog/livelogger.cpp:194` stamps records with `CLOCK_REALTIME`, which is
subject to NTP steps and can move backwards. A viewer that sorts or measures intervals by
timestamp can therefore see negative durations.

`CLOCK_MONOTONIC` is the right clock for durations. If wall-clock time is also wanted —
and it is, for correlating against external logs — record both, or record one
`CLOCK_REALTIME` epoch at startup and monotonic deltas thereafter.

### Also worth fixing, though not a bug

`live_log_port` exists as a field (`core/include/cmdline_utils.hpp:78`, default `"3000"`)
but **no command-line argument sets it**. `cmdline_utils.cpp:235` parses `--live_log` as a
bare flag. One line to add `--live_log_port`.

<a name="milestones"></a>

## Milestones

**Milestone zero contains no raylib.** This is the main piece of process advice in the
note.

| # | Deliverable | Why this order |
|---|---|---|
| 0 | A ~150-line console client: connect to `:3000`, send `STORAGE_IDX2FQS`, then poll `CMD_GET_NEW_LOG_ENTRIES`, print decoded chunks | Proves the protocol in isolation. Starting in raylib means debugging the protocol and the renderer simultaneously, with no way to tell which is lying |
| 1 | The three bug fixes, plus `--live_log_port` | Cheap, and milestone 0 is what will surface them |
| 2 | Chunk arena plus a file source; dump and replay a static trace | Establishes the [one-stream](#one-stream) design before any UI depends on it |
| 3 | raylib window: state tree from the dictionary, event list | First pixels. Deliberately not the timeline |
| 4 | The swimlane timeline with viewport culling, both x-axes | The product. Also the hard part |
| 5 | Live source behind the same arena, follow toggle | Should be small if milestone 2 was done honestly |
| 6 | Emit `STORAGE_WHAT_TRANSITION`; draw transition arrows | The viewer becomes a *why* tool rather than a *what* tool |

<a name="later"></a>

## What not to architect away

Do not build this now. Do not make it impossible either.

The highest-value version of this tool does not show one trace. It shows **two traces
aligned**:

- **Concept against implementation.** Shadow states already compute exactly this
  relation — `core/src/sm_sim_core_shadow_states.cpp` requires the shadow map to be a
  simulation relation, and per `POSITIONING.md` the `test/alloy/` and `test/agda/`
  material goes further with a `symbolic_equality` primitive returning a structured
  difference. A viewer that shows the implementation trace against the concept trace,
  with the conformance obligation drawn between them, visualises a check that is already
  implemented and that currently reports only as a pass or an abort.

- **Revision *N* against revision *N+1*.** Per
  [`doc/model-revision/README.md`](../model-revision/README.md), a revised model produces
  a trace that differs from its predecessor's in ways that note argues should be
  *diagnosable* rather than merely detected. The viewer is where that diagnosis would be
  read.

The practical consequence for the architecture is small and worth paying now: make the
arena, the lane layout and the viewport own a **trace id**, so that "two traces" is a
widening rather than a rewrite.

<a name="criteria"></a>

## Acceptance criteria

1. **The protocol is exercised end to end by a non-GUI client.** A console tool connects,
   retrieves the dictionary, polls for new entries and prints decoded records, with a test
   that fails if the socket never opens. Until this exists, the live path is unproven, not
   working.

2. **One chunk iterator, two sources.** Socket and file sources are interchangeable behind
   a single interface, and the timeline renderer cannot tell which it is reading.
   Demonstrated by rendering the same trace both ways and diffing the result.

3. **The client persists everything it receives.** Given the server's ring buffer
   ([above](#one-stream)), a session longer than the buffer capacity is replayable from
   the beginning in the GUI. A test writes more than the ring capacity and asserts that
   record id 0 is still displayable.

4. **The state tree is derived from the dictionary alone.** No `.ceps` parsing, no second
   export format. Adding a state to the model and re-running changes the tree with no
   change to the viewer.

5. **Both x-axes are available, and switching preserves the selection.** The selected
   record stays selected and stays visible across an axis switch.

6. **Viewport culling is measured, not assumed.** A trace of at least 10<sup>5</sup>
   records renders at an interactive frame rate, stated as a number on named hardware,
   with the measurement reproducible from a committed trace file.

7. **`STORAGE_WHAT_TRANSITION` carries enough to answer "why".** At minimum: source state,
   target state, triggering event, and the identity of the transition in
   `executionloop_context().transitions`. It is sufficient when a reader can distinguish
   two transitions that produce the same current-state snapshot.

8. **Live is a source, not a mode.** No branch in the renderer tests for liveness. The
   follow toggle is the only live-specific behaviour, and it lives in the viewport, not in
   the data path.

9. **The three bugs are fixed, the race first.** The `publish()` ordering is the one that
   fails silently and therefore the one that will waste a day.

<a name="open"></a>

## Open questions

1. **Is `Variant` decoding in the client worth generating?** `storage_read_variant` exists
   on the C++ side (`core/include/sm_livelog_storage_utils.hpp`). A separate viewer binary
   can link the same translation unit — but that couples the viewer to the simulator's
   build. Link it, or re-implement the format and accept the risk of drift?

2. **Should the viewer be a separate binary at all,** or `bin/ceps --trace-gui`? Separate
   keeps raylib out of the simulator's dependency set, which is the stronger argument. But
   then the trace file format becomes an interface with two owners.

3. **What holds a short simulation open?** Noted under [Verification](#verification):
   there is currently no way to keep the process alive for a client to attach to. A
   `--live_log_wait` switch is the obvious answer. Whether the simulation should also
   *block* until a client attaches is a separate question, and answering it yes would make
   the viewer usable as a debugger rather than only as an observer.

4. **Does the websocket API overlap with this?** `core/src/api/websocket/ws_api.cpp` and
   `core/src/websocket.cpp` are built, and were the subject of the last commit in this area
   (`4561bf5`). If the websocket path already carries trace data to a browser then there
   are two transports, and which one is canonical should be settled before a second client
   is written against the older one.

5. **Does anything still read `trace.bin`?** `core/src/trace.cpp` is a 23-line `main()`
   that memory-maps a `log4kmw` trace and prints it as ceps. Its relationship to the
   `livelog` storages is unclear and may be vestigial.

<a name="see-also"></a>

## See also

- [`doc/model-revision/README.md`](../model-revision/README.md) — the revision design whose
  diagnosis story would be read in this viewer
- [`POSITIONING.md`](../../POSITIONING.md) — the single-binary, no-setup claim that argues
  for raylib over a toolkit
- [`DEFECTS.md`](../../DEFECTS.md) — the silent-failure family that bug 1 belongs to
- `core/src/livelog/test.cpp` — an existing standalone harness that drives a
  `Livelogger_source` with synthetic states and events; a trace generator for GUI work that
  needs no model at all
