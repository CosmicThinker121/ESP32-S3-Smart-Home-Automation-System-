This pull request reorganizes the repository to improve structure and fix README links.

Changes included on this branch (fix/file-layout):

- Added firmware/SmartHome_ESP32S3.ino (moved from repository root)
- Added docs/bill-of-materials.md (moved from repository root)
- Added docs/testing-and-troubleshooting.md (moved from repository root)
- Updated README.md to reference the files in firmware/ and docs/, and to reference PNG images from the repository root (PNG files were intentionally left at the root by request)

Files that still exist in the repository root (these root copies were not deleted on this branch):
- SmartHome_ESP32S3.ino
- bill-of-materials.md
- testing-and-troubleshooting.md

To finish the move you can remove the root copies with:

  git checkout fix/file-layout
  git rm SmartHome_ESP32S3.ino bill-of-materials.md testing-and-troubleshooting.md
  git commit -m "Remove root copies after moving into firmware/ and docs/"
  git push origin fix/file-layout

No PNG files were duplicated or deleted.
