# LA-2A Arm Nong Muang — GitHub Ready (FIXED47 PRESET.NAM)

โปรเจกต์ JUCE/C++ สำหรับปลั๊กอิน **VST3 + Standalone** บน Windows พร้อม NeuralAmpModelerCore และไฟล์โมเดลที่แนบมา

## สิ่งที่เพิ่มใน FIXED47

- เพิ่ม `Models/preset.NAM` ซึ่งเป็นสำเนาโมเดล `LA-2A Leveling – Light.nam` ที่ผู้ใช้อัปโหลด (NAM JSON v0.7.0 / `SlimmableContainer`).
- เพิ่มเอกสาร `Models/README.txt` และ `Presets/README.md` อธิบายวิธีโหลดโมเดลและความแตกต่างระหว่าง NAM model กับ parameter preset.
- ปรับบัฟเฟอร์รับ/ส่ง NAM ให้ใช้ชนิด `NAM_SAMPLE` โดยตรง แทนการ cast หน่วยความจำจาก `double*` เป็น `NAM_SAMPLE*` ซึ่งอาจทำให้ข้อมูลเสียงเสียหายเมื่อชนิด sample ไม่ตรงกัน.
- คงค่าเริ่มต้น GAIN 50.0 และ **PEAK REDUCTION 25.0**.
- GitHub Actions จะ build VST3/Standalone, ทดสอบโหลดและประมวลผลเสียงด้วย `Models/preset.NAM` จริง แล้วแนบโมเดลไปใน artifact ด้วย หากโมเดลโหลดไม่ผ่าน workflow จะ fail

> หมายเหตุ: ปุ่ม `load NAM model` ใช้เลือก `Models/preset.NAM` ด้วยตนเอง ไม่ได้ auto-load จาก path ของโปรเจกต์ เพราะ path ของไฟล์โมเดลเปลี่ยนได้เมื่อย้าย VST3 ไปเครื่องอื่น

## โครงสร้างโปรเจกต์

```text
.
├── .github/workflows/build-windows.yml
├── Assets/LA2A_Interface.png
├── Models/preset.NAM
├── Presets/README.md
├── Source/PluginEditor.cpp
├── Source/PluginEditor.h
├── Source/PluginProcessor.cpp
├── Source/PluginProcessor.h
├── CMakeLists.txt
└── README.md
```

## วิธี Build ผ่าน GitHub Actions (แนะนำ)

1. แตกไฟล์ ZIP แล้วเปิดโฟลเดอร์โปรเจกต์ที่มี `CMakeLists.txt` อยู่ระดับบนสุด
2. สร้าง repository ใหม่บน GitHub เช่น `LA2A-Arm-Nong-Muang` แล้วกด **Create repository**
3. อัปโหลดไฟล์และโฟลเดอร์ทั้งหมดใน ZIP ไปที่ root ของ repository (อย่าอัปโหลดโฟลเดอร์ชั้นนอกที่ครอบโปรเจกต์อีกชั้น)
4. ตรวจสอบว่าเห็นไฟล์ `.github/workflows/build-windows.yml` ในหน้า repository โดยเปิด/แสดง hidden files ด้วยหากจำเป็น
5. กด **Commit changes** เพื่อส่งไฟล์ขึ้น GitHub
6. เปิดแท็บ **Actions** → เลือก workflow ชื่อ **Build LA-2A VST3 (Windows)**
7. รอ job `build-windows` ทำงานจนจบ หากสำเร็จจะเป็นเครื่องหมายสีเขียว
8. เปิดรายการ workflow run ล่าสุด เลื่อนลงส่วน **Artifacts** แล้วดาวน์โหลด `LA2A-Windows-VST3`
9. แตก artifact จะได้ไฟล์ `.vst3`, Standalone `.exe` (ถ้าถูกสร้าง) และ `Models/preset.NAM`

ทุกครั้งที่ push ไป branch `main` หรือ `master` หรือเปิด Pull Request workflow จะทำงานอัตโนมัติ นอกจากนี้สั่งเองได้จาก **Actions → Build LA-2A VST3 (Windows) → Run workflow**

## วิธี Build บน Windows ด้วยตนเอง

ต้องติดตั้ง Visual Studio 2022 พร้อม workload **Desktop development with C++**, CMake 3.22+, Git และ Windows 10/11 SDK

เปิด **x64 Native Tools Command Prompt for VS 2022** ที่ root โปรเจกต์ แล้วรัน:

```bat
cmake -S . -B build -A x64 -DCMAKE_BUILD_TYPE=Release -DFETCHCONTENT_QUIET=OFF
cmake --build build --config Release --parallel 2
```

JUCE 8.0.6, NeuralAmpModelerCore v0.6.0 (รองรับ NAM file v0.7.0), Eigen 5.0.1 และ AudioDSPTools จะถูกดึงมาโดย CMake ระหว่าง configure ครั้งแรก จึงต้องเชื่อมต่ออินเทอร์เน็ต

