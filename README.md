<p align="center">
  <img src="https://github.com/Official-Novadesk/novadesk-assets/blob/master/app-logo/logo-text.png?raw=true" alt="Novadesk Logo"/>
  <br>
  <a href="https://novadesk.pages.dev">
    <img src="https://img.shields.io/badge/Website-Visit-3b82f6?style=for-the-badge&logo=google-chrome&logoColor=white" alt="Website"/>
  </a>
  <a href="https://novadesk-docs.pages.dev">
    <img src="https://img.shields.io/badge/Documentation-Read-f59e0b?style=for-the-badge&logo=gitbook&logoColor=white" alt="Documentation"/>
  </a>
  <a href="https://github.com/Official-Novadesk/novadesk/releases/latest">
    <img src="https://img.shields.io/badge/Download-Latest-10b981?style=for-the-badge&logo=github&logoColor=white" alt="Download"/>
  </a>
  <br>
  <img src="https://img.shields.io/github/license/Official-Novadesk/novadesk?style=for-the-badge" alt="License"/>
  <img src="https://img.shields.io/github/v/release/Official-Novadesk/novadesk?style=for-the-badge" alt="Release"/>
  <img src="https://img.shields.io/github/stars/Official-Novadesk/novadesk?style=for-the-badge" alt="Stars"/>
  <img src="https://img.shields.io/github/issues/Official-Novadesk/novadesk?style=for-the-badge" alt="Issues"/>
</p>

Novadesk is a open source powerful desktop widget platform built with C++. It allows users to create system monitors, custom interfaces, and desktop enhancements using familiar JavaScript syntax.

## 🔗 Resources

- 🌐 [Official Website](https://novadesk.pages.dev) - Learn more about Novadesk features and updates.
- 📚 [Documentation](https://novadesk-docs.pages.dev) - Detailed guides on creating widgets and using the platform.

## ❤️ Support / Donate

If you enjoy using Novadesk and want to support its development, consider becoming a patron:

- ☕ [Patreon](https://www.patreon.com/c/officialnovadesk) - Support the project's growth and get exclusive updates.

## Novadesk Ecosystem

Full diagrams (all `src/apps` apps, `novadesk/` layers, render class tree, and data flow): **[src/apps/ARCHITECTURE.md](src/apps/ARCHITECTURE.md)**

```mermaid
flowchart TB
  subgraph src_apps ["src/apps"]
    Novadesk["novadesk/<br/>Runtime EXE"]
    Nwm["nwm/<br/>CLI: init, run, build"]
    Manage["manage_novadesk/<br/>Widget manager GUI"]
    Restart["restart_novadesk/<br/>Restart helper"]
    Ndpkg["ndpkg_installer/<br/>.ndpkg installer"]
    Stub["installer_stub/<br/>Setup bootstrap"]
    Assets["assets/images"]
  end

  Dev["Developer"] --> Nwm
  User["User"] --> Manage
  User --> Novadesk

  Nwm -->|"build"| Stub
  Nwm --> Novadesk
  Manage --> Novadesk
  Ndpkg --> Novadesk
  Restart --> Manage
  Stub -->|"install"| Novadesk

  subgraph novadesk_layers ["novadesk/ internals"]
    JS["scripting/quickjs<br/>JSEngine, PropertyParser, modules"]
    Domain["domain/<br/>Widget, DesktopManager, Animation"]
    Render["render/<br/>Elements, Shapes, Direct2D"]
    Shared["shared/<br/>Settings, Logging, Utils"]
  end

  Novadesk --> JS
  JS --> Domain
  Domain --> Render
  Domain --> Shared
  Render --> Shared
```