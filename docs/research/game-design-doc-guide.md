# Game Design Doc Guide for a Small Demo

**Purpose:** Write just enough documentation to keep a small game demo focused, testable, and finishable.
**Scope:** A solo developer or a tiny team. Big-studio advice is noted where it conflicts.

## The rules

1. **Start with one page.** Many teams start with a one-page GDD and add detail only as the game proves itself in prototypes ([GitBook](https://www.gitbook.com/blog/how-to-write-a-game-design-document)).
2. **Keep it living.** Update the doc as the game changes. A doc that is out of date is not useful ([Nuclino](https://www.nuclino.com/articles/game-design-document-template)).
3. **Write for skimming.** Use short sections, lists, strong headings, and visuals. Long walls of text do not get read ([GitBook](https://www.gitbook.com/blog/how-to-write-a-game-design-document)).
4. **Lead with vision, pillars, and loop.** A one-line vision, 2 to 3 pillars, and the core loop anchor every later decision ([GDC Vault](https://gdcvault.com/play/1035120/The-Four-One-Page-Design)).
5. **Name your non-goals.** Write what the demo will not include. This is the main defense against scope creep ([Indie Game Academy](https://indiegameacademy.com/free-game-design-document-template-how-to-guide/)).
6. **Prototype before you detail.** Prototypes answer "should we make this game" ([Rami Ismail](https://ltpf.ramiismail.com/prototypes-and-vertical-slice/)). Make them fast, cheap, and disposable.
7. **Aim for a vertical slice.** One of each thing at near-final quality answers "can we make this game" ([Rami Ismail](https://ltpf.ramiismail.com/prototypes-and-vertical-slice/)).
8. **Playtest early and often.** Watch behavior, ask open questions, and act on aggregate trends ([Arkadium](https://www.gamedeveloper.com/design/best-practices-five-tips-for-better-playtesting)).
9. **Cut until it hurts.** Keep cutting until only the proven fun core remains. If you quit a project, scale the next one down, not up ([Derek Yu](https://www.tumblr.com/makegames/1136623767/finishing-a-game)).
10. **Do not mix doc types.** A pitch deck sells the idea, a GDD defines the game, a TDD explains implementation ([GitBook](https://www.gitbook.com/blog/how-to-write-a-game-design-document)).

## The one-page template

Copy this into `docs/` and fill it in. Keep each section to a few lines. Delete any section that does not apply; the template is a starting point, not a checklist ([Jason Bakker](https://www.gamedeveloper.com/design/a-gdd-template-for-the-indie-developer)).

```markdown
# <Game title>

One-line pitch:
Genre / platform / target session length:

## Player fantasy
What the player should feel, in 2 or 3 sentences.

## Design pillars (max 3)
1. <Pillar> -> <what it forces into the design>
2. <Pillar> -> <what it forces into the design>
3. <Pillar> -> <what it forces into the design>

## Core loop
<Verb> -> <Verb> -> <Verb> -> repeat.
Why is this loop fun on the 10th repetition?

## Demo definition
Success means: <for example, a playtester finishes the level and starts a second run unprompted>
Target length: <minutes>
Done when: <concrete build checklist>

## Scope
Must have (cap at 3 to 5):
Nice to have:
Non-goals (explicit cuts):

## Content list
Levels:
Enemy types:
Items or abilities:
UI screens:
Cutscenes:

## Controls and UI
Input map:
Key screens and HUD elements:

## Art and audio direction
References (2 or 3 games or films):
Palette, mood, and sound in one line each:

## Open questions
- [ ] <Question> -> <decision + date>

## Decision log
- <date>: <decision and reason>
```

## How the doc evolves with the work

```
one-page GDD
     |
     v
  prototype   <-- fast, placeholder art, answer "should we?"
     |
     v
  playtest
     |
     v
    cut  ----------> update the doc with what survived
     |
     v
vertical slice  <-- one of each thing at near-final quality
     |
     v
  playtest
     |
     v
    demo
```

1. Write the one-pager. Time-box it to about half a day.
2. Prototype the core loop with placeholder art. Use ASCII assets if that is fastest ([Derek Yu](https://www.tumblr.com/makegames/1136623767/finishing-a-game)).
3. Playtest the prototype. Record what players did, not only what they said ([Arkadium](https://www.gamedeveloper.com/design/best-practices-five-tips-for-better-playtesting)).
4. Cut features that did not earn their place ([Derek Yu](https://www.tumblr.com/makegames/1136623767/finishing-a-game)).
5. Build the vertical slice: one level, one enemy, one full UI path, at near-ship quality ([Rami Ismail](https://ltpf.ramiismail.com/prototypes-and-vertical-slice/)).
6. Update the doc with the decisions that survived. Delete the rest.
7. Repeat until the demo "Done when" checklist is true.

## Playtest checklist

- Recruit testers who match the target player. Friends of the team add bias because they tend to want to love the game ([Arkadium](https://www.gamedeveloper.com/design/best-practices-five-tips-for-better-playtesting)).
- Run the whole test once before real testers arrive, including the setup and the survey ([Arkadium](https://www.gamedeveloper.com/design/best-practices-five-tips-for-better-playtesting)).
- Tell testers that you are testing the game, not them. Ask them to think aloud and stay quiet otherwise ([Arkadium](https://www.gamedeveloper.com/design/best-practices-five-tips-for-better-playtesting)).
- Ask open questions that test knowledge: "What does this button do?" instead of "Was this confusing?" ([Arkadium](https://www.gamedeveloper.com/design/best-practices-five-tips-for-better-playtesting)).
- Collect data in aggregate, then look for trends. Do not redesign from one strong reaction ([Arkadium](https://www.gamedeveloper.com/design/best-practices-five-tips-for-better-playtesting)).

## Anti-patterns

- **The design bible.** Hundred-page docs are hard to maintain and rarely read ([Stone Librande](https://www.gamedeveloper.com/design/video-one-page-designs), [Nuclino](https://www.nuclino.com/articles/game-design-document-template)).
- **Wiki sprawl.** A large wiki can break the relationships between design elements ([Stone Librande](https://www.gamedeveloper.com/design/video-one-page-designs)).
- **Spec-first thinking.** The doc is "a general description more than a blueprint." Everything must be allowed to change ([Jason Bakker](https://www.gamedeveloper.com/design/a-gdd-template-for-the-indie-developer)).
- **Doc before fun.** More detail does not de-risk a game. Prototypes do ([Rami Ismail](https://ltpf.ramiismail.com/prototypes-and-vertical-slice/), [Game Design Skills](https://gamedesignskills.com/game-design/document/)).
- **Over-documenting solo work.** "Over-document everything" is AAA advice for many disciplines. A solo demo needs far less ([Game Design Skills](https://gamedesignskills.com/game-design/document/)).
- **No doc at all.** Even solo developers benefit from a lightweight doc to stay consistent and remember early decisions ([GitBook](https://www.gitbook.com/blog/how-to-write-a-game-design-document)).

## If the demo grows

Add detail only when a feature needs it, one doc per feature ([Game Design Skills](https://gamedesignskills.com/game-design/document/)). Common sections for a larger doc ([Indie Game Academy](https://indiegameacademy.com/free-game-design-document-template-how-to-guide/)):

- Gameplay and mechanics
- Story and world
- Art and aesthetics
- Audio
- Tech and tools
- Schedule and team roles

## Examples worth studying

- **Deus Ex**: annotated early design doc. Shows scope that was cut, including multiplayer and a space station act ([GitBook](https://www.gitbook.com/blog/how-to-write-a-game-design-document)).
- **GTA (Race'n'Chase)**: early concept doc with gameplay, team, and timeline ([GitBook](https://www.gitbook.com/blog/how-to-write-a-game-design-document), [Nuclino](https://www.nuclino.com/articles/game-design-document-template)).
- **Doom Bible**: 1992 design bible with weapons, characters, and sounds ([GitBook](https://www.gitbook.com/blog/how-to-write-a-game-design-document)).
- **Grim Fandango**: puzzle document with flow charts and handwritten notes ([GitBook](https://www.gitbook.com/blog/how-to-write-a-game-design-document)).
- **BioShock and Diablo pitch documents**: short, persuasive, and visual ([GitBook](https://www.gitbook.com/blog/how-to-write-a-game-design-document)).
- **Stone Librande one-page designs**: Spore, Diablo 3, SimCity, and The Simpsons Game ([Stone Librande](https://www.gamedeveloper.com/design/video-one-page-designs)).

Caveat: public examples are usually pitch artifacts or large-studio docs. Use them for ideas, not as a size target.

## Sources

- GitBook, "How to write a game design document", updated 2026-01-28. Modern GDD practice, one-page start, doc types, examples. https://www.gitbook.com/blog/how-to-write-a-game-design-document
- Nuclino, "Game Design Document Template and Examples", updated 2026-01-14. Agile documentation, lightweight single page, visual aids. https://www.nuclino.com/articles/game-design-document-template
- Jason Bakker, "A GDD Template for the Indie Developer", Game Developer, 2009-06-04. Indie-oriented sections and "general description more than a blueprint". https://www.gamedeveloper.com/design/a-gdd-template-for-the-indie-developer
- Ostap Dovbush, "Game Design Document: Definition, Template, Example", Game Design Skills. AAA 9-step doc process, master and feature docs, prototype quickly. https://gamedesignskills.com/game-design/document/
- Stone Librande, "One-Page Designs", GDC 2010, summary by Game Developer. One-page annotated diagrams; problems with long docs and wikis. https://www.gamedeveloper.com/design/video-one-page-designs
- Ian Schreiber, "The Four One-Page Design Docs You Need (And How to Use Them)", GDC 2025. Vision, pillars, loops, and resource flow. https://gdcvault.com/play/1035120/The-Four-One-Page-Design
- Rami Ismail, "Prototypes & Vertical Slice", 2022-09-26. Prototype answers "should we"; vertical slice answers "can we". https://ltpf.ramiismail.com/prototypes-and-vertical-slice/
- Derek Yu, "Finishing a Game", makegames, 2010. Finishing as a skill, cut scope, scale down, use ASCII. https://www.tumblr.com/makegames/1136623767/finishing-a-game
- Vin St. John / Arkadium, "Best Practices: Five Tips for Better Playtesting", Game Developer, 2013-01-23. https://www.gamedeveloper.com/design/best-practices-five-tips-for-better-playtesting
- Indie Game Academy, "Free Game Design Document Template & How-To Guide", 2025-04-29. One-pager first, GDD sections, scope creep, visuals. https://indiegameacademy.com/free-game-design-document-template-how-to-guide/
- Practitioner threads: r/gamedev and r/gamedesign discussions agree on a living doc and often recommend keeping the early doc to about a page. Some solo devs skip docs entirely. Reddit blocked automated reads, so these findings come from search snippets of the threads. https://www.reddit.com/r/gamedev/comments/13u2zsa/ and https://www.reddit.com/r/gamedesign/comments/m97es2/

## Evidence gaps

- There is no controlled study showing that a GDD improves game outcomes. The guidance above is practitioner consensus.
- Advice conflicts by team size. Game Design Skills recommends over-documenting for AAA. GitBook, Nuclino, and Librande recommend a minimal living doc for small teams. This guide takes the small-demo side.
- Most public GDDs are old, large-team, or pitch-stage artifacts. Few public one-page docs exist for small demos.
- The Game Developer sources from 2009 to 2013 are old but still widely cited. Treat the principles as stable and the tooling advice as dated.
