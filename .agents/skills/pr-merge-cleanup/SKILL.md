---
name: pr-merge-cleanup
description: "swbt-daemon GitHub Flow workflow for pushing a non-default branch, creating a PR with the project template, checking CI/status, merging with the repo/user-approved method, syncing the local default branch, and deleting local/remote work branches. Use when Codex is asked to PR, merge, publish a branch, clean up after merge, or move swbt changes into main."
---

# PR Merge Cleanup

Use this skill to publish a completed swbt branch through GitHub and clean up local state.

## Preconditions

- Current branch is not the default branch.
- `git status --short` is clean.
- Required commits already exist.
- Work-unit self review is complete when the change is nontrivial.
- Hardware and source-audit sections are truthful.
- User has approved merge if checks and repository policy allow it.

## Workflow

1. Resolve context:
   - `git branch --show-current`
   - `git remote get-url origin`
   - default branch from `origin/HEAD` or repository metadata.
2. Stop if on default branch.
3. Stop if worktree is dirty.
4. Build PR commit log:
   - `git log --oneline <default>..HEAD`
5. Fill `.github/PULL_REQUEST_TEMPLATE.md`.
6. Push branch.
7. Create PR with GitHub app or `gh pr create`.
8. Check CI/status with GitHub app or `gh pr checks`.
9. Stop if required checks are failing or still running.
10. Merge using the repository/user-approved method.
11. Fetch and sync default branch:
    - `git fetch --prune origin`
    - `git switch <default>`
    - `git pull --ff-only origin <default>`
12. Verify default branch is clean and at expected head.
13. Delete local and remote work branches.

## Merge Policy

- Prefer the repository's configured project policy.
- If no project policy is specified, ask before choosing merge, squash, or rebase.
- Do not squash merely because a branch has multiple commits; commit history may carry work-unit evidence.
- Stop if merge commit is disabled but the PR body assumes merge-commit history.

## PR Body Requirements

The PR body must include:

- Summary.
- Related issue/spec/journal entries.
- Changes grouped by behavior, structure, docs, and workflow.
- Source Audit status.
- Testing commands and results.
- Hardware status or not-run reason.
- BTstack / License impact.
- Checklist.

## Stop Conditions

- dirty worktree.
- default branch.
- required check failure.
- unknown mergeability.
- missing hardware approval for hardware-gated changes.
- missing source audit for protocol or BTstack facts.
- PR template sections are materially incomplete.
- local default branch cannot fast-forward after merge.

## Final Report

Report:

```text
PR:
merge method:
merge commit:
synced default branch:
deleted branches:
gates:
hardware:
source audit:
follow-up:
```
