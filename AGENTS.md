# Agent Instructions & Guidelines

## Remotes
- **Origin**: `origin` (`git@github.com:cryvate/augustus.git`)
- **Original Upstream Remote**: `upstream` (`git@github.com:Keriew/augustus.git`)

## Central Branch Management
- **Central Branch**: `hj-android` contains all current changes we want included.
- **Branching Strategy**:
  - When stable, any distinct changes/features should exist in their own dedicated branch.
  - The central branch (`hj-android`) should be the merge of those feature branches.

## Active Feature Branches
- `feature/android-app-name`: Renames Android app display name to "Caesar III".
- `feature/auto-pause`: Automatically pauses game on dialogs, scenario start, and save load.
- `feature/autosave-resume`: Periodic 10s & background resume-autosave and startup auto-load system.
- `feature/sidebar-extra-info`: Optimized sidebar extra info panel layout with compact god mood tracker, festival timer, ratings reasons, and quick action buttons.
- `feature/save-ini-in-user-dir`: Saves `augustus.ini` and configuration files directly in the user directory (`user_dir`).
- `feature/respect-orientation-on-restore`: Respects screen orientation mode on restore and unlocks manual navigation bar rotation when auto-rotate is disabled.
