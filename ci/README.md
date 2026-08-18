# AFF3CT CI

Shared logic for our pipelines. The CI runs on GitHub Actions
(`.github/workflows/`); `.gitlab-ci.yml` is the historical pipeline, still around
and reusing these same scripts, so check it too when you change one.

Everything runs on runners provided by GitHub, except the regression and coverage
suite (`.github/workflows/test.yml`), which runs on **self-hosted** machines
because it needs hours of CPU. If you administer one of those machines, read on.

## Thread budget of a self-hosted runner

**How many threads a job may use is decided by the administrator of the machine
it runs on**, per runner. The repository has no say in it: it knows neither the
size of your machine, nor how many runners you installed on it, nor what else it
does.

Write it in the runner's own directory — the one where you ran its `config.sh` —
as the user that owns the runner:

```bash
echo 5 > _work/.ci-threads
```

It takes effect on the next job, without restarting the runner service and without
a commit. Repeat it in each runner you installed; since the budget is per runner,
nothing stops you from giving a large one to a runner meant for heavy builds and
small ones to its neighbours on the same machine.

### How it works, and how it can bite

Every job of the suite runs in a container. A container sees the **whole** CPU
count of its host, whatever the number of runners sharing it, and the environment
of the runner process does not reach it — so neither `nproc` nor the runner's
`.env` file can express this budget. The one channel that works is the file
system: the runner bind-mounts `<runner>/_work` to `/__w` in the container.

So the budget is a file, `<runner>/_work/.ci-threads`, holding a number of
threads. The jobs read it through [`ci/tools/threads.sh`](tools/threads.sh), which
exports `THREADS`, which every build and test script uses (`make -j $THREADS`,
`--n-threads $THREADS`). For a job running directly on the host rather than in a
container, `threads.sh` also reads `/etc/aff3ct/ci-threads`.

With no budget set, a job falls back to every CPU it can see and prints a warning
in its log. That fallback is right on a GitHub-hosted runner, which we get to
ourselves, but on a self-hosted machine it means **one job alone can saturate the
machine**, and stopping runners does not help, since each surviving job still
claims every core. If a machine feels overloaded while its runners look idle, that
warning is the first thing to look for in the job logs.

### Checking a machine

From the directory holding your runner installations:

```bash
grep -H . */_work/.ci-threads
```

The log of any build step states the budget it used and where it came from, e.g.
`THREADS=5 (from /__w/.ci-threads)`.

Note that reinstalling a runner (`config.sh --replace`, or recreating its
directory) wipes its `_work`, hence its budget. Set it again afterwards.
