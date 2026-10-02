QClip release candidates must pass the active SPEC acceptance gates before publication. Keep upstream and third-party licenses in the installed package and provide the corresponding source tarball. A local build is not a published release.

Audit an installed layout with `python3 utils/check-release.py --platform macos --root PATH/QClip.app` (or `windows` / `linux`). The macOS audit checks every bundled binary against the declared minimum OS, not just the main executable. Add `--source SOURCE.tar.gz` to verify corresponding source material. `utils/package-source.py OUTPUT.tar.gz` creates an archive of the current tracked and non-ignored source, plus a file hash manifest, for reviewing uncommitted development builds. Tagged release source still comes from `git archive` in the release script.

Automatic expansion starts disabled. macOS requires Accessibility and Input Monitoring permissions. Windows requires Windows 10 1903 or newer. Linux builds require ICU development headers; global expansion currently uses X11 RECORD/TEST with confirmed ASCII keyboard input and pauses when an IME is configured. Wayland reports global input monitoring unavailable while retaining the existing clipboard integration. Validate signatures and input permissions on the final install path.

This is step-by-step description on how to release new version of QClip (based on CopyQ).

# Update Version and Changelog

Update `CHANGES.md` file (go through all commits since the last release tag).

Bump the version:

    utils/bump_version.sh 14.0.0

Verify and push the changes:

    git push --follow-tags origin master

# Draft Release

Run the release script:

    utils/github/draft-release.sh 14.0.0

This automates the following steps:

1. Creates a draft GitHub Release with the changelog from `CHANGES.md`.
2. Creates the source tarball and uploads it to the release.
3. Waits for CI to attach build artifacts (`.dmg`, `.zip`, `.exe`) to the release.
4. Downloads the release assets, generates `checksums-sha512.txt`, and signs it
   with `cosign` (opens a browser for OIDC authentication).
5. Uploads `checksums-sha512.txt` and `cosign.bundle` to the release.

The script is idempotent. If interrupted, rerun it with the same working
directory to resume:

    utils/github/draft-release.sh 14.0.0 ./release-14.0.0

Artifacts produced by CI (attached automatically by GitHub Actions):

- Windows installer (`qclip-VERSION-setup.exe`)
- Windows portable zip (`qclip-VERSION.zip`)
- macOS Intel DMG (`QClip-VERSION-macos-13.dmg`)
- macOS Apple Silicon DMG (`QClip-VERSION-macos-13-m1.dmg`)
- Linux AppImage (`QClip-VERSION-x86_64.AppImage`)

# Upstream Flatpak reference

Update [flathub package](https://github.com/flathub/com.github.hluk.copyq):

1. Update "tag" and "commit" in "com.github.hluk.copyq.json" file.
2. Push to your fork.
3. [Create pull request](https://github.com/flathub/com.github.hluk.copyq/compare/master...hluk:master).
4. Verify the build when the build finishes (flathubbot will add comments).
5. Merge the changes if the build is OK.

# Publish Release

Review and publish the draft release on GitHub.

Publish only to the QClip repository after authorization. The upstream CopyQ release destinations are retained as attribution and are not QClip upload targets.
