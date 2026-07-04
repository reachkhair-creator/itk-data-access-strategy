# ITK Data Access Strategy

A mental model for deciding how and when server-side ITK code should read, write, or query Teamcenter objects.

This is not an API tutorial. It's the discipline layer that sits above the APIs — the rules that decide whether a handler runs fine at 5 users and falls over at 200.

Every example in this repo uses HVAC product structures (AHU assemblies, FCU revisions, ducting BOMs) because that's the environment this strategy was built and tested in.

---

## What "Data Access Strategy" Means

A set of rules that keep server-side ITK logic:

- Reading only the data it needs
- Writing only what must change
- Free of heavy, repeated, or unsafe operations
- Upgrade-safe and performance-stable as data volume and user count grow

The difference between "ITK code that works" and "ITK code that scales for 200+ concurrent users" is almost never the business logic. It's this layer.

---

## The 5 Pillars

### 1. Minimize Reads — Never Fetch the World
Most slowness comes from reading too many properties or objects per handler call.

- Use minimal property sets — ask for what the logic actually consumes
- Avoid `AOM_ask_value_string` in a loop
- Prefer batch reads (`AOM_ask_values`) over per-object calls
- Cache repeated values inside the handler instead of re-fetching

**Rule:** If you read 200 properties but use 12, your code is the problem.
See [`examples/01_minimize_reads.c`](examples/01_minimize_reads.c) — an FCU revision-name lookup that goes from N server calls to 1.

### 2. Control Object Expansion
GRM relations and BOM structures expand fast. An AHU top-level assembly can pull in thousands of GRM lines if traversal isn't bounded.

- Use controlled GRM traversal with an explicit depth limit
- Avoid recursive expansion unless the task genuinely requires full structure
- Use a query instead of manual traversal when you're filtering, not walking

**Rule:** Expanding a BOM in ITK is expensive — expand only what you need.
See [`examples/02_control_expansion.c`](examples/02_control_expansion.c) — duct-component GRM traversal with a depth cap.

### 3. Write Safely — Lock Less, Validate More
Writing is where production incidents happen: deadlocks, orphaned locks, half-committed changes.

- Lock only the objects you must modify
- Unlock immediately after save — don't hold locks across unrelated logic
- Validate business rules before acquiring the lock, not after
- Never write inside a loop without batching the commit

**Rule:** Locking too many objects causes deadlocks and workflow freezes.
See [`examples/03_safe_writes.c`](examples/03_safe_writes.c) — updating a revision status on a filtered FCU set.

### 4. Use the Right API for the Right Job

| API family | Use for |
|---|---|
| AOM | Properties, save, lock/unlock |
| GRM | Relations (BOM lines, references) |
| WSO | Workspace object-level operations |
| Query | Searching — replaces manual looping |
| POM | Low-level access — use sparingly, last resort |

**Rule:** If you're looping through 10,000 objects, you probably needed a query.
See [`examples/04_right_api.c`](examples/04_right_api.c) — finding all released AHU items by query instead of full-BOM iteration.

### 5. Avoid Repeated Server Calls
Every ITK call is a server round-trip. A handler that "feels slow" is usually a handler making too many of them, not one doing heavy computation.

- Combine reads into a single batch call
- Cache values already fetched in this handler execution
- Never call the same API inside a loop when a list-based variant exists
- Batch writes the same way you batch reads

**Rule:** A slow handler is usually a handler making too many calls.
See [`examples/05_batch_calls.c`](examples/05_batch_calls.c) — before/after for a duct-BOM naming pass.

---

## Quick Reference: Slow vs Optimized

```c
/* Slow — one server round-trip per child */
for each child:
    AOM_ask_value_string(child, "object_name", &name);

/* Optimized — one server round-trip for the whole list */
AOM_ask_values(child_list, "object_name", &names_array);
```

One call vs hundreds. This is the pattern behind all five pillars.

---

## Why This Matters in GCC Manufacturing

UAE/KSA HVAC manufacturing environments typically run:

- Large, multi-level BOMs (AHU/FCU product families with hundreds of duct and sheet-metal children)
- Heavy workflow chains (design release → ECO → manufacturing handoff)
- Multi-CAD environments (Inventor + others feeding the same PLM structure)
- High concurrent user counts across design, planning, and manufacturing

A poor data access strategy shows up as slow ECO release, slow AW BOM loading, fragile integrations, painful upgrades, and audit reports that time out. A disciplined one keeps the system stable under 200+ users running 24/7.

---

## Repo Contents

```
itk-data-access-strategy/
├── README.md              this file
├── CHECKLIST.md            pre-deployment review checklist
└── examples/
    ├── 01_minimize_reads.c
    ├── 02_control_expansion.c
    ├── 03_safe_writes.c
    ├── 04_right_api.c
    └── 05_batch_calls.c
```

## Note on the Code

Examples are written in ITK-style C to illustrate the pattern, not to compile as-is — error handling, includes, and environment setup are trimmed for readability. Treat them as before/after reference, not drop-in production code.

## Author

Syed Khair (Foumin) — Senior Design Engineer / Teamcenter PLM Administrator, 20+ years HVAC product engineering and enterprise PLM.
LinkedIn: [syed-khair-plm](https://www.linkedin.com/in/syed-khair-plm)
