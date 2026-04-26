# DeskDisplay

Windows 桌面系统监控悬浮窗 —— 一个半透明、置顶的桌面小窗口，实时展示 CPU、内存、网络、磁盘、电池等系统状态。

## 功能

- **CPU 监控** — 总体使用率、每核心使用率、60 点历史折线图（绿/黄/红分级着色）
- **内存监控** — 已用/总量、百分比进度条、Top 5 进程
- **网络监控** — IP/MAC 地址、每块网卡下载/上传实时速率
- **磁盘监控** — 所有固定/可移动磁盘的已用/总量/百分比
- **电池监控** — 电量百分比、充电状态（台式机自动隐藏）
- **计算机信息** — 主机名、域/工作组、系统运行时间、Windows 版本
- **点击穿透** — 右键菜单或 F8 切换，穿透后可操作下方桌面图标
- **窗口可拖拽** — 左键拖拽移动位置，位置自动保存到 INI

## 构建

### 环境要求

- Visual Studio 2022（Platform Toolset v145）
- Windows 10 SDK
- C++20

### 构建步骤

```powershell
# 方式 1：Visual Studio IDE
# 打开 DeskDisplay.slnx → 选择 Release x64 → 生成

# 方式 2：命令行
msbuild DeskDisplay.vcxproj /p:Configuration=Release /p:Platform=x64
```

### 代码签名（Release）

Release 构建会在链接完成后自动运行 `signtool.exe` 对 `.exe` 进行 SHA256 签名。签名所需信息：

| 信息 | 来源 |
|------|------|
| 证书文件 (.pfx) | 项目目录下的 `DeskDisplay.pfx` |
| **证书密码** | 系统环境变量 `DeskDisplayCertPassword` |
| 时间戳服务器 | `http://timestamp.digicert.com` |

#### 首次配置步骤

**1. 生成自签名证书**

首先从模板创建脚本：

```powershell
# 复制模板
copy gen-cert.ps1.example gen-cert.ps1

# 编辑 gen-cert.ps1，将 CHANGE_ME 替换为你的密码
notepad gen-cert.ps1
```

然后在项目目录下运行 PowerShell：

```powershell
# 运行证书生成脚本
.\gen-cert.ps1
```

执行成功后会在项目目录生成 `DeskDisplay.pfx`。

> 如果 PowerShell 提示"无法加载文件，因为在此系统上禁止运行脚本"：
> ```powershell
> Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy RemoteSigned
> ```

**2. 设置证书密码环境变量**

`.vcxproj` 通过 `%DeskDisplayCertPassword%` 读取密码，不会把明文密码写入 git。

| 变量名 | 值 |
|--------|-----|
| `DeskDisplayCertPassword` | `你设置的密码`（和 gen-cert.ps1 中的密码一致） |

两种设置方式，任选其一：

**方式一：界面操作**

1. `Win + R` 输入 `sysdm.cpl`，打开"系统属性"
2. 点击 **高级** → **环境变量**
3. 在"用户变量"区域点击 **新建**
4. 填写：
   - 变量名：`DeskDisplayCertPassword`
   - 变量值：你设置的密码（和 gen-cert.ps1 中的密码一致）
5. 点击 **确定**，一路确定关闭窗口

**方式二：PowerShell 操作**

```powershell
[System.Environment]::SetEnvironmentVariable(
    'DeskDisplayCertPassword',
    '你的密码',
    'User'
)
```

> **必须重启 Visual Studio** 才能生效。VS 在启动时继承系统环境变量，运行中修改不会立即反映。

**如果不设置会怎样？**

```
MSB3073: 命令 "signtool sign /fd SHA256 /f "DeskDisplay.pfx" /p ...
已退出，代码为 1。
```

构建不会失败（MSBuild 默认继续），但生成的 `.exe` **没有数字签名**。在资源管理器右键 → 属性 → 数字签名 页签为空，SmartScreen 仍会警告。

**3.（可选）导入证书到受信任发布者**

只在当前开发机上消除 SmartScreen 警告。需要管理员权限：

```powershell
Import-PfxCertificate -FilePath "D:\PycharmProjects\DeskDisplay\DeskDisplay.pfx" `
    -CertStoreLocation "Cert:\CurrentUser\TrustedPublisher" `
    -Password (ConvertTo-SecureString -String "你的密码" -Force)
```

#### 多台开发机器

如果你在多台机器上编译：

1. 把 `DeskDisplay.pfx` 复制到每台机器的项目目录（文件在 `.gitignore` 中，不会随 git 传输）
2. 每台机器执行步骤 2（设环境变量）
3. 记得重启 VS

#### 分发给用户

**自签名证书在其他用户的电脑上不受信任**，他们会看到 SmartScreen 红色警告。要给用户正常使用：

| 方案 | 费用 | SmartScreen |
|------|------|-------------|
| 用户自行导入你的 `.pfx` 证书 | 免费 | 不弹窗 |
| [Azure Trusted Signing](https://azure.microsoft.com/products/trusted-signing/) | ~$10/月 | 积累信誉后通过 |
| EV 代码签名证书（DigiCert/Sectigo） | $300-600/年 | 立即通过 |

## 配置文件

首次运行后在 `%APPDATA%\DeskDisplay\settings.ini` 自动生成：

```ini
[General]
RefreshMs=2000    ; 刷新间隔（毫秒），默认 2000
Opacity=255       ; 窗口透明度（0 全透明 - 255 完全不透明）

[Window]
X=1520            ; 窗口左上角 X 坐标
Y=50              ; 窗口左上角 Y 坐标
```

修改后重启程序生效。

## 日志

日志文件：`%LOCALAPPDATA%\DeskDisplay\DeskDisplay.log`

超过 1 MB 自动轮转。

## 项目结构

```
DeskDisplay/
├── DeskDisplay.cpp        # WinMain 入口、窗口过程、消息循环
├── SystemMonitor.h/.cpp   # 系统数据采集（PDH/WMI/API）
├── OverlayRenderer.h/.cpp # GDI+ 双缓冲渲染
├── Config.h/.cpp          # INI 配置读写
├── Logger.h/.cpp          # 线程安全日志
├── TestRunner.cpp         # SystemMonitor 独立测试
├── framework.h            # 统一链接库声明
├── DeskDisplay.rc         # 资源文件（图标、菜单、版本信息）
├── DeskDisplay.vcxproj    # MSBuild 项目
├── gen-cert.ps1.example   # 自签名证书生成脚本模板
├── gen-cert.ps1           # 你个人的证书生成脚本（不提交 git）
└── DeskDisplay.pfx        # 代码签名证书（不提交 git）
```

## 技术栈

| 技术 | 用途 |
|------|------|
| C++20 | 核心语言 |
| Win32 API | 窗口管理 |
| GDI+ | 抗锯齿 2D 渲染 |
| PDH (Performance Data Helper) | CPU 性能计数器 |
| Toolhelp32 / PSAPI | 进程内存查询 |
| IP Helper API | 网络适配器速率 |
| `UpdateLayeredWindow` | 每像素 Alpha 混合 |
