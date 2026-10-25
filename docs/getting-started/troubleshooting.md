# Troubleshooting

| Symptom | Check |
| --- | --- |
| `idf.py` not found | Activate the ESP-IDF environment in this shell |
| SDK version mismatch | Use 5.4.2, or deliberately upgrade all SDK pins |
| Cannot open serial port | Check cable, port name, permissions, and other monitors |
| Board reports insufficient flash | Review board flash capacity and partition defaults |
| Target or old settings appear wrong | Use the intended project directory and regenerate its sdkconfig |
| Host tests cannot fetch GoogleTest | Check network access; the first configure needs GitHub |
| Formatting tool rejects its version | Install clang-format 18 and rerun the wrapper |
| Pages or Releases do not run | Review opt-in variables and repository settings |

Do not delete local configuration without saving needed settings. `set-target`
regenerates SDK configuration; understand the effect before changing targets.
For bugs, include SDK version, board, commit, reproduction, and redacted logs.
