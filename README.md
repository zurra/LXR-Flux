# ⚡ LXRFlux – Real-Time GPU-Based Lighting Analysis for Unreal Engine

**LXRFlux** is a cutting-edge Unreal Engine plugin that captures, analyzes, and interprets **HDR lighting** on the GPU — in real-time — with near-zero overhead.

Developed by the creator of [**LXR**](https://docs.clusterfact.games/docs/LXR), LXRFlux is a lightweight, insanely fast subsystem built entirely on **render thread** execution and **RDG-based GPU compute shaders**. This is **next-level light detection** — perfect for AI awareness, dynamic gameplay, adaptive visuals, and more.

> 🧠 "Know the light. Own the scene."

---

## ✨ Features

- ✅ **True HDR Scene Analysis** from SceneCaptureComponent2D
- ⚡ **Runs entirely on the Render Thread** — ultra-low CPU overhead
- 🎯 **Accurate indirect & direct light detection**
- 📈 Outputs **Max Luminance**, **Average Luminance**, and **RGB Color**
- 🔁 **Real-time updates every frame** or on demand
- 🔬 Built using **Unreal’s RDG system** and **compute shaders**
- 🧪 Includes debug logs, smoothing buffer, and plug-and-play architecture
- 🛠️ 100% Blueprint and C++ compatible

---

## 🚀 Use Cases

- 🌗 Stealth mechanics
- 🔥 Light-based environmental triggers (e.g., blind from a flashbang)
- 🧱 Adaptive materials or visibility effects
- 🌐 Debugging lighting behavior in complex scenes

---

## 🛠️ How It Works

1. **Two SceneCaptures** (Top & Bottom view)
2. Custom **RDG Compute Shader** samples the HDR data
3. GPU accumulates:
   - Max Luminance
   - Encoded RGB
   - Count of bright pixels (thresholded)
4. Results are **read back once the GPU fence is complete**
5. Output is smoothed using `TCircularHistoryBuffer`
6. Final results are available to Blueprint/C++ for gameplay logic

---

## 📦 Installation

1. Clone this repo into your project's `Plugins/` directory:
   ```bash
   git clone https://github.com/zurra/LXR-Flux.git
