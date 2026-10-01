<div align="center">

```
 ███████╗ ██████╗ ██████╗  ██████╗ ███████╗ ██████╗ ███████╗
 ██╔════╝██╔═══██╗██╔══██╗██╔════╝ ██╔════╝██╔═══██╗██╔════╝
 █████╗  ██║   ██║██████╔╝██║  ███╗█████╗  ██║   ██║███████╗
 ██╔══╝  ██║   ██║██╔══██╗██║   ██║██╔══╝  ██║   ██║╚════██║
 ██║     ╚██████╔╝██║  ██║╚██████╔╝███████╗╚██████╔╝███████║
 ╚═╝      ╚═════╝ ╚═╝  ╚═╝ ╚═════╝ ╚══════╝ ╚═════╝ ╚══════╝
```

### ⚡ Your Linux machine, interrogated. Live. In the terminal. ⚡

**A menu-driven Linux System Information & Resource Monitoring Tool, forged in pure C.**
No frameworks. No dependencies. No `htop` install. Just `/proc`, system calls, and raw attitude.

![Language](https://img.shields.io/badge/language-C-00599C?style=for-the-badge&logo=c&logoColor=white)
![Platform](https://img.shields.io/badge/platform-Linux-FCC624?style=for-the-badge&logo=linux&logoColor=black)
![Compiler](https://img.shields.io/badge/compiler-GCC-A42E2B?style=for-the-badge&logo=gnu&logoColor=white)
![Dependencies](https://img.shields.io/badge/dependencies-ZERO-brightgreen?style=for-the-badge)
![Course](https://img.shields.io/badge/OSSP-25CS2104E-orange?style=for-the-badge)

</div>

---

## 🔥 Why ForgeOS?

Checking a Linux box normally means juggling `uname`, `top`, `free`, `df`, `ps`, and `uptime`, six commands, six output formats, six tabs of chaos.

**ForgeOS crushes all six into one clean, colour-coded, menu-driven dashboard.** Launch it, pick a number, get answers.

> Every number on screen is pulled straight from the kernel's own interfaces. No wrappers. No shortcuts. No lies.

---

## 🧰 Feature Arsenal

| Module | What it gives you |
|---|---|
| 🖥️ **System Information** | OS, kernel version, architecture, hostname, CPU details, current user |
| 🧠 **CPU Monitoring** | CPU utilization percentage + **live monitoring mode** (press `End` to stop) |
| 💾 **Memory Monitoring** | Total / used / available RAM, cached & buffer memory, swap usage, memory status |
| 📀 **Disk Monitoring** | Total / used / free space, usage percentage, inode information |
| ⚙️ **Process Monitoring** | Running processes, PID & PPID, process state, **process tree**, statistics |
| ⏱️ **System Uptime** | Days, hours, minutes, seconds + total CPU time since boot |

### ✨ Interface Highlights

- 🎨 **Colour-coded status**: usage levels change colour as your system heats up
- 📊 **Visual usage bars**: percentages you can read at a glance
- 📦 **Boxed terminal UI**: tidy panels, titled sections, aligned fields
- 🔁 **Smooth menu flow**: jump between modules and return to the main menu without restarting
- 🛡️ **Input handling**: invalid choices get an error message, not a crash

---

## ⚙️ Under the Hood

ForgeOS doesn't shell out to other tools. It talks to Linux directly.

| Data | Source |
|---|---|
| OS / kernel / arch / hostname | `uname()` |
| CPU model & details | `/proc/cpuinfo` |
| CPU utilization | `/proc/stat` (sampled twice, delta-calculated) |
| RAM & swap | `/proc/meminfo` |
| Disk & inodes | `statvfs()` |
| Processes, PID/PPID, state | `/proc/<pid>/stat`, `/proc/<pid>/comm` |
| Uptime & CPU time | `/proc/uptime`, `/proc/stat` |
| Current user | `pwd.h` APIs |

**Stack:** C · GCC · Linux/Ubuntu · `/proc` filesystem · POSIX system calls & APIs

---

## 🚀 Launch Sequence

```bash
# 1. Clone the repo
git clone https://github.com/venkataramavinaybandla-crypto/V-Max_OSSP.git
cd V-Max_OSSP/ForgeOS/Code

# 2. Compile
gcc main.c -o forgeos

# 3. Run
./forgeos
```

**Requirements:** any Linux distro (built and tested on Ubuntu) + GCC. That's the entire list.

---

## 🗂️ Repository Map

```
V-Max_OSSP/
├── ForgeOS/                          ← 🔥 the main event
│   ├── Code/
│   │   └── main.c                    # full source
│   ├── Linux_System_Monitor_OSSP.pptx  # project presentation
│   ├── V-Max_OSSP_Abstract.docx        # project abstract
│   └── README.md                       # project-level notes
├── 2520030437_Practical/             # OSSP practical submissions
└── 2520030437_Skill/                 # OSSP skill submissions
```

---

## 📸 Screenshots

> _Drop your terminal screenshots in `/assets` and swap these in._

| Main Menu | Live CPU Monitor | Process Tree |
|:---:|:---:|:---:|
| `![menu](assets/menu.png)` | `![cpu](assets/cpu.png)` | `![tree](assets/tree.png)` |

---

## 🎓 Academic Context

Built for the **Operating Systems & System Programming (OSSP)** course, **25CS2104E**, 2026–27 Term-I, **Section 08, Team 09**, at **KL University, Hyderabad**.

The goal: prove that a monitoring tool can be built from first principles by understanding how Linux exposes its internals, instead of leaning on pre-built utilities.

---

## 👥 Team Member

| Name | Role |
|---|---|
| **Bandla Venkata Rama Vinay** | Disk monitoring · Testing · Documentation |
---

## 🛣️ Roadmap

- [✅] Per-core CPU breakdown
- [✅] Network interface statistics
- [✅] Sortable process list (by CPU / memory)
- [✅] Export snapshots to a log file

---

<div align="center">

**Built with C, caffeine, and an unreasonable amount of `/proc` reading.**

⭐ **Star the repo if ForgeOS made your terminal cooler.** ⭐

</div>
