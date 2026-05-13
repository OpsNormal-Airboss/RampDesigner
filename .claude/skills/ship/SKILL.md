# /ship - Document, commit, push, close
1. Update CLAUDE.md, README.md, RUNBOOK.md to reflect recent changes
2. Run the full test suite; abort if failing
3. Stage all changes, write a conventional commit referencing the issue/sprint
4. git push; if SSH passphrase blocks, print the command for the user
5. Close referenced GitHub issues via `gh issue close`
