# ITK Data Access Review Checklist

Run through this before an ITK handler goes to code review or production. Written for self-checks, peer review, or onboarding a junior developer.

## Reads
- [ ] Property list requested is the exact set the logic consumes — no "just in case" properties
- [ ] No `AOM_ask_value_string` (or equivalent single-object read) inside a loop
- [ ] Batch read APIs used wherever more than one object needs the same property
- [ ] Values used more than once in the handler are cached locally, not re-fetched

## Object Expansion
- [ ] GRM traversal has an explicit depth limit
- [ ] Recursive expansion is justified by the actual task, not just convenient
- [ ] If the goal is filtering (not walking structure), a query is used instead of traversal

## Writes
- [ ] Validation happens before any lock is acquired
- [ ] Only objects that will actually be modified are locked
- [ ] Every code path — success and failure — reaches an unlock
- [ ] No write calls inside a loop when a batched/list write API is available

## API Selection
- [ ] AOM used for properties, save, lock/unlock
- [ ] GRM used for relations, not for filtering
- [ ] Query API used for any "find objects matching X" task instead of manual iteration
- [ ] POM avoided unless there's a documented reason no higher-level API covers the case

## Server Round-Trips
- [ ] Count the number of ITK calls made per object processed — if it's more than 1-2, look for a batch alternative
- [ ] No identical API call repeated inside a loop when a list-based variant exists
- [ ] Reads and writes are each batched separately, not interleaved per object

## Scale Sanity Check
- [ ] Handler has been mentally tested against realistic production volume (e.g. full AHU/FCU product line, not a 5-item test case)
- [ ] Handler behavior considered under concurrent execution (multiple users triggering it at once)
- [ ] Upgrade impact considered — does this rely on anything non-standard that a Teamcenter upgrade could break?

## Rule of Thumb
If you can't explain, in one sentence, why each server call in this handler is necessary, it probably isn't.
