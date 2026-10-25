# Personal fork and upstream workflow

The official upstream repository is `nawuv/esp-idf-template`. Personal development
uses `yhna941/esp-idf-template`, with both main and develop branches retained.

```text
origin    -> personal fork
upstream  -> organization repository
```

Start features from the upstream integration branch:

```sh
git fetch upstream
git switch -c feature/my-change upstream/develop
# Implement and verify the change.
git push -u origin feature/my-change
```

Open a pull request from the personal feature branch to `nawuv:develop`. A release
pull request later promotes organization develop to main. Do not publish stable
releases from experimental fork branches. Pages/release variables are not inherited
from the upstream repository; configure services separately if required.

Forking improves the template itself. Use **Use this template** when starting a
separate smart-farm or robot product. A fork retains history and upstream linkage;
a template copy starts a new product history.

Public API and hardware-dependent changes remain pre-alpha until the acceptance
checks in [Hardware and MQTT](hardware-mqtt.md) are completed. Existing source tags
do not certify physical device behavior.
