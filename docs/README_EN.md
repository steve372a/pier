# Pier Package Installer Tutorial

## Introduction

Pier is a lightweight, free package manager built on Windows Batch and C language. It is fully compatible with Windows XP/7/10/11 and focuses on extreme compatibility. It also supports intelligent software launching.

## Installing Pier

### Method 1: Download Release Version

1. Visit [GitHub Releases](https://github.com/steve372a/pier/releases)
2. Download the latest `Pier-vX.X.X.zip`
3. Extract to any directory (e.g., `E:\Pier`)
4. Add the extracted directory to your system PATH environment variable (optional)

### Method 2: Using Other Package Managers

- **Winget**: `winget install pier`

## Basic Commands

### 1. Install a Package

```batch
# Basic installation
pier install <package-name>

# Example: Install Notepad++
pier install notepadplusplus

# Auto-install (skip confirmation)
pier install <package-name> -y
```

Installation process:
1. Download package metadata from the software source
2. Display package info (name, version, size) (use `-y` to skip)
3. Confirm installation (use `-y` to skip)
4. Download and extract to `app\<package-name>` directory
5. Create desktop shortcut (optional)

### 2. Uninstall a Package

```batch
# Basic uninstall
pier remove <package-name>

# Example: Uninstall Notepad++
pier remove notepadplusplus

# Auto-uninstall (skip confirmation)
pier remove <package-name> -y
```

**Note**: Uninstalling will delete the `app\<package-name>` directory and all its contents, including configuration files!

### 3. Run Installed Software

```batch
# Launch with default settings
pier o <package-name>

# Example: Launch VS Code
pier o vscode

# Launch with alias for specific version
pier o <package-name> <alias>

# Example: Launch with alias
pier o sanakaprix/czadb

# Pass arguments
pier o <package-name> [alias] [args...]
```

### 4. Search for Packages

```batch
# Search online
pier search <keyword>

# Example: Search for Python
pier search python
```

### 5. List Available Packages

```batch
# Get full list from server
pier list
```

## Software Source Management

### View Current Source

```batch
pier sources list
```

### Switch Software Source

```batch
# Interactive switching
pier sources

# Direct switch to specific URL
pier sources change <URL>
```

Default software sources:
- Official: `https://steve372a.github.io/pier-repo`

### Source Configuration File

Source config is saved in `etc\sourceimage.ini`:
```ini
[package_source]
https://steve372a.github.io/pier-repo
[alias_source]
https://steve372a.github.io/pier-repo
```

## Language Settings

### Switch Language

```batch
# Switch to Chinese
pier --setlang set zh-CN

# Or shorthand
pier sl set zh-CN

# Switch to English
pier sl set en-US
```

### Install New Language Pack

```batch
pier --setlang install <language-code>
```

### Reinstall Language Pack

```batch
pier --setlang reins <language-code>
```

Language files are located at `share\language\<language-code>\lang.ini`.

## Alias System

### What is an Alias?

Traditionally, most package managers would install software and leave it at that, requiring users to find and launch the software themselves. Later, Scoop introduced the Shim system, which creates symbolic links to launch software from anywhere, but users still had to input parameters manually. Pier introduces the alias template system, enabling more intelligent preset parameter launching.

An alias template is a preset quick-launch method created by package authors or third parties. It can:

- Pass parameters
- Use placeholders for precise parameter positioning

### Using Aliases

```batch
# Install/run third-party preset package with alias
pier o sanakaprix(third-party alias template author)/czadb(package name)

# Directly launch officially preset alias template
pier o czadb

# The actual package is specified in the [ToUse] field
```

## Directory Structure

```
pier-2.0.0-beta1/
├── bin/                    # Executable files
│   ├── pier-pkg.exe       # Package manager main program
│   ├── pier-op.exe        # Program launcher
│   ├── pier-ver.exe       # Version checker
│   ├── vecho.exe          # Color output tool
│   ├── sed.exe            # Text processing
│   ├── unzip.exe          # Decompression tool
│   └── uma-get.exe        # Download tool
├── etc/                    # Configuration files
│   ├── language.ini       # Language configuration
│   └── sourceimage.ini    # Software source configuration
├── share/                  # Shared data
│   ├── language/          # Language packs
│   │   ├── zh-CN/
│   │   │   └── lang.ini
│   │   └── en-US/
│   │       └── lang.ini
│   ├── cache/             # Cache directory
│   └── metadata/          # Package metadata
│       └── alias/         # Alias templates
├── app/                    # Installed packages
│   └── <package-name>/
├── piec.bat               # Main entry script
└── src/                    # Source code
    ├── pier-pkg-simple.c
    ├── pier-op.c
    ├── pier-ver.c
    └── vecho.c
```

## License

Pier is open source under MIT License.

Third-party components:
- GNU sed (GPLv3): https://www.gnu.org/software/sed/
- GNU wget (GPLv3): https://www.gnu.org/software/wget/
- curl (curl license): https://curl.se/
- Info-ZIP (BSD-style): http://www.info-zip.org/

## Contact

- Author: Sanakaprix
- Email: steve372@foxmail.com
- GitHub: https://github.com/steve372a/pier/
- Website: https://steve372a.github.io/pier/

---

**Thanks for using Pier Package Installer by Sanakaprix.**
