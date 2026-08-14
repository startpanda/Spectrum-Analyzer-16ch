# SA16 Spectrum Analyzer

**[中文](README.md)** | [English](README.en.md)

[![Open Source](https://img.shields.io/badge/Open%20Source-Yes-brightgreen.svg)](#)
[![Release](https://img.shields.io/badge/Release-v1.0.0-blue.svg)](#)
[![Platform](https://img.shields.io/badge/Platform-Windows%20x64-0078D4.svg)](#)
[![C++](https://img.shields.io/badge/C%2B%2B-17-00599C.svg?logo=cplusplus)](#)
[![JUCE](https://img.shields.io/badge/JUCE-8.0.6-orange.svg)](#)
[![VST3](https://img.shields.io/badge/Format-VST3-black.svg)](#)
[![Channels](https://img.shields.io/badge/Channels-16-00d4ff.svg)](#)
[![PRs Welcome](https://img.shields.io/badge/PRs-Welcome-ff69b4.svg)](#)

**作者：AI参数之间** · 16 通道实时频谱 / 频谱图分析 VST3 插件

## 简介

SA16 Spectrum Analyzer 是一款面向宿主（DAW）的多通道频谱分析插件，用于实时查看最多 16 路输入的频谱曲线、峰值保持与频谱图（Spectrogram），并提供 Analyzer / Stereo / Mastering / Spectrogram / Surround 等多种视图模式。

显示频段覆盖 **20 Hz – 20 kHz**（受采样率奈奎斯特限制），幅度范围默认到 **-110 dB**，支持对数 / 线性频率轴、可拖动的 dB 视窗顶部，以及示波器风格的平滑迹线渲染。

## 界面预览

<table>
  <tr>
    <td align="center" width="50%"><img src="Docs/Analyzer.png" alt="Analyzer"/><br/><b>Analyzer</b></td>
    <td align="center" width="50%"><img src="Docs/Stereo.png" alt="Stereo"/><br/><b>Stereo</b></td>
  </tr>
  <tr>
    <td align="center" width="50%"><img src="Docs/Surround.png" alt="Surround"/><br/><b>Surround</b></td>
    <td align="center" width="50%"><img src="Docs/Spectrogram.png" alt="Spectrogram"/><br/><b>Spectrogram</b></td>
  </tr>
</table>

## 功能概览

- **16 通道分析**：独立开关 / Solo / Hold，通道条峰值指示
- **多种视图**：Analyzer、Stereo、Mastering、Spectrogram、Surround
- **FFT 可选**：256 – 32768（8 档），多种窗函数（Hann / Hamming / Blackman / Flat-Top / Kaiser）
- **示波器风格迹线**：密集采样 + 低频平滑，磷光余辉
- **专业频谱图**：时间 × 频率热力图（Inferno 色阶），Inspect 读出频率 / 时间 / dB
- **交互**：Freeze、LIN 频率、Inspect、Measure、dB Top 拖动 / 滚轮
- **底部旋钮**：Channel Gain、Reactivity、dB Top、Smoothing、Preamp

## 构建说明

### 环境要求

- Windows ×64
- CMake ≥ 3.22
- Visual Studio 2022
- 同级目录已放置 [JUCE 8.0.6](https://juce.com/)（本工程通过 `../JUCE-8.0.6` 引用）

### 编译 Release

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

### 产物路径

| 格式 | 路径 |
|------|------|
| Standalone | `build/SA16Spectrum_artefacts/Release/Standalone/SA16 Spectrum Analyzer.exe` |
| VST3 | `build/SA16Spectrum_artefacts/Release/VST3/SA16 Spectrum Analyzer.vst3` |

工程在 CMake 中配置了 `COPY_PLUGIN_AFTER_BUILD`，Release 构建后会尝试复制到 `D:/work/audio/vt3/VST3`。若 DAW 正在占用插件文件，请先关闭宿主再重新构建 / 复制。

将 `.vst3` 拷贝到宿主可扫描的插件目录后，在 DAW 中重新扫描即可加载。

## 项目结构（简要）

```
Spectrum Analyzer 16ch/
  CMakeLists.txt
  Source/
    Constants.h              # 布局常量、频率/dB 映射、FFT 尺寸
    PluginProcessor.*        # APVTS 参数、总线与引擎同步
    PluginEditor.*           # 主 UI 布局与坐标轴
    MainStandalone.cpp       # 自定义 Standalone 入口
    DSP/SpectrumEngine.*     # FFT、窗、平滑、磷光、快照
    UI/
      SpectrumDisplay.*      # 频谱迹线 / 频谱图绘制
      NeonKnob.*             # 圆形旋钮
      ChannelStrip.*         # 通道条
      NeonButtonLnf.h        # 霓虹按钮 LookAndFeel
      AboutDialog.h          # 「关于」弹窗
  Docs/                      # 文档配图
```

## 主要参数

| 参数 ID | 说明 |
|---------|------|
| `fft_size` | FFT 点数 |
| `window` | 窗函数类型 |
| `smoothing` | 时间平滑 |
| `reactivity` | 攻击 / 释放响应 |
| `channel_gain` | 通道增益 |
| `preamp` | 前置增益 |
| `zoom` | dB 视窗顶部（dB Top） |
| `linear` | 线性频率轴 |
| `freeze` / `bypass` | 冻结 / 旁通 |

## 依赖与许可

- UI / 插件框架：[JUCE](https://juce.com/)

请遵循各依赖库的原始许可条款使用与分发。

---

## 联系与关注

- 作者：**AI参数之间**
- 邮箱：[zpan477@gmail.com](mailto:zpan477@gmail.com)

欢迎关注微信公众号 **「AI参数之间」**，获取更多音频 / 插件相关内容：

<p align="center">
  <img src="Docs/ai.jpg" alt="微信公众号：AI参数之间" width="220" />
</p>

<p align="center"><b>扫码关注 · 微信公众号「AI参数之间」</b></p>
