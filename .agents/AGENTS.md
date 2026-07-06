# AFF3CT & Refs Project Rules

## Commit & Push Policy
- **Never commit or push**: Never commit and/or push any code or modifications before the user explicitly asks for it.

## Branching Model & Deprecation Policy
- **Deprecated Branches**: `master` and `main` are deprecated across the project repositories (`aff3ct` and `refs`). Do NOT target or create new workflows/deployments on `master` or `main`.
- **Active Development Branch**: The primary living branch is `develop`.
- **CI/CD Triggers**: All continuous integration and deployment workflows must trigger exclusively on `develop` and active feature/migration branches (e.g., `github-actions-migration`).

## C/C++ Code Verification Policy
- **Compile & Test**: Whenever C or C++ source code (`.c`, `.cpp`, `.h`, `.hpp`) is modified, you MUST always compile the project inside `build/` (`make -j$(nproc)`) and execute a short test (`./bin/aff3ct-* -v` or simulation) to verify the fix locally before completing your turn.
