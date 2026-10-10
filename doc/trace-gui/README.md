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
- **transition coverage** — which transitions were never taken

That is the difference between a viewer that shows *what happened* and one that shows
*why*, and the second is the entire reason to build the thing. This is the single piece
of C++ work the proposal requires.

The last item deserves separate mention because it blocks a view rather than a detail.
*State* coverage is derivable client-side with no back-end change at all
([the treemap section](#colour) shows how). *Transition* coverage is not derivable from
anything currently on the wire, so the coverage map can colour states and not transitions
until this record exists — which will look half-finished in exactly the way that invites
the question.

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

**Two views, and they are duals rather than alternatives.**

The timeline spends its two dimensions on *time × flattened states*. The treemap
([below](#treemap)) spends its two dimensions on *hierarchy*, with time as a parameter.
Each is blind exactly where the other sees: the timeline cannot show containment, and the
treemap cannot show duration. Neither is a weaker version of the other, and building only
one is the mistake.

The coupling that makes them work together is simple and should be decided now, because
it constrains the data model: **the treemap is a function of a cursor position in the
trace, and the timeline is the cursor.** Scrub in the timeline, the treemap updates.
Select in the treemap, the timeline filters. Everything else is layout.

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

<a name="treemap"></a>

## The treemap: global state at a glance

> Another alternative view which carries less information of course but gives a bird's eye
> view of the global state is a treemap where the areas correspond to state machines and
> their sub state machines, collapsable, and a color scheme which is load-bearing.

A space-filling view of the containment hierarchy: each state machine is a rectangle,
its sub-machines subdivide it, collapsible at every level.

The mapping is natural rather than forced, because the model *is* a containment hierarchy
and the dictionary already carries it as dotted paths
([above](#exists)). And it has one structural advantage over the timeline that is easy to
miss: **it is O(1) in trace length.** It renders one configuration, so it stays legible at
10<sup>6</sup> records, where the timeline needs binning and level-of-detail to survive.

<a name="static-area"></a>

### The rule: static area, dynamic colour

The temptation is to make area load-bearing as well — area proportional to residence time,
or to transition count, so that the biggest rectangle is where the machine spends its
life. **Resist it.**

A treemap's real power in a monitoring context is that it becomes a *memorised map*. After
a few sessions the reader stops parsing labels and starts reading position: anomaly
detection collapses into "the top-left is wrong", which is pre-attentive and essentially
free. The moment the layout moves, that is gone, and every frame has to be re-read from
scratch.

The second argument is independent and decides it even if the first is unconvincing: **if
colour is load-bearing, area must be stable**, or there are two channels in motion at once
and neither can be read.

So: **area = descendant count, computed once from the dictionary, never recomputed.**

This also dissolves the standard treemap headache. Layout algorithms trade aspect ratio
against stability — squarified layouts (Bruls, Huizing and van Wijk, 2000) give pleasant
proportions but reorder when the data changes, while slice-and-dice is stable but produces
slivers. With static areas the question evaporates: compute a squarified layout once at
load time and never think about it again.

<a name="colour"></a>

### What colour should carry, and one answer is free

Candidates, and they compete for a single channel:

| encoding | good for | cost |
|---|---|---|
| active / inactive | nothing — one bit on a high-bandwidth channel | wasteful |
| recency decay | **live**: watch activity propagate through the hierarchy | needs a decay constant to tune |
| cumulative residence | where the machine spends its life | needs a full pass; meaningless live |
| transitions fired in subtree | activity density, hot spots | needs [the missing record](#gap) |
| **coverage: visited / never visited** | **test adequacy** | **none — derivable today** |
| conformance (shadow states) | concept-vs-implementation divergence | needs shadow data in the stream |

**Build coverage first, because it costs nothing on the back end.**

The dictionary enumerates *every* state in the model — `state_id_to_idx` is built at
model-build time, not at run time, so it contains states the run never reached. The
`STORAGE_WHAT_CURRENT_STATES` stream gives the states that *were* reached. Union the
stream, divide by the dictionary, and state coverage falls out **entirely client-side,
with no change to the simulator.**

The simulator already prints the number:

```
State Coverage: 0.75 ( 75% )
Transition Coverage: 0.166667 ( 16.6667% )
```

The treemap would show *which* 25%, as a map rather than a scalar. That is a view the
timeline structurally cannot produce, because never-visited states have no bars — they are
invisible in a Gantt chart by construction. It is the strongest single argument for
building both views.

Note the second line, though: **transition coverage needs
[the missing record](#gap).** The gap bites in a second place, and this is where it will
be felt first, because a coverage map that can colour states but not transitions is
visibly half-finished.

<a name="xor-and"></a>

### The honest objection: XOR and AND look identical

This is specific to state machines, and generic treemap tooling will not solve it.

A treemap draws *containment*. It does not distinguish a composite state, where exactly
one child is active, from an orthogonal region, where several are active at once. Three
lit cells therefore mean either healthy concurrency or a serious bug, and the picture
cannot say which.

That has to be a **drawn convention, decided before anything is rendered** — dashed
dividers between orthogonal regions, say, with XOR children drawn as a radio-set so that
more than one highlighted child is immediately visibly wrong. It is the distinction that
makes this a state-machine treemap rather than a disk-usage chart, and it is not optional:
without it the view actively misleads on exactly the cases worth looking at.

<a name="treemap-details"></a>

### Two cheap wins and one channel to reserve

**Cushion shading** (van Wijk and van de Wetering, 1999) — a per-cell gradient suggesting
a rounded surface — makes nesting depth legible past three levels, where plain nested
rectangles stop being parseable. In raylib it is a gradient per cell and nothing more.

**Reserve one channel for linking.** Outline, not fill, for "this is what the timeline has
selected". If colour is spent on coverage or heat, selection needs a channel of its own,
or the two views cannot point at each other — which was the whole architecture.

**Colour scheme discipline**, since the scheme is meant to be load-bearing:

- perceptually uniform ramps (viridis, magma) for continuous quantities, never a rainbow —
  rainbow ramps invent boundaries that are not in the data
- distinct hues for categorical status, with the *bad* category most salient
- never two quantities on one channel; status beside heat means border, hatch or badge
- red/green as the pass/fail pair fails for roughly 8% of men

<a name="small-multiples"></a>

### The payoff that only a static layout allows

Render the treemap at thumbnail size, once per time bin, and lay forty of them out in a
strip: **the entire run's global-state evolution at a glance.**

Small multiples work here precisely *because* the layout never moves — every thumbnail
uses the same floorplan, so differences between them are differences in the data rather
than in the layout. The moment area becomes dynamic this view is worthless, which is the
third independent argument for [static area](#static-area).

It is also the view most worth having after a long overnight simulation, and it is nearly
free once the treemap renders at all.

<a name="heatmap"></a>

## The hierarchical heatmap

Once colour is load-bearing, the treemap *is* a hierarchical heatmap: area carries the
structure, colour carries the quantity. Naming it separately is still worthwhile, because
it surfaces a question the treemap framing hides, and that question has no neutral answer.

<a name="aggregation"></a>

### Aggregation under collapse

The view is collapsible. So a collapsed cell must show **one** colour standing for **N**
descendants, and the aggregation function is a design decision:

| function | reads as | fails when |
|---|---|---|
| max | "something in here is hot" | one hot leaf in a thousand paints the whole parent; proportion is lost entirely |
| mean | proportional | the one hot leaf vanishes into 999 cold ones — precisely the case the tool was opened for |
| sum, area-weighted | honest about totals | colour and area then encode correlated quantities, half-wasting a channel |
| count over threshold | "how much of this is hot", which is usually what is meant | needs a threshold, which is another decision |

The recommendation is **max for anomaly colourings, area-weighted mean for quantity
colourings, and the choice visible in the interface rather than buried in a constant.**
The two answer different questions, and a reader who does not know which is active will
misread the picture confidently — which is worse than not reading it.

<a name="normalisation"></a>

### Normalisation, which is the sharper problem

A heatmap over a hierarchy carries an implicit normalisation, and all three available
answers are wrong in different ways:

- **Global range.** Deep leaves are then almost always cold, because the interesting
  variation is local and gets crushed by whatever the global maximum happens to be.
- **Sibling-relative.** Every parent now has a red child, including the parents where
  nothing is happening at all. The view manufactures signal.
- **Per level.** A third wrong answer, with the single merit that it fails *legibly* —
  the reader can at least see that comparisons across levels are meaningless.

There is no fourth option in general. The escape is to choose quantities that do not have
the problem.

<a name="coverage-dodges"></a>

### Why coverage dodges all of it

Coverage is a **count with a natural denominator**: visited leaves over total leaves in
the subtree.

- Aggregation is unambiguous — sum both numerator and denominator.
- Normalisation is unambiguous — the value is already a ratio in [0, 1].
- The parent's value is genuinely meaningful rather than a summary artifact. "This subtree
  is 40% covered" is a true statement about the subtree, not a lossy compression of its
  children.

That is a second and stronger reason to build coverage first, beyond it being
[free on the back end](#colour): it is the one quantity for which the hierarchical heatmap
has **no open design questions at all.** Every other colouring inherits both problems
above.

<a name="diff"></a>

### Difference maps

The same structure gives a difference view for almost nothing:

> colour = coverage(run A) − coverage(run B)

which answers *what did this test add* and *what did this revision stop exercising*. It is
the concrete, buildable form of the two-traces-aligned idea filed under
[what not to architect away](#later) — and unlike the shadow-state version it needs no new
data, only two traces and a subtraction.

**At leaves the difference is categorical, not continuous.** There are four cases, and
only three of them lie on the difference axis:

| case | difference | meaning |
|---|---|---|
| covered in both | 0 | no change |
| covered in A only | +1 | **lost** in B |
| covered in B only | −1 | **gained** in B |
| covered in neither | 0 | *a persistent gap, which is not the same as "no change"* |

The fourth row is the trap. It has the same numeric difference as the first, and it is the
one the reader most needs to see. It needs a channel of its own — hatching, or a
desaturated fill — because it is not a point on the difference ramp at all.

The continuum only appears **above the leaves**: at a collapsed node the difference is a
real number in [−1, +1], and that is where the [aggregation](#aggregation) choice starts
to matter again.

<a name="scheme"></a>

## The colour scheme

The scheme has to serve both the absolute view and the difference view without the reader
learning two of them. That is a real constraint and it drives everything below.

<a name="ink"></a>

### The principle: ink is proportional to attention required

The naive unification fails, and it is worth seeing why. Absolute coverage and difference
have opposite *zeros*:

- absolute view: 0 = never visited = **the thing being hunted**, so zero must be loud
- difference view: 0 = unchanged = **the boring case**, so zero must be quiet

A single ramp anchored at zero therefore cannot serve both — unless the absolute view
encodes the **deficit** rather than the quantity:

> absolute view colours `1 − coverage`, not `coverage`.

Now both views agree: **the background means "nothing to see here", and ink means "look".**
Fully covered subtrees fade out; gaps glow. Unchanged subtrees fade out; changes glow. One
semantic, learned once, and it is the right one for a monitoring tool — the view is not
drawing coverage, it is drawing *deficit and change*.

The construction rule follows directly, and it is more useful than a fixed palette:

> **The neutral midpoint of the ramp is the panel background colour itself.**

Zero is then literally invisible, the eye goes straight to the ink, and a light-background
variant for screenshots and slides falls out by swapping one anchor instead of designing a
second scheme.

<a name="hues"></a>

### The hues: blue and orange, and never green

| direction | hue | meaning, in both views |
|---|---|---|
| positive | orange | **worse** — uncovered, or coverage lost |
| negative | blue | **better** — coverage gained |
| zero | panel background | nothing to report |

Orange always means worse and blue always means better, in the absolute view and the
difference view alike. That consistency is the point; the hues themselves are chosen under
three constraints:

1. **Colour-vision deficiency.** Blue–orange is the most robust diverging pair across
   protanopia, deuteranopia and tritanopia. Red–green is the single most common mistake in
   this kind of tooling and would fail for roughly 8% of men. Green is therefore
   unavailable for "good", which is why blue takes that role even though it reads slightly
   against convention.
2. **Perceptual uniformity.** Build the ramp by interpolating in **OKLab** (or CIELAB)
   from the background anchor to each endpoint, holding chroma monotone. Interpolating in
   sRGB produces banding and false boundaries, which is the same defect that makes rainbow
   ramps unusable.
3. **Symmetry.** The difference ramp must be **symmetric and anchored at zero**, scaled by
   `max(|Δ|)`, *even when every change is positive*. An asymmetric range moves the neutral
   point off zero, and the picture then lies about which cells are unchanged.

Published diverging maps — Crameri's `vik` or `broc`, Moreland's cool–warm, ColorBrewer
`RdBu` — are all white-centred and so are not directly usable on a dark panel. They are
the right *reference*: take one, and re-anchor its midpoint to the background.

<a name="scheme-catches"></a>

### Two catches

**Background-coloured cells are invisible as cells.** When zero fades into the panel, the
treemap's floorplan disappears along with it — which defeats the memorised-map property
that [static area](#static-area) exists to protect. Structure must therefore be carried by
something other than fill: **thin low-contrast borders carry the floorplan, fill carries
the quantity.** This is not optional once the midpoint is the background.

**Cushion shading and a perceptual ramp compete for luminance.** Cushion shading
([above](#treemap-details)) conveys nesting depth by modulating luminance, and a
perceptually uniform ramp also varies luminance along its length. Used at full strength
together they corrupt each other. The honest resolutions are to run cushions at low
amplitude, or to enable them only in the structural mode where no quantity is being
coloured. Pick one deliberately rather than discovering the interference later.

<a name="scheme-practical"></a>

### Practical notes

- **Bake the ramp into a 256-entry lookup table at startup.** Per-cell OKLab conversion
  every frame is pointless work in an immediate-mode renderer that redraws everything
  anyway.
- **Keep the outline channel reserved** for timeline linking, as
  [above](#treemap-details). With fill spent on the quantity and borders spent on
  structure, outline is the only channel left for selection.
- **The fourth difference category** ([above](#diff)) needs hatching or desaturation — a
  texture, not a hue, since every hue is already committed.

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
| 0 | A ~150-line console client: connect to `:3000`, send `STORAGE_IDX2FQS`, then poll `CMD_GET_NEW_LOG_ENTRIES`, print decoded chunks | Proves the protocol in isolation. Starting in raylib means debugging the protocol and the renderer simultaneously, with no way to tell which is lying. Also where the [reload boundary](#hotreload) is settled, because this is the code that ends up on the host side of it |
| 1 | The three bug fixes, plus `--live_log_port` | Cheap, and milestone 0 is what will surface them |
| 2 | Chunk arena plus a file source; dump and replay a static trace | Establishes the [one-stream](#one-stream) design before any UI depends on it |
| 3 | raylib window: state tree from the dictionary, event list | First pixels. Deliberately not the timeline. Put the [reload boundary](#hotreload) in on day one, not after ramp-tuning has become painful |
| 3a | **Treemap, static layout, coloured by state coverage** | Cheapest real view in the note: layout from the dictionary, colour from `CURRENT_STATES`, no back-end change. Produces something useful before the timeline exists |
| 3b | **Difference map: two traces, one subtraction** | Needs nothing beyond 3a and a second arena. The highest value-per-line item in the note, and the only form of [trace alignment](#later) buildable today |
| 4 | The swimlane timeline with viewport culling, both x-axes | The product. Also the hard part |
| 5 | Live source behind the same arena, follow toggle | Should be small if milestone 2 was done honestly |
| 6 | Emit `STORAGE_WHAT_TRANSITION`; draw transition arrows | The viewer becomes a *why* tool rather than a *what* tool |
| 7 | Treemap/timeline cursor linking; small multiples strip | Both cheap once 3a and 4 exist, and the [duality](#draw) only pays off when they are coupled |

<a name="hotreload"></a>

### The reload boundary: decided at milestone 0, paid for at milestone 3

Milestone 0 produces no pixels, but it settles who owns what, and that split is what makes
the rendering work later either pleasant or miserable. Decide it now:

| Host process — never reloaded | Reloadable plugin |
|---|---|
| The socket, its cursor, the decode loop | Layout, colour, the frame |
| The chunk arena and the dictionary | Everything in [What to draw](#draw) |
| The raylib window and GL context | Input handling, and the cursor's *position* |

The reference implementation worth reading first is **tsoding's `musializer`**
(<https://github.com/tsoding/musializer>), which is close to this program in shape: a live
data stream turned into pixels, raylib, one binary, no framework. Three of its decisions
transfer directly, and all three are easy to get wrong from scratch.

**1. The window belongs to the host.** In `src/musializer.c`, `InitWindow` and
`InitAudioDevice` are called from `main()`, *outside* the reload path; only `plug_update()`
is called through the library. The GL context therefore survives a reload. Put `InitWindow`
in the plugin and every reload destroys and recreates the window.

**2. State crosses the boundary explicitly.** The plugin interface in `src/plug.h` is six
functions, two of which exist solely for this:

```c
PLUG(plug_pre_reload,  void*, void)
PLUG(plug_post_reload, void,  void*)
```

The host's loop is literally `state = plug_pre_reload(); reload_libplug();
plug_post_reload(state);` — the state is handed *out* before `dlclose` and *back* after
`dlopen`, so nothing in it is re-initialised. Here that means the arena, the dictionary,
the socket and the last-transmitted id ride across untouched: **the trace survives the
reload.** That is the entire payoff. Tuning the [OKLab ramp](#hues), a cushion-shading
constant or an [aggregation choice](#aggregation) against a live trace, without re-running
the model to get the trace back, is a two-second loop instead of thirty. And per
[Verification](#verification), re-running is not merely slow — these simulations finish in
well under a second, so a live trace is often not reproducible on demand at all. Losing it
to a recompile means losing it.

**3. Ignore `SIGPIPE`.** `musializer.c` disables it in `main()` with a comment explaining
why: it writes into a pipe that can break, and a "relatively friendly GUI application"
should recover rather than die. This viewer has precisely that hazard and will meet it on
day one — the simulator exits, the connection closes, the next write raises `SIGPIPE`, and
the default disposition kills the process. **A trace viewer that dies the moment the run
it is watching finishes is useless, and the run finishing is the normal case.**

One non-transfer: `musializer` builds with `nob.c` rather than make. There is no reason to
change this repository's `Makefile` for it. The mechanism needs only `-fPIC -shared` for
the plugin and an rpath of `.` on the host, and `MUSIALIZER_HOTRELOAD` shows the whole
thing is worth keeping behind a compile-time flag, so release builds stay a single static
binary and the [no-setup claim](../../POSITIONING.md) is unaffected.

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

**One form of this is buildable immediately**, and the note has moved it forward to
milestone 3b accordingly: the [coverage difference map](#diff) needs no new records, no
shadow data and no alignment algorithm — two arenas and a subtraction over a shared
floorplan. It is worth treating as the proof that the trace-id generalisation is real,
rather than waiting for the harder two.

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

10. **The treemap layout is computed once and is byte-identical across the run.** A test
    renders the layout at the first and last record of a trace and asserts the rectangles
    are unchanged. This is [the static-area rule](#static-area) made checkable, and it is
    what the small-multiples strip depends on.

11. **State coverage is derived client-side, and agrees with the simulator.** The union of
    `STORAGE_WHAT_CURRENT_STATES` over a trace, divided by the dictionary size, matches
    the `State Coverage:` figure the simulator prints for the same run. If the two
    disagree, one of them is wrong and it is worth knowing which.

12. **XOR and AND are visually distinguishable without reading labels.** Shown by a model
    containing both an orthogonal region and a composite state: a reader who has never
    seen the model can say which is which from the rendering alone. Until this holds, the
    treemap [misleads on the interesting cases](#xor-and).

13. **The two views are coupled through one cursor.** Scrubbing the timeline updates the
    treemap and selecting in the treemap filters the timeline, with a single piece of
    state between them. Two independently maintained notions of "current position" is the
    failure mode.

14. **Zero is the background, in both colourings.** A fully covered trace renders as a
    blank panel with only borders visible, and a trace differenced against itself renders
    identically blank. If either shows ink, the ramp is not anchored where it claims to
    be. This is [the ink principle](#ink) made checkable in one screenshot.

15. **The difference ramp is symmetric even when the data is not.** Differencing two traces
    whose changes are all in one direction still places neutral at exactly zero. Checked
    by asserting that the colour of a zero-difference cell is the background colour,
    independent of the data range.

16. **The scheme survives simulated colour-vision deficiency.** The coverage map and the
    difference map remain readable under protanopia, deuteranopia and tritanopia
    simulation. Cheap to check once, and the failure it guards against is invisible to the
    author by construction.

17. **The aggregation function is visible in the interface.** A reader can tell, without
    consulting source or documentation, whether a collapsed cell shows max or mean
    ([above](#aggregation)). Not being able to tell is worse than either choice.

18. **The persistent-gap category is distinguishable from no-change.** States uncovered in
    *both* traces are visually distinct from states covered in both, despite having the
    same numeric difference ([above](#diff)). This is the row most likely to be dropped on
    a first implementation, and the one most worth seeing.

19. **The trace survives a recompile of the renderer.** With a live connection open and
    chunks arriving, rebuilding and reloading the drawing code leaves the arena, the
    dictionary and the stream cursor intact; no chunk is lost and no re-run is needed
    ([above](#hotreload)). Since a run completes in well under a second, a reload path
    that discards the trace is equivalent to not having one.

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

6. **What is the right decay constant for a recency colour ramp?** Raised by
   [the colour table](#colour) and left open deliberately: too fast and the live view
   flickers, too slow and everything is uniformly warm. It probably has to be relative to
   the step rate rather than to wall-clock time, which means it differs between a
   timer-driven model and a free-running one.

7. **Does the treemap need its own aggregation for orthogonal regions?** A composite
   state's area is the sum of its children's. For an orthogonal region, where several
   children are active at once, "how much of this rectangle is live" has an obvious
   reading, whereas for a XOR composite it does not — exactly one child is live by
   definition, so the fraction is uninformative. Whether that argues for different
   *shading* rules in the two cases, on top of the different *dividers* from
   [XOR and AND](#xor-and), is unresolved.

8. **What are the two traces in a difference map, by default?** Latest against previous
   run is the obvious pairing, but *latest against the best coverage ever achieved* is
   probably the more useful one, and it needs a stored baseline. A baseline raises the
   question of where it lives and when it is updated, which is a workflow decision rather
   than a rendering one.

9. **Should a difference map ever be live?** Differencing a running trace against a
   finished baseline is well-defined and would show coverage filling in as the simulation
   proceeds. Whether that is useful or merely hypnotic is untested.

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

### Implementation references

- A. Kutepov (tsoding), *Musializer* — <https://github.com/tsoding/musializer>. A raylib
  application of near-identical shape: a live data stream rendered in real time, single
  binary, hand-written C, no framework. Read `src/musializer.c` and `src/plug.h` before
  starting milestone 3; the [reload boundary](#hotreload) above is lifted from them, as is
  the `SIGPIPE` disposition.
- A. Kutepov (tsoding), *nob.h* — <https://github.com/tsoding/nob.h>. The build system
  `musializer` uses. Noted for completeness only; this repository already has a `Makefile`
  and the reload mechanism does not need more than it.

### Visualization references

Cited inline above; collected here because the treemap design leans on all three.

- B. Johnson and B. Shneiderman, *Tree-Maps: a space-filling approach to the
  visualization of hierarchical information structures*, IEEE Visualization 1991 — the
  original space-filling construction.
- M. Bruls, K. Huizing and J. J. van Wijk, *Squarified Treemaps*, Data Visualization 2000
  (Eurographics/IEEE TCVG) — the aspect-ratio-versus-stability trade-off that
  [static area](#static-area) sidesteps.
- J. J. van Wijk and H. van de Wetering, *Cushion Treemaps: visualization of hierarchical
  information*, IEEE InfoVis 1999 — the per-cell shading that makes nesting depth legible
  past three levels.

### Colour references

For [the colour scheme](#scheme). All four are white-centred and so are references rather
than drop-in palettes; the construction rule here re-anchors the midpoint to the panel
background.

- F. Crameri, *Scientific colour maps* (`vik`, `broc`, `cork`) — perceptually uniform and
  CVD-tested diverging maps, with the accompanying argument in *The misuse of colour in
  science communication*, Nature Communications 11, 5444 (2020).
- K. Moreland, *Diverging Color Maps for Scientific Visualization*, ISVC 2009 — the
  cool–warm map, and the reasoning against rainbow ramps.
- C. Brewer, ColorBrewer (`RdBu`) — the long-standing reference for CVD-safe diverging
  pairs.
- B. Ottosson, *OKLab* (2020) — the perceptual space to interpolate the ramp in;
  interpolating in sRGB is what produces banding and false boundaries.
