# ⚡ ImGui Blur Background Demo (Windows) ⚡

A simple proof-of-concept demonstrating how to create **blurred transparent backgrounds** using **C++**, **ImGui**, and native **Windows APIs**.

This was my first attempt at combining ImGui with Windows’ blur effects — after a few hours of experimentation, I finally got it working and wanted to share the source for anyone interested or struggling with the same.

> ⚠️ *I'm new to C++/ImGui development, so don't expect perfect code! I'm just trying to learn just like you.*

---

## 📸 What It Looks Like (might change following updates)

### Main Window UI Example

![Screenshot](preview.png)


---

## ✨ Features

- Transparent ImGui window with **native Windows blur** (`DwmEnableBlurBehindWindow`)
- Custom acrylic/blur using `SetWindowCompositionAttribute`
- Simple **drag-to-move** window logic
- Placeholder UI layout with ImGui tables, ready for expansion
- Rounded corners and styled regions

---

## 🛠️ Requirements

- **Windows 10 or 11**
- Visual Studio 2022 (or compatible)
- Windows transparency effects must be **enabled**  
  *(Settings → Personalization → Colors → Enable "Transparency Effects")*

---

## 🚀 Build & Run

1. Clone the repo:
   ```bash
   git clone https://github.com/ZyphFX/BlurredCheatmenu
