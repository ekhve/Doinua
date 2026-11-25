# Agent coordination

Use the GitHub issue and pull request as the shared record. Before starting, check the open PRs, branch heads,
and current work noted on the issue. Record one implementer per story and one story branch per PR. If a change
touches a shared interface or another story's files, coordinate the order on the PR before editing.

| Role | Responsibility |
| --- | --- |
| Implementer | Own the story branch, preserve approved scope, and post the commit, tests, screenshots, and documentation changes for review. |
| Independent reviewer, when available | Review the exact PR head. Check the change against the story and approvals, inspect regressions and test coverage, and record findings or a clear review result on the PR. The implementer addresses findings. |
| Factory lead | Keep the issue and PR current, verify the applicable gates and owner approvals, and merge in dependency order. |
| Product owner | Decide scope, review screens that need approval, and validate the physical FM-1 at handoff. |

When both agents are available, each can implement a separate story and review the other's changes.
When only one is available, that agent continues and records a separate self-review pass against the same
checklist. Mark that review as self-review. The normal tests, documentation, and owner gates apply in both cases.

At handoff, record the branch and exact commit, completed and pending checks, relevant approvals, and the next
action or blocker on the PR. A reviewer checks the new head again when a fix changes the reviewed behavior.

