```
[UE 5.3 ✅] [UE 5.4 ✅] [UE 5.5 ✅]
```

> ⚠️ **Unreal Engine Compatibility**
>
> - ✅ `main` branch supports **Unreal Engine 5.5+**
>   - Uses `FRHIGPUBufferReadback` (introduced in UE 5.5)
> - ✅ `feature/dev53` branch supports **Unreal Engine 5.3 – 5.4**
>   - Uses custom fallback readback system
>   - Compatible with 5.3 asset format (which also works in 5.4)
>
> 📝 If you're on **5.4**, use the `feature/dev53` branch to avoid:
> - Forward-incompatible assets from 5.5
> - Missing `FRHIGPUBufferReadback` API


# ⚡ LXRFlux – Real-Time GPU-Based Lighting Analysis for Unreal Engine

**LXRFlux** is a cutting-edge Unreal Engine plugin that captures, analyzes, and interprets **HDR lighting** on the GPU — in real-time — with near-zero overhead.


https://github.com/user-attachments/assets/203b4b62-02e8-42f8-a283-1213ba9bbe6d


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


## 🧠 Technical Breakdown

LXRFlux is a lightweight, high-performance lighting analysis system built entirely on Unreal Engine’s **Render Thread** and **Compute Shaders**. It captures average color, maximum luminance, and pixel count from your scene using the RDG (Render Dependency Graph) system, with zero game thread overhead.

### 🔁 Frame Loop Integration

- Uses Unreal’s `OnResolvedSceneColor` callback to hook directly into the **RDG render graph**, right after scene color is resolved.
- Runs entirely on the **Render Thread**, ensuring no interference with the Game Thread.
- LXRFlux supports asynchronous dispatch using **`FTSTicker`** to safely handle readback polling.

### 🎯 Scene Capture & Target

- Two `SceneCaptureComponent2D` instances positioned around a proxy mesh.
- Each capture renders to a **32×32 PF_FloatRGB RenderTarget** (GPU shared).
- These render targets provide full HDR scene information via **FinalColorHDR**.
- Both top and bottom captures are triggered in the same frame, followed by GPU-side analysis.
   -  The next capture round is only queued after the previous GPU processing and readback complete, ensuring low overhead and stable pacing.

### ⚙️ Compute Shader

- A custom HLSL shader processes the two scene captures (top & bottom) in parallel.
- The shader:
  - Loads both textures
  - Averages the RGB values
  - Computes **luminance** using standard coefficients:  
    `dot(float3(0.2126, 0.7152, 0.0722))`
  - Encodes values to `uint` with a defined `LUMINANCE_SCALE` (e.g., 10,000)
  - Uses **`InterlockedAdd`** to accumulate R, G, B, and filtered pixel count
  - Uses **`InterlockedMax`** to track the **maximum luminance**


### 🧮 Compute Shader Outputs (RDG)

The `LXRFlux Analyze` compute shader processes two scene captures (Top & Bottom), combines them, and writes the following values into a single `RWBuffer<uint>`:

| Index | Value                         | Description                                              |
|-------|-------------------------------|----------------------------------------------------------|
| `[0]` | Max Encoded R                 | Maximum Red value found (float × scale → uint)           |
| `[1]` | Max Encoded G                 | Maximum Green value                                      |
| `[2]` | Max Encoded B                 | Maximum Blue value                                       |
| `[3]` | Valid Pixel Count             | Number of pixels above the `LUMINANCE_THRESHOLD`         |
| `[4]` | Max Luminance Encoded         | Maximum luminance value across all pixels                |

All encoded values use:

```hlsl
uint(LinearValue * LUMINANCE_SCALE)
```

The system uses `InterlockedMax` for all RGB and luminance values, allowing it to detect **brightest pixel color** in the scene, not just average.

---

### 🎛️ Smoothing Options (Updated)

Both `Luminance` and `Color` smoothing are applied *after* max values are decoded. While the original system averaged color, the updated implementation tracks **max color (R, G, B)** and **max luminance**, ensuring that brief flashes or bright objects aren't lost in the smoothing.

So now:

- **Color** is smoothed using the *brightest RGB values* captured
- **Luminance** is based on the *maximum brightness in the scene*

This makes LXRFlux especially suited for gameplay-relevant lighting like:

- Lightning flashes
- Flashlight detection
- Short bursts of extreme lighting

### 🧪 Output Interpretation

- Blueprint-accessible properties:
  ```cpp
  UPROPERTY(BlueprintReadOnly, Category="LXRFlux|Detection")
  float Luminance;

  UPROPERTY(BlueprintReadOnly, Category="LXRFlux|Detection")
  FLinearColor Color;
  ```

