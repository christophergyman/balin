# AGENTS.md

## Repo
Balin is a minimal 2D raylib game written in C. The repo docs are the source of truth: `docs/product-requirements-document.md` for product, `docs/architecture.md` for decisions.

## Linear
Linear is the tracker. Use team **Balin-game**. Before doing anything Linear-related, read `docs/linear-ticket-guidance.md`. It defines how to structure tickets.

- One project: **Balin: Demo**. Every ticket belongs to it.
- Milestones **M1** to **M5** come from PRD section 16. They are the build order. Finish them in order.
- Every ticket gets one **Area** label: engine, gameplay, editor, content, audio, docs.
- Every ticket gets one **Type** label: Bug, Feature, Improvement.
- Status flow: Backlog, Todo, In Progress, Playtest, Done. Done means verified in a playtest run, not just merged. Canceled drops work.
- Views: **Active work** (grouped by milestone) and **Playtest queue**.

## Rules for agents
- Do not use skills from the harness. Do all Linear work with the Linear MCP tools directly.
- When creating a ticket, set: team Balin-game, project Balin: Demo, one milestone, one Area label, one Type label.
- Do not create or change projects, milestones, labels, or statuses. Ask cman first.
- Do not create tickets unless cman asks. He populates the board.
- Do not delete or rewrite tickets cman authored unless he asks.
- Keep one deliverable per ticket. Keep design detail in the repo docs, not in tickets.
