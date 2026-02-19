# Web MVP Export Plan (Chat + PixelPen)

Last updated: 2026-02-19

This document defines the **path-of-least-resistance** MVP for an exported Phoenix web build.

## Goal

Ship one exported web build that is useful for early prototyping and validation while minimizing risk.

MVP scope:

1. Chat workflow
2. PixelPen workflow

Out of scope for this MVP:

- Full addon parity with desktop editor
- GDTerm, Git plugin, Diff Margin, BFXR, GUT parity in web
- Multi-tool parity guarantees

## Access and security requirement

The web build must be account-gated:

- Unauthenticated users should not be able to access protected Phoenix runtime/orchestration paths.
- Runtime access continues to enforce bearer token validation on backend boundaries.
- Website auth and backend identity alignment continue to use Entra identity anchors (`tid` + `oid`, fallback `sub`).

## Current CI/CD baseline in Engine

Current web workflow (`.github/workflows/web_builds.yml`) builds template artifacts (`target=template_release`) and uploads Web artifacts from `bin`.

This is good for template output but not yet a dedicated, account-gated web editor MVP package.

## MVP CI/CD target (doc plan)

Update Engine web CI/CD to produce a web MVP artifact lane focused on chat + PixelPen:

1. Add a dedicated web MVP build job/lane in `web_builds.yml`.
2. Keep artifact naming explicit (for example `web-mvp-chat-pixelpen-*`).
3. Stage only required MVP payload assets in the web artifact package.
4. Exclude/additionally disable non-MVP addons from MVP payload checks.
5. Keep artifact guard checks strict so broken payloads fail fast.

## Packaging policy for MVP lane

Required for MVP lane:

- Core web build artifact output (`.wasm`, `.js`, and required runtime files)
- Chat/assistant UI runtime path
- PixelPen addon payload required for web flow
- Capability manifest (`bin/phoenix_web_capabilities.json`) declaring MVP-enabled/disabled features

Not required for MVP lane:

- Non-web-native addon payloads not used in MVP
- Full desktop-equivalent addon bundle

## Runtime behavior policy for MVP lane

- The MVP lane should default to only chat + PixelPen user-visible capabilities.
- Any unsupported features should be hidden or hard-disabled in MVP builds.
- Failure mode should be explicit and user-safe (no partial silent failures).

## Validation checklist

Before publishing MVP artifact:

1. Artifact guard confirms expected web files are present.
2. MVP payload guard confirms chat + PixelPen assets are present.
3. MVP capability manifest is present and matches expected profile/flags.
4. Smoke run confirms account-gated path can authenticate and reach orchestration endpoints.
5. Smoke run confirms unauthorized requests fail (`401`/`403`) at protected runtime routes.

## Follow-on phases

After MVP is stable:

1. Add one addon family at a time.
2. Add parity test cases per addon before enabling by default.
3. Keep account-gated auth boundary unchanged as features expand.
