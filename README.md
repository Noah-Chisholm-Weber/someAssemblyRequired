# Some Assembly Required

An open-source educational programming game that helps players understand how high-level programming concepts translate into lower-level, assembly-like operations through interactive challenges and visual systems.

## Required Software

* Windows 11
* Unreal Engine 5.7.4
* Visual Studio 2022 17.14.39
* Git
* Git LFS

Visual Studio components required by the project are specified in the included `.vsconfig`. Import this file through the Visual Studio Installer to install any missing development tools.

## Cloning

Git LFS must be installed and initialized before cloning the project.

```bash
git lfs install
git clone https://github.com/Noah-Chisholm-Weber/someAssemblyRequired.git
cd someAssemblyRequired
```

## Building and Opening

1. Install Unreal Engine 5.7.4 through the Epic Games Launcher.
2. Install Visual Studio 2022 17.14.39 and import the included `.vsconfig` through the Visual Studio Installer.
3. Right-click `someAssemblyRequired.uproject` and select **Generate Visual Studio project files** if project files have not already been generated.
4. Open `someAssemblyRequired.uproject` in Unreal Engine.

If Unreal Engine asks to rebuild missing or out-of-date C++ modules, allow it to do so.

## Running

The project automatically opens `devTest_lvl`, which is configured as both the editor startup map and game default map.

Click **Play** in the Unreal Editor toolbar to run the project in Play In Editor (PIE).

## Git LFS Troubleshooting

If `git lfs` is not recognized, install Git LFS, reopen your terminal, and run:

```bash
git lfs install
```

If Unreal assets are missing, corrupted, or appear as small text files containing a Git LFS pointer, run:

```bash
git lfs install
git lfs pull
```

To check which files in the repository are currently managed by Git LFS:

```bash
git lfs ls-files
```

## Development Workflow

Keep `main` in a usable state. Development should normally be done on short-lived `feature/`, `fix/`, or `docs/` branches and merged through pull requests after a lightweight review or test.

Coordinate before making concurrent changes to shared Unreal binary assets such as `.uasset` and `.umap` files, as these files cannot generally be merged like source code.