### 🧩 Modularity

- Everything is encapsulated in a `ULXRFluxComponent`
- Can be dropped into any actor from the Blueprint Editor

---


## ⚙️ Engine Version Compatibility

`LXRFlux` supports **Unreal Engine 5.3+**, with optimized paths for newer versions:

### ✅ UE 5.5 and above
Uses [`FRHIGPUBufferReadback`](https://github.com/EpicGames/UnrealEngine/blob/release/Engine/Source/Runtime/RHI/Public/RHIGPUReadback.h) — a built-in, high-level GPU readback helper.

- ✅ **Clean RDG integration**  
- ✅ **No manual staging buffers or fence polling** (on 5.5+)  
- ✅ **Fully async** — runs entirely on Render Thread + GPU  
- ✅ **Non-blocking readback polling** via `FTSTicker` on Render Thread  

### ♻️ UE 5.3 – 5.4
Falls back to **manual staging buffer logic**:

-  Uses `FRHIStagingBuffer`, `RHICmdList.CopyToStagingBuffer()`
- **Non-blocking readback polling** via `FTSTicker` on Render Thread  
- Reads back with `RHILockStagingBuffer()` and `RHIUnlockStagingBuffer()`
- Slightly more boilerplate, but identical final behavior

➡️ This is handled transparently inside the custom wrapper class `FLXRBufferReadback`, which abstracts away version-specific readback details.

---

### 💡 Why this matters

GPU readback is notoriously tricky in Unreal due to the multithreaded rendering model and asynchronous nature of RDG.  
`LXRFlux` handles this **efficiently and safely**, regardless of engine version.

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

### **Debug Widgets**

   Use the Console Command `FLXRFlux.debug.capture 1` to enable the LXRFlux Light Detection Debug widget.

`Execute Console Command` node can be used like this:

![UnrealEditor_g7BHJis33U](https://github.com/user-attachments/assets/1d96909a-da65-4dbb-afa1-92fc57c27374)

   
![Debugs](https://github.com/user-attachments/assets/2fb4b609-772f-4292-a97a-0c29766cc810)

The Debug widget contains following data: 
1. Final Smoothed Color
2. Final Smoothed Luminosity
3. Bottom Capture
4. Top Capture

---


## 🎛️ Smoothing Options

`LXRFlux` includes flexible **output smoothing** for both **luminance** and **average color**, making lighting data reliable and consistent across frames.

You can choose the smoothing strategy via:

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LXRFlux|Detection|Smoothing")
ELXRFluxSmoothingMethod SmoothMethod;
```

### Supported Modes

| Method                | Description                                                                                  |
|----------------------|----------------------------------------------------------------------------------------------|
| `None`               | No smoothing — instantly returns the most recent captured value.                             |
| `HistoryBuffer`      | Averages the latest N results using a `TCircularHistoryBuffer`. Best for stable values.      |
| `TargetValueOverTime`| Smoothly interpolates toward the newest value using `FMath::CInterpTo`. Great for ambient fade. |

---

### Used for:

- `Luminance` (float)
- `Color` (FLinearColor)

Both use the same smoothing logic, applied in your getter like this:

```cpp
switch (SmoothMethod)
{
    case TargetValueOverTime:
        return FMath::InterpTo(Current, Previous, DeltaTime, SmoothingSpeed);

    case HistoryBuffer:
        FLinearColor Sum = FLinearColor::Black;
        for (auto Sample : History)
        {
            Sum += Sample;
        }
        return Sum / History.Num();

    case None:
    default:
        return Previous;
}
```

Feel free to adjust smoothing speed or buffer size depending on your use case.

---


## 📸 Capture Setup

### 🔍 Selective Actor Inclusion via Component Tags

LXRFlux isolates scene capture visibility using a component-driven filtering system. Only actors explicitly tagged with `ULXRFluxRenderActorComponent` are included in the render pass for analysis.

At LXRFlux Initialization, all instances of this component are gathered:

```cpp
TArray<AActor*> ActorsToRender;

for (TObjectIterator<ULXRFluxRenderActorComponent> Itr; Itr; ++Itr)
{
	AActor* Actor = Itr->GetOwner();
	if (Actor != nullptr)
	{
		ActorsToRender.Add(Actor);
	}
}
```

The collected actors are then whitelisted in the SceneCapture:

```cpp
SceneCapture->ShowOnlyActors.Append(ActorsToRender);
```

This ensures **only the relevant actors** contribute to the capture buffer — avoiding visual contamination from gameplay actors, the LXR system owner, or unrelated geometry.

### 🧱 Use Case

This system is ideal when:
- You need **clean indirect lighting samples** without noise from the full scene.
- You want to define a subset of **proxy or lighting-only geometry**.
- You need to suppress contributions from highly dynamic or emissive actors.

### ✅ To include an actor:
Add `ULXRFluxRenderActorComponent` to it. That’s it.

> This component acts as an opt-in flag. Any actor with it will be included in the next capture cycle.

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
| **Performance**               | Asynchronous, multithreaded CPU analysis via task graph | Lightweight (pure GPU compute) |
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

## 🛠️ Development & Branching Strategy

### 🔀 Git Flow

This repository follows the **[Git Flow](https://nvie.com/posts/a-successful-git-branching-model/)** branching model.

Key branches:

- `main`:  
  Production-ready, stable releases only.
  
- `develop`:  
  Ongoing development. Contains the latest tested but not-yet-released features.

- `feature/*`:  
  Individual feature branches branched off of `develop`.

- `release/*`:  
  Used to prepare a new release. Branched from `develop`, merged into `main` and `develop` when ready.

- `hotfix/*`:  
  For critical fixes to `main`. Merged into both `main` and `develop`.

---


## 🛠️ Git Flow + Unreal Engine Versioning Strategy

This repository uses a **Git Flow**-inspired branching model, adapted for Unreal Engine compatibility.

### 🌿 Branch Overview

- `main`:  
  Stable, production-ready plugin releases.

- `develop`:  
  The main integration branch for the **latest supported Unreal version**.

- `feature/dev54`, `feature/dev53`, etc.:  
  These are **active development branches per Unreal version**.  
  For example:
  - `feature/dev54`: current Unreal Engine 5.4 development
  - `feature/dev53`: maintained branch for 5.3-specific features or fixes

This approach allows:
- 🧩 Clean isolation of version-specific changes (e.g., breaking API changes in 5.4)
- 🔁 Cherry-picking or merging shared features across engine versions
- 🚀 Safe staging of new features before merging into `develop` or tagging for release

---

### ✍️ Example Workflow

1. Develop new feature on a version-specific branch:
   ```bash
   git checkout -b feature/my-awesome-thing feature/dev54
   ```

2. Merge into the main dev branch for that UE version:
   ```bash
   git checkout feature/dev54
   git merge feature/my-awesome-thing
   ```

3. (Optional) Backport to older UE version:
   ```bash
   git checkout feature/dev53
   git cherry-pick <commit_hash>
   ```

4. Tag when stable:
   ```bash
   git tag -a v1.4.0-54 -m "Release for UE 5.4"
   git push origin v1.4.0-54
   ```

---

### 📦 Release Tags

We use semantic versioning with Unreal suffixes:

```
v1.4.0-54   → for Unreal Engine 5.4
v1.3.2-53   → for Unreal Engine 5.3
```

This allows version-specific package distribution.

---

### 🛠️ LXRFlux – Advanced TODO

1. **Investigate Custom Pixel Shader Path (Optional)**
   - Explore replacing `SceneCaptureComponent2D` with a custom render pass or manual scene view capture.
   - Gain finer control over rendering and materials.
   - *Note:* This is highly advanced and likely overkill unless developing an engine fork.

2. **Explore GPU-Only Detection (Experimental)**
   - Consider a full GPU pipeline: compute shader → UAV → light tagging in screen space.
   - This would avoid any CPU readback entirely.
   - *Tradeoff:* Would lose game-thread-friendly access to luminance values.

3. **Consider Async Compute Queue (Not currently needed)**
   - Only relevant if compute shader becomes heavier or starts conflicting with graphics work.
   - Current use is lightweight; no need to split into a dedicated async compute queue yet.

4. **Improve Code Documentation**
   - Add inline comments to key areas of RDG/compute dispatch.
   - Document shader input/output formats and logic.
   - Clarify RenderThread vs GameThread roles throughout.
  

---

## 📚 Want to Learn More?

If you're curious about how this system works under the hood — from RDG buffer setup to GPU fence polling and Render Thread async execution — I've documented some of the key technical patterns and Unreal-specific insights here:

👉 [**Snippets & Notes**](https://docs.clusterfact.games/docs/Snippets/)

Includes breakdowns of:
- RDG & RHI usage in production systems
- Render thread-only GPU workflows
- Polling with `FTSTicker` and deferred GPU readbacks
- Cross-version compatibility strategies for UE 5.3 – 5.5

Whether you're new to GPU readbacks or just want a clean example of RDG integration, it’s all there.

---
