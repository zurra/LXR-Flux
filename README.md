# ⚡ LXRFlux – Real-Time GPU-Based Lighting Analysis for Unreal Engine

**LXRFlux** is a cutting-edge Unreal Engine plugin that captures, analyzes, and interprets **HDR lighting** on the GPU — in real-time — with near-zero overhead.

Developed by the creator of [**LXR**](https://docs.clusterfact.games/docs/LXR).
LXRFlux is a lightweight, insanely fast subsystem built entirely on **render thread** execution and **RDG-based GPU compute shaders**. This is **next-level light detection** — perfect for stealth mechanics, dynamic gameplay, adaptive visuals, and more.

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
   ```


---

## 🚀 How to Use LXRFlux

LXRFlux is plug-and-play and can be used in both **Blueprints** and **C++** with minimal setup.

---

### 🧩 In Blueprints

1. **Add the Component**
   - Open your Actor Blueprint.
   - Click **Add Component** → Search for `LXRFluxComponent`.
   - Select it to attach the real-time lighting detector to your actor.

   🎥 *[Insert Screenshot: Add Component → LXRFluxComponent]*

2. **Access the Data**
   Use these exposed variables in your Blueprint logic:

   - `Luminance` *(float)* – Brightness of the brightest area in view.
   - `Color` *(FLinearColor)* – Average color from visible indirect/direct light.

   🎥 *[Insert Screenshot: "Get Luminance" and "Get Color" nodes]*

---

### 🧠 In C++

1. **Add the Component in Your Header (.h):**
   ```cpp
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LXR")
   class ULXRFluxComponent* FluxComponent;
   ```

2. **Create It in Your Constructor (.cpp):**
   ```cpp
   FluxComponent = CreateDefaultSubobject<ULXRFluxComponent>(TEXT("LXRFlux"));
   FluxComponent->SetupAttachment(RootComponent);
   ```

3. **Read Lighting Data Anywhere:**
   ```cpp
   float CurrentLuminance = FluxComponent->Luminance;
   FLinearColor CurrentColor = FluxComponent->Color;
   ```

---



## 🧠 About the Author

Developed by [Lord Zurra](https://www.linkedin.com/in/hannes-v%C3%A4is%C3%A4nen-2194403) – the creator of [**LXR**](https://docs.clusterfact.games/docs/LXR), the most advanced light detection plugin for Unreal Engine.

**LXRFlux** is a spin-off project from the LXR system, designed to be **open-source, accessible**, and perfect for devs wanting **GPU-level light awareness** in their game.

### 🧠 LXR vs LXRFlux – What's the Difference?

| Feature                        | **LXR** | **LXRFlux** |
|-------------------------------|----------------------------------|-------------|
| **Detection Type**            | Precise world space *line-traced* light exposure | GPU-averaged HDR scene luminance |
| **Accuracy**                  | Ultra-precise, based on *actual visibility* | Fast approximation via scene capture |
| **Performance**               | Heavier (raycasts, async traces) | Lightweight (pure GPU compute) |
| **Data Granularity**          | Per-AI, Per-Frame, Shadow Silhouettes | Scene-wide average or max values |
| **Use Case**                  | Stealth AI, exposure-based detection, silhouette vision | Ambient lighting checks, visual triggers, global feedback |
| **Post-Processing Aware**     | Yes (before and after PP)        | Yes (via CaptureSource settings) |
| **Extensibility**             | Plugin with deep gameplay integration | Open-source tool, plug-and-play |
| **Core Focus**                | *Perception realism and AI awareness* | *Fast, reactive environmental sensing* |

   - **LXR** is a complete AI perception ultimate light detection framework with line-traced lighting, memory of light states, beam visibility, and player silhouette detection.
 
   - **LXRFlux** is your GPU-powered light radar, ideal for lightweight sensing and ambient triggers.

---


## 📄 License

MIT – Free to use, modify, and include in commercial and personal projects. Attribution welcome but not required.

---

## 💬 Feedback & Contributions

Pull requests are welcome. If you have suggestions, bug reports, or cool use cases, open an issue or reach out on [Discord](https://discord.gg/aWvgSa9mKd).

---
