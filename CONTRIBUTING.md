# Contributing to Novadesk

First off, thanks for taking the time to contribute! 🎉

Novadesk is an open-source project, and we love to receive contributions from our community you! There are many ways to contribute, from writing tutorials or blog posts, improving the documentation, submitting bug reports and feature requests, or writing code which can be incorporated into Novadesk itself.

## 🛠️ Reporting Bugs

A good bug report shouldn't leave others needing to chase you up for more information. Therefore, we ask you to investigate carefully, collect information, and describe the issue in detail in your report.

**Please use our [Issue Tracker](https://github.com/Official-Novadesk/Novadesk/issues) to report bugs.**

## 💡 Suggesting Enhancements

This section guides you through submitting an enhancement suggestion for Novadesk, including completely new features and minor improvements to existing functionality.

**Please use our [Issue Tracker](https://github.com/Official-Novadesk/Novadesk/issues) to suggest features.**

## 💻 Development Setup

To start contributing to the codebase, you'll need to set up your development environment.

### Prerequisites
*   **Windows OS** (Project targets Windows)
*   **Visual Studio 2022** (with C++ Desktop Development workload)
*   **Git**

### Building the Project

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/Official-Novadesk/Novadesk.git
    cd Novadesk
    ```

2.  **Run the Setup Script:**
    We have provided a PowerShell script to handle compilation and setup.
    ```powershell
    .\Compile_and_Setup.ps1
    ```
    This script will restore dependencies and verify the build environment.

3.  **Open in Visual Studio:**
    Open `Novadesk.sln` in Visual Studio 2022 to edit C++ source files.

## 📥 Submitting Pull Requests

1.  Fork the repo and create your branch from `master`.
2.  If you've added code that should be tested, add tests.
3.  Ensure your code builds successfully without errors.
4.  Make sure your code follows the existing coding style.
5.  Issue that pull request!

## 📐 Coding Standards

### C++
*   Use modern C++ practices (C++17/20).
*   Follow standard naming conventions (PascalCase for classes/methods, camelCase for variables).
*   Keep headers clean and minimal.

### JavaScript (Widgets)
*   Use strict mode where applicable.
*   Document your widget API functions clearly.
*   Ensure widgets are performant and don't block the UI thread.

## 📜 License

By contributing, you agree that your contributions will be licensed under its GNU General Public License v3.0.