## วิธีเปิดใช้งานใน Nuendo 15

1. ดาวน์โหลด artifact จาก GitHub Actions และแตกไฟล์
2. คัดลอกโฟลเดอร์ `LA-2A Arm Nong Muang.vst3` ไปยังโฟลเดอร์ VST3 ของผู้ใช้ เช่น `%LOCALAPPDATA%\\Programs\\Common\\VST3` (หรือโฟลเดอร์ VST3 ที่ Nuendo สแกนอยู่)
3. เปิด Nuendo 15 แล้วสั่ง rescan ปลั๊กอิน VST3 หากยังไม่ปรากฏ
4. แทรก **LA-2A Arm Nong Muang** ลงบน audio track
5. กดปุ่ม **load NAM model** แล้วเลือก `Models/preset.NAM` ที่อยู่ในโฟลเดอร์ artifact
6. ตรวจข้อความสถานะบนหน้าปลั๊กอิน หากโหลดไม่สำเร็จ ข้อความจะแสดงสาเหตุที่ตรวจพบ

## ค่าตั้งต้น

| Parameter | Default |
|---|---:|
| GAIN | 50.0 |
| PEAK REDUCTION | 25.0 |
| ATTACK | 1.0 |
| RELEASE | 500.0 ms |

## ข้อจำกัดและการตรวจสอบ

- `.NAM` เป็นโมเดล Neural Amp Modeler ไม่ใช่ไฟล์ preset ของปุ่ม/พารามิเตอร์ JUCE โดยตรง
- ตัวโหลดนี้ส่ง JSON ให้ NeuralAmpModelerCore และรองรับโมเดลที่ Core โหลดได้และมี I/O แบบ mono-in / mono-out; ปลั๊กอินสร้าง DSP แยกสำหรับ L/R
- `.gen` ที่เป็น binary proprietary ไม่สามารถแปลงให้เป็น NAM ด้วยการเปลี่ยนนามสกุลได้; ต้องมี decoder/SDK ของรูปแบบนั้น
- ไฟล์ workflow มีขั้นตอน build และรวบรวม artifact แต่การสร้าง ZIP นี้ไม่ได้รัน MSVC build หรือทดสอบเสียงจริงใน Nuendo 15 ดังนั้นต้องรอผล GitHub Actions และทดสอบโหลดโมเดลบนเครื่องจริงก่อนถือว่าผ่าน

## NAM model compatibility note

`Models/preset.NAM` is the full-size (`max_value = 1.0`) WaveNet submodel extracted
from the supplied v0.7.0 `SlimmableContainer`. This converts the container wrapper
to a conventional standalone WaveNet NAM document while preserving the selected
submodel's config and weights. The plugin loads `.nam` files through NAM Core's
canonical file-path loader so Core performs its own schema/version validation.

To use it, open the plugin and choose `Models/preset.NAM` with **Load NAM Model**.
The preset is not a parameter-state preset; it is the neural model itself.


## NAM v0.7.0 compatibility fix (FIXED50)

The project is restored from FIXED48 (the earlier UI/parameter baseline; it does not include the later FIXED49 program-dependent release experiment). The bundled `Models/preset.NAM` is the exact full-size `max_value=1.0` WaveNet child from the uploaded `SlimmableContainer`; its architecture, config, weights, sample rate, and metadata are preserved.

The previous build pinned NeuralAmpModelerCore v0.5.4. This project now pins v0.6.0, whose documented support includes NAM file version 0.7.0. GitHub Actions builds `nam_model_smoke_test` and runs it against `Models/preset.NAM`: the test loads the file through `nam::get_dsp`, checks mono I/O, resets at 48 kHz, processes a 512-sample test signal, and rejects non-finite or all-zero output. This is an automated model-load/process check, not a substitute for listening tests inside Nuendo.

### NAM Core parser registration note

The NAM Core sources are compiled as a CMake OBJECT library. This is intentional:
NAM architecture/config parsers are registered by static initialization in
translation units that may not otherwise be directly referenced. Building NAM
Core as a STATIC archive can allow the linker to omit those object files, which
causes runtime errors such as `No config parser registered for architecture:
WaveNet`. The Windows workflow runs a real load-and-process smoke test against
`Models/preset.NAM` before publishing artifacts.


## Premium vector rotary controls (FIXED53)

The GAIN and PEAK REDUCTION controls are real JUCE rotary sliders connected to APVTS parameters. Their knob faces are rendered entirely in C++ with concentric machined metal bezels, a glossy black annulus, a ten-lobed scalloped grip, a brushed-silver radial-machined aluminum center cap, and a raised silver position pointer that follows the live parameter value. No knob PNG or knob bitmap is loaded; the faceplate PNG remains only for the rest of the plugin panel artwork. Mouse drag, wheel interaction, double-click reset behavior, and host automation continue to use the underlying JUCE sliders.
