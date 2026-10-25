# Create your project

1. Open the template repository and select **Use this template**.
2. Create a new repository and clone it locally.
3. Rename `espkit_device` in the root `CMakeLists.txt` if desired.
4. Follow [Build and run](build-and-run.md).

Use the stable `main` branch for new projects. Contributors work on `develop`.
A template copy starts a new history; it does not automatically receive upstream updates.

## Make it yours

- Choose concrete implementations in `main/device.cpp`.
- Set execution periods in `main/device_config.hpp`.
- Select product safety limits through `output_policy` in `main/device_config.hpp`.
- Review flash size, console transport, license attribution, and board requirements.
- Keep credentials out of Git and build artifacts.

## Optional repository services

CI builds and tests without repository secrets. Documentation deployment and
GitHub Releases are opt-in through repository variables. Complete
[repository setup](../repository-setup.md) to enable them in your copy.
