# Linear Ticket Guidance

How to structure tickets for Balin. Read this before doing any Linear work.

Board: team **Balin-game**, project **Balin: Demo**, milestones M1 to M5.

## Fields

Every ticket sets these:

- Project: Balin: Demo
- Milestone: M1 to M5 from PRD section 16
- One Area label: engine, gameplay, editor, content, audio, docs
- One Type label: Bug, Feature, Improvement
- Status: Todo when scheduled. Done means verified in a playtest run, not just merged.
- Priority: No priority. Use Urgent only for blockers.
- Estimate: none.

## Body

The title carries the work. The body is optional for trivial tickets. Use this skeleton by default:

**What**
One or two lines on what changes and why. Link the PRD section or ADR when useful.

**Done when**
- Testable checks. Each check is observable in a playtest run, the F3 overlay, or balin_tests.

**Notes**
Links, dependencies, edge cases, file paths, tuning keys.

Rules:

- Title: strong, imperative, one deliverable. Write "Dash: i-frames", not "Dash work".
- No user stories. No essays.
- Product detail stays in `docs/product-requirements-document.md`.
- No sub-issues for the demo. Split work into separate tickets instead.

## Bugs

Bugs add a repro block before "Done when":

**Steps to reproduce**
1. First step.
2. Second step.

Expected: what should happen.
Actual: what happens instead.
Build: commit hash and build type (Debug or Release).

## Examples

Feature:

```
Title: Dash: add i-frames during the burst

What
Dash has no protection right now. Add brief i-frames so dash is the main defensive move.
PRD 5.4. ADR-010 buffers the input.

Done when
- A hit during the dash deals no damage.
- The window matches tuning.txt.
- Verified in a playtest run.

Notes
- tuning.txt: dashDuration, iframeDuration
- ADR-004, ADR-017
```

Bug:

```
Title: Crawler pack separation can push one through a wall

What
With 4 crawlers in a corridor, separation shoves one through the north wall.

Steps to reproduce
1. Load the section 0 test room.
2. Stand against the north wall with 4 crawlers behind you.

Expected: crawlers slide along the wall.
Actual: one crawler overlaps the wall tile and escapes.
Build: commit 6e9db43, Debug.

Done when
- No crawler overlaps a wall tile in the repro.
- balin_tests covers the corridor case.
```

## Sources

- Linear Method, Write issues not user stories: https://linear.app/method/write-issues-not-user-stories
- Linear Method, Principles and Practices: https://linear.app/method/introduction
- Linear Docs, Create issues: https://linear.app/docs/creating-issues
- Linear Docs, Issue templates: https://linear.app/docs/issue-templates
- GitStart, Writing Effective Tickets: https://gitstart.com/guides/1-better-tickets/
- Atlassian, Acceptance Criteria: https://www.atlassian.com/work-management/project-management/acceptance-criteria
