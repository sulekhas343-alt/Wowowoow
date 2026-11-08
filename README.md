# Daily Level Suggestions (Geode mod for Geometry Dash)

Suggests which Demon List levels to beat today. Open it from the green star button in the main menu.

## Before you build
1. `mod.json` is already set to developer `rallygagler` (id `rallygagler.daily-suggestions`).
2. Install the Geode SDK + CLI: https://docs.geode-sdk.org/getting-started/
3. Make sure `GEODE_SDK` is set (the CLI does this for you).

## Build
```
geode build
```
This produces a `.geode` file in `build/`. Drop it into your `geode/mods` folder
(or run `geode install`). For other platforms (Android, macOS, iOS) use the
included GitHub Actions workflow: push the repo to GitHub and download the artifacts.

## How it works
- Fetches the top 75 from the Pointercrate API (`/api/v2/demons/listed`).
- Picks N random levels using a daily seed, filtered by your settings and
  (optionally) skipping levels you have already beaten.
- Saves today's picks so they don't change when you beat one.
- "View" opens the level via a level ID search.

## Settings
| Setting | Meaning |
|---|---|
| Suggestions per day | 1-6 |
| Hardest / easiest list position | Restrict the difficulty range (#1 = hardest) |
| Skip levels I've beaten | Exclude completed levels |

Targets GD 2.2074 / Geode v4.
