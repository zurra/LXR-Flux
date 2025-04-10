# ⚡ LXRFlux – Real-Time GPU-Based Lighting Analysis for Unreal Engine

**LXRFlux** is a cutting-edge Unreal Engine plugin that captures, analyzes, and interprets **HDR lighting** on the GPU — in real-time — with near-zero overhead.


https://github.com/user-attachments/assets/e40c2b66-76a0-49f8-9d3c-ec37cd4d03c0


Developed by the creator of [**LXR**](https://docs.clusterfact.games/docs/LXR).

LXRFlux is a lightweight, insanely fast light detection system built entirely on **render thread** execution and **RDG-based GPU compute shaders**. This is **next-level light detection** — perfect for stealth mechanics, dynamic gameplay, adaptive visuals, and more.

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

1. Clone this repo where ever you want.:
   ```bash
   git clone https://github.com/zurra/LXR-Flux.git
   ```

### 🔧 Building the Project

This repository does **not** include precompiled binaries by default, so you’ll need to build them locally. Make sure you have all the necessary Unreal Engine C++ prerequisites installed (Visual Studio, UE5 source access, etc).

Once cloned, simply open the `FPTemplate.uproject` file to begin the build process. Unreal will automatically generate project files and start compiling the code.

> 💡 Prebuilt binaries are available for your convenience and can be downloaded from the [releases page](https://github.com/zurra/LXR-Flux/releases/tag/release).  
> These binaries include both the plugin and the sample project, and they are built for **Unreal Engine 5.5.4**.

> ⚠️ If you're using a different engine version, it's strongly recommended to build from source to ensure compatibility.

---

## 🚀 How to Use LXRFlux

LXRFlux is plug-and-play and can be used in both **Blueprints** and **C++** with minimal setup.

---

### 🧩 In Blueprints

1. **Add the Component**
   - Open your Actor Blueprint.
   - Click **Add Component** → Search for `LXRFluxComponent`.
   - Select it to attach the real-time lighting detector to your actor.

   🎥 ![AddComponent](https://github.com/user-attachments/assets/4644ce44-26a2-4878-a474-dd85c3f81856)


2. **Access the Data**
   Use these exposed variables in your Blueprint logic:

   - `Luminance` *(float)* – Brightness of the brightest area in view.
   - `Color` *(FLinearColor)* – Average color from visible indirect/direct light.

   🎥 ![GetData](https://github.com/user-attachments/assets/c6814476-1374-4a0d-987c-0bc08846b0f5)

3. **Debug Widgets**

   Use the Console Command `FLXRFlux.debug.capture 1` to enable the LXRFlux Light Detection Debug widget.

`Execute Console Command` node can be used like this:

![UnrealEditor_g7BHJis33U](https://github.com/user-attachments/assets/1d96909a-da65-4dbb-afa1-92fc57c27374)

   
![Debugs](https://github.com/user-attachments/assets/2fb4b609-772f-4292-a97a-0c29766cc810)

The Debug widget contains following data: 
1. Final Smoothed Color
2. Final Smoothed Luminosity
3. Top Capture
4. Bottom Capture


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


### ⚡ Performance Note

**LXRFlux** is *blazing fast*, with nearly all logic running on the **GPU** and **Render Thread**, and minimal overhead on the **Game Thread**.  
The system is designed for maximum efficiency and minimal latency.

However, the only real bottleneck is Unreal Engine's own **Lumen Global Illumination update speed** for **Scene Captures**.  
Unreal does not update Lumen bounce lighting for SceneCaptures every frame, which may occasionally delay lighting accuracy in ultra-dynamic environments.

> 💡 This is an Unreal limitation, *not* a performance issue with LXRFlux.

If you want real-time lighting responsiveness in gameplay, consider experimenting with *Direct Lighting Only Mode* or using [**LXR**](https://docs.clusterfact.games/docs/LXR).

--- 


### 🧪 Tips to Maximize Performance

While **LXRFlux** is already extremely fast, you can further optimize performance with the following best practices:

#### ✅ Reduce Scene Capture Resolution
- Lower the `RenderTextureSize` if you're analyzing simple or localized lighting.
- Even at **32x32**, LXRFlux gives useful luminance and color results due to smart accumulation.

#### ✅ Avoid Redundant SceneCaptures
- Use the **"One Frame Per Capture"** strategy (already implemented in the system).
- Avoid enabling `bCaptureEveryFrame` unless you're debugging.

#### ✅ Use Only Required Channels
- Avoid capturing unnecessary render features:
  - Turn off `ShowFlags.Materials`, `ShowFlags.AntiAliasing`, `ShowFlags.ToneMapper`, etc.
  - Disable PostProcessing if you only care about raw lighting.

#### ✅ Enable Direct Light-Only Mode (Optional)
- If you don't need indirect bounce info, limit detection to **direct lighting** for instant response.

#### ✅ Capture Only What Matters
- Use `ShowOnlyComponents` or `HideComponent()` to restrict SceneCapture to the key actors/lights.
- Example: Only capture your lighting proxy mesh or zone.

#### ✅ Keep SceneCapture Mesh Simple
- Use a low-poly lighting proxy mesh (e.g., a cube dome) to catch bounces efficiently.



---

### 🔁 From LXR to LXRFlux

**LXRFlux** is a spin-off project from the LXR system, designed to be **open-source, accessible**, and perfect for devs wanting **GPU-level light awareness** in their game.

**LXRFlux** is a **lightweight, standalone indirect lighting detection system**—built as a distilled version of the much larger **[LXR]([https://clusterfact.games](https://docs.clusterfact.games/docs/LXR))** system.

After building **LXR**, which includes advanced detection subsystems like:

- 🔦 **Silhouette Detection**  
- 🧠 **Light Memory and Anomaly Tracing**  
- 👁️ **Beam Awareness and Light Volume Sense**  
- 🎮 **Gameplay Ability System (GAS) Integration**

…it became clear that **indirect lighting detection** deserved its own dedicated, GPU-accelerated solution.

**LXRFlux** was born from that realization.

---

### ⚡ Need Real-Time, Fully-Integrated Lighting Detection?

If you’re working on AI stealth mechanics, visibility perception, or light-reactive gameplay and need something **tightly integrated** and **responsive frame-to-frame**, consider using:

- 🎯 **[LXR]([https://clusterfact.games](https://docs.clusterfact.games/docs/LXR))** — the full detection framework for Unreal Engine, including direct, indirect, silhouette and memory-based lighting detection, built to integrate into real games.

- ✅ LXRFlux is used under the hood as its indirect lighting backend.

---


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

   - **LXR** is a complete AI perception and ultimate light detection framework with line-traced lighting, memory of light states, beam visibility, and player silhouette detection.
 
   - **LXRFlux** is your GPU-powered light radar, ideal for lightweight sensing and ambient triggers.


---

## 🧠 About the Author

Developed by [Lord Zurra](https://www.linkedin.com/in/hannes-v%C3%A4is%C3%A4nen-2194403) – the creator of [**LXR**](https://docs.clusterfact.games/docs/LXR), the most advanced light detection plugin for Unreal Engine.

---


## 📄 License

MIT – Free to use, modify, and include in commercial and personal projects. Attribution welcome but not required.

---

## 💬 Feedback & Contributions

Pull requests are welcome. If you have suggestions, bug reports, or cool use cases, open an issue or reach out on [Discord](https://discord.gg/aWvgSa9mKd).

---
