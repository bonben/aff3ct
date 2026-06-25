# AFF3CT CI/CD Migration: Deployment Decisions & Open Questions

During the migration of the AFF3CT continuous integration pipeline from **GitLab CI** to **GitHub Actions**, several architectural differences emerged regarding how artifacts and deployments are handled. 

Currently, `.github/workflows/deploy.yml` has been created and configured to handle **PPA Uploads** (for Ubuntu `jammy`, `noble`, and `resolute`). However, the remaining deployment tasks have been temporarily commented out pending team alignment.

This document outlines the context and options for the team to discuss.

---

## 1. Zipped Binaries Distribution (`deploy-builds-linux.sh`)

### Previous GitLab Workflow
Every time a commit landed on `master`, `develop`, or `ppa_upload`, GitLab CI compiled the binaries (`build_linux`, `build_windows`) and zipped them. The deployment script then pushed an update to `aff3ct.github.io/download/download_<branch>.csv` containing a direct link to the **GitLab job artifact URL** (`/-/jobs/<CI_JOB_ID>/artifacts/raw/...`).

### The GitHub Actions Challenge
**GitHub Actions does not provide unauthenticated, public download URLs for run artifacts.** Run artifacts are tied to the internal run ID, expire after 90 days (or less), and require authentication via GitHub account or API token to download. Therefore, keeping the old CSV update logic pointing to run artifact URLs will result in broken 404 links for users visiting `aff3ct.github.io`.

### Options for the Team to Discuss

* **Option A: Rolling Pre-Release Assets (Recommended Standard)**
  Instead of creating 10,000 separate GitHub Releases for every development commit, create **one single rolling Pre-Release tag per branch** (e.g., `dev-latest` for `develop`, and `stable-latest` for `master`). Every time CI runs, `softprops/action-gh-release` overwrites the zip binaries attached to that single tag. 
  * *Pros:* Clean release dashboard, permanent public asset URLs (`https://github.com/aff3ct/aff3ct/releases/download/dev-latest/aff3ct_windows...zip`), perfectly matches the CSV update requirement.
  * *Cons:* Does not preserve an infinite historical archive of zip binaries for every individual past commit.

* **Option B: GitHub Releases per Commit / Tag**
  Only trigger binary packaging and release creation when explicit Git version tags (e.g., `v2.4.0`) are pushed, rather than on every push to `develop`.

* **Option C: External Storage (e.g., UBS / Shannon server)**
  Upload the compiled zip binaries via SSH/rsync to an external university server (such as `shannon.ubs.fr`) and update the CSV to point to that server's HTTP URL.

---

## 2. Code Coverage Dashboard (`Pages`)

### Previous GitLab Workflow
GitLab CI ran the regression test suite with `--coverage` enabled, generated an HTML dashboard using `lcov`/`gcov`, and deployed it to **GitLab Pages** (`https://aff3ct.gitlab.io/aff3ct/`).

### Current Status
Because Step 3 (`test.yml` regression tests) is waiting for the provisioning of dedicated IMT Atlantique hardware runners, coverage generation is currently skipped.

### Options for the Team to Discuss
* **Option A: GitHub Pages**
  Once the IMT self-hosted runners are active and `test.yml` is running, deploy the coverage HTML directory to **GitHub Pages** (`https://aff3ct.github.io/aff3ct-coverage/` or similar) using `actions/deploy-pages`.
* **Option B: Codecov / Coveralls Integration**
  Upload coverage XML reports to a dedicated third-party coverage service like Codecov.io, which provides rich pull request annotations and badges out of the box.

---

## 3. SonarQube Analysis (`deploy-sonarqube-linux.sh`)

### Current Status
Requires both static analysis outputs (`cppcheck`, `clang`) and test coverage reports. Commented out until Step 3 (`test.yml`) is active.

### Next Steps
Once Step 3 is active, add `SONAR_TOKEN` and `SONAR_HOST_URL` to the GitHub repository secrets and uncomment the job in `deploy.yml`.

## 4. Ubuntu PPA Management (Launchpad Reference)

The PPA upload job connects to Canonical's **Launchpad** service (`https://launchpad.net`) via `dput`. 

### PPA Archive Locations
Your team's PPAs are hosted under the `~aff3ct` Launchpad team:
* **Development PPA:** `https://launchpad.net/~aff3ct/+archive/ubuntu/aff3ct-dev`
* **Stable PPA:** `https://launchpad.net/~aff3ct/+archive/ubuntu/aff3ct-stable`

### How Uploads & Authentication Work
1. When `deploy.yml` runs, `dput` uploads the source package (`.dsc`, `.tar.gz`, `.changes`) to Launchpad's incoming FTP/HTTP queue.
2. Launchpad authenticates the upload by verifying the **GPG signature** on the `.changes` file. If the GPG key (from `GPG_PRIVATE_KEY` secret) matches a registered maintainer on the `~aff3ct` team, the upload is accepted.
3. Launchpad automatically provisions virtual machines on Canonical's build farm to compile `.deb` packages for the target Ubuntu versions (`jammy`, `noble`, `resolute`).

### How Maintainers Can Verify PPA Status
* **Automated Emails:** The GPG key owner will receive an automated email from Launchpad within 5–10 minutes of a CI run stating either `[Accepted]` or `[Rejected]` (with detailed rejection reasons like version conflicts or signature errors).
* **Web UI Dashboard:** Maintainers can visit the archive links above and click **"View package details"** to monitor live build logs on Canonical's build servers.

---

## Summary Checklist for IT / Maintainers
- [ ] Add `GPG_PRIVATE_KEY` repository secret to enable automated PPA uploading.
- [ ] Align on Binary Distribution (Option A rolling pre-release vs external storage).
- [ ] Provision IMT Atlantique self-hosted runner and attach to repo to unblock Step 3 (Tests) and Coverage.
