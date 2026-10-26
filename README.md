# Sonoderm

**Sonoderm** is a real-time, audio-reactive music visualizer built in modern *C++17* and *OpenGL 3.3 Core*. It generates a deterministic, lite living 3D world for every audio track using *Signed Distance Fields (SDF), Raymarching, Fast Fourier Transform (FFT) analysis,* and *temporal feedback memory.*

## Showcase

![Sonoderm Running Preview](assets/screenshots/preview.png)

*Screenshot of Sonoderm runned in real-time: reacting a music played, using a raymarched geometry with a cosine palette, and displaying a bottom progress line.*

## Features

- **Procedural 3D SDF Raymarching:** Renders volumetric shapes (spheres, rounded boxes, octahedrons, tori) without polygonal meshes.
- **Organic Liquid Fusion:** Blends distinct geometric primitives smoothly via polynomial `smin`.
- **Psychoacoustic Audio Engine:** Uses `miniaudio` with lock-free/thread-safe sample tapping.
- **In-House FFT & AGC:** Cooley-Tukey FFT with Hann windowing and Automatic Gain Control across Bass, Mid, and Treble.
- **Physical Inertia (`MotionBrain`):** Framerate-independent spring-damper filters and audio-driven time dilation (`u_audio_time`).
- **Deterministic Song DNA:** Generates a persistent visual universe (geometry, satellites, cosine color palette) unique to each audio file hash.
- **Temporal Feedback Memory:** Ping-pong FBO pipeline creating persistent light trails and shockwaves.
- **Ghost UI & Freeze State:** Embedded progress line at the viewport base; physical freeze/frost crystallization on pause.
- **Dynamic Resolution:** Responsive viewport handling window resizing and HiDPI displays without geometric distortion.

## Controls

- `SPACE`: Toggle Play / Pause (activates freeze/frost state)
- `ESC` / `Q`: Exit application
- Window Resize: Automatically adapts rendering resolution

## Architecture Pipeline

| Stage | Module | Input | Output / Role |
| :--- | :--- | :--- | :--- |
| **0. Input** | File System | Audio path (MP3/WAV) | Raw file stream & binary header |
| **1. DNA** | `SongDNABuilder` | 64 KB header + filename | FNV-1a Hash $\to$ Unique Palette $(a,b,c,d)$ & Geometry type |
| **2. Stream** | `AudioEngine` | miniaudio streaming | Audio playback + 512 PCM samples ring buffer |
| **3. FFT** | `AudioAnalyzer` | 512 raw samples | Hann Window $\to$ Cooley-Tukey FFT $\to$ AGC Bass/Mid/Treble |
| **4. Physics** | `MotionBrain` | Frequency bands + $\Delta t$ | Framerate-independent smoothed uniforms & $u\_audio\_time$ |
| **5. Pass 1** | `sound.frag` | Uniforms + Previous frame | Offscreen SDF Raymarching + 60% Temporal Feedback FBO |
| **6. Pass 2** | `screen.frag` | Offscreen texture | Direct fullscreen quad blit scaled to active viewport |

## Prerequisites (Linux)

Install standard build tools, CMake, GLFW, and Mesa OpenGL development libraries (only tested on Ubuntu):

```bash
# Debian / Ubuntu
sudo apt update
sudo apt install build-essential cmake libglfw3-dev libgl1-mesa-dev

# Arch Linux
sudo pacman -S base-devel cmake glfw-x11 mesa

# Fedora
sudo dnf install gcc-c++ cmake glfw-devel mesa-libGL-devel
```

## Build and Run

```bash
mkdir build && cd build
cmake ..
cmake --build .
cd ..

# Run default track
./build/sonoderm assets/audio/shinning_star.mp3

# Run any custom track
./build/sonoderm /path/to/song.mp3
```

---

## Appendix: Core Concepts & References

Here are a few resources that I find very useful for grasping important concepts, if you haven’t already done so:

### 1. Signed Distance Fields (SDF)
- **Definition:** Mathematical functions $f(\mathbf{p}): \mathbb{R}^3 \to \mathbb{R}$ that return the shortest orthogonal distance from a point $\mathbf{p}$ to a surface. Sign denotes whether the point is outside ($>0$), on ($=0$), or inside ($<0$) the volume.
- **Reference:** [Inigo Quilez - 3D Distance Functions](https://iquilezles.org/articles/distfunctions/)

### 2. Raymarching (Sphere Tracing)
- **Definition:** A volumetric rendering technique where rays march forward from the camera through pixels by safe step lengths equal to the evaluated SDF distance, until touching a surface threshold or escaping the scene.
- **Reference:** [Jamie Wong - Ray Marching and Signed Distance Functions](https://jamie-wong.com/2016/07/15/ray-marching-signed-distance-functions/)

### 3. Smooth Minimum (`smin`)
- **Definition:** A polynomial blending operator replacing the sharp $C^0$ intersection of $\min(a, b)$ with a smooth $C^1$ transition, simulating fluid surface tension between shapes.
- **Reference:** [Inigo Quilez - Smooth Minimum](https://iquilezles.org/articles/smin/)

### 4. Cosine Gradient Palettes
- **Definition:** Parametric color generation formula $\mathbf{c}(t) = \mathbf{a} + \mathbf{b} \cdot \cos(2\pi(\mathbf{c}\cdot t + \mathbf{d}))$ ensuring continuous, mathematically balanced color harmonies across normalized coordinates.
- **Reference:** [Inigo Quilez - Cosine Palettes](https://iquilezles.org/articles/palettes/)

### 5. Fast Fourier Transform (FFT) & Windowing
- **Definition:** The Cooley-Tukey algorithm computes the Discrete Fourier Transform in $\mathcal{O}(N \log N)$ complexity, translating time-domain audio samples into frequency-domain spectrum bins. A Hann window is applied before FFT to mitigate spectral leakage caused by discontinuous sample boundaries.
- **Reference 1:** [Cooley-Tukey FFT Algorithm (Wikipedia)](https://en.wikipedia.org/wiki/Cooley%E2%80%93Tukey_FFT_algorithm)
- **Reference 2:** [Spectral Leakage and Windowing Functions](https://en.wikipedia.org/wiki/Window_function)

*If you would like to delve deeper into the Fourier transform, you can check out this article written by my fellow friend [Igor MOGOU](https://github.com/IgorGreenIGM): [Deep Dive into Fourier Transform](https://igormogou86.wordpress.com/2025/04/27/deep-dive-into-fourier-transform)*

### 6. Automatic Gain Control (AGC)
- **Definition:** An adaptive normalization algorithm tracking dynamic peaks and applying decay over sliding temporal windows, ensuring audio bands remain bounded within $[0.0, 1.0]$ regardless of source mastering volume.
- **Reference:** [Automatic Gain Control (Wikipedia)](https://en.wikipedia.org/wiki/Automatic_gain_control)

### 7. Ping-Pong Framebuffer Objects (FBO)
- **Definition:** A double-buffering texture setup where one offscreen texture is read while the other is written, swapping targets each frame to achieve feedback loops, motion blur, and temporal persistence.
- **Reference:** [LearnOpenGL - Framebuffers](https://learnopengl.com/Advanced-OpenGL/Framebuffers)

### 8. FNV-1a Hash
- **Definition:** A non-cryptographic, high-dispersion hash function applying bitwise XOR and prime multiplication to generate 64-bit deterministic seeds from arbitrary byte sequences.
- **Reference:** [Fowler-Noll-Vo Hash Function](https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function)