# X-AnyLabeling 训练模块实现详解

> 参考源码：`E:\m_code\X-AnyLabeling`
> 核心范围：`anylabeling/views/training/`、`anylabeling/services/auto_training/ultralytics/`

---

## 1. 定位与总体思路

X-AnyLabeling 的训练模块是一个**内嵌在桌面标注程序里的本地训练器**，不是网络服务。它要解决的核心问题是：

1. 标注程序（PyQt6 主进程）**不能被训练卡死**，训练中还要能继续标注
2. 训练可能要用**另一个 Python 环境**（主程序是打包的 exe，里面没有 torch）
3. 训练崩溃、主程序崩溃、环境缺失，**三者互不牵连**
4. 训练过程中的日志、进度、结果图要能实时回显到 UI

它的答案是：**训练跑在独立子进程里，父子之间只靠「临时 JSON 文件进 + stdout 事件流出」通信，进度和结果图直接从磁盘读。**

这套设计里**没有任何网络链路**，所以它天然不存在 ZMQ 那种"订阅晚了丢消息""REQ 卡死""断链"的问题。

---

## 2. 源码地图

```
anylabeling/
├── app.py                                     # CLI 入口，注册 train-worker / training-worker 子命令
├── views/
│   ├── mainwindow.py
│   └── training/
│       ├── ultralytics_dialog.py              # 训练窗口 UI（103KB，单文件 QDialog）
│       └── widgets/
│           └── ultralytics_widgets/
│               ├── custom_widgets.py          # CustomSpinBox / CustomComboBox 等控件
│               └── custom_dialogs.py
└── services/
    └── auto_training/
        └── ultralytics/
            ├── worker.py          # 子进程 worker：probe / train / export 三合一（协议 v1）
            ├── trainer.py         # TrainingManager：拉起子进程、读 stdout、发事件
            ├── environment.py     # EnvironmentManager：环境探测子进程 + 进程树终止
            ├── exporter.py        # ExportManager：导出子进程（与 TrainingManager 同构）
            ├── general.py         # create_yolo_dataset：数据集快照生成
            ├── validators.py      # 配置 / data.yaml / 类别校验
            ├── config.py          # 路径常量、默认超参、状态文案
            ├── _io.py             # JSON / YAML 读写（含 GBK 兼容解码）
            ├── style.py           # QSS 样式
            └── utils.py           # 图片统计等工具
```

**分层职责**

| 层 | 文件 | 职责 |
|---|---|---|
| UI 层 | `ultralytics_dialog.py` | 三个 Tab、状态机、轮询磁盘、事件渲染 |
| 服务层 | `trainer.py` / `environment.py` / `exporter.py` | 管理子进程生命周期，把 stdout 解析成事件 |
| 协议层 | `worker.py` + `_io.py` | 子进程侧：payload 校验、事件发射、实际调用 ultralytics |
| 数据层 | `general.py` / `validators.py` | 数据集快照、参数校验 |

---

## 3. 进程模型

### 3.1 三个角色

```
┌─────────────────────────────────────────────────────────┐
│  主进程（PyQt6 GUI）                                     │
│                                                         │
│  UltralyticsDialog  ──callbacks──►  TrainingManager     │
│        ▲                                │               │
│        │ pyqtSignal(QueuedConnection)   │ threading.Thread│
│        │                                ▼               │
│  TrainingEventRedirector      stdout.readline() 循环     │
│  TrainingLogRedirector                                 │
└─────────────────────────────────────────────────────────┘
                                    │ subprocess.Popen
                                    ▼
┌─────────────────────────────────────────────────────────┐
│  训练子进程（独立 Python 解释器）                          │
│                                                         │
│  python -m anylabeling.app train-worker --payload x.json│
│    └─► run_training_worker_command()                    │
│          └─► YOLO(...).train(**args)                    │
│              stdout/stderr 被重定向为事件流                │
└─────────────────────────────────────────────────────────┘
                                    │
                                    ▼
                        磁盘：runs/<name>/results.csv 等
```

### 3.2 关键点

- **主进程只做两件事**：往子进程写一个 payload JSON，然后阻塞读它的 stdout
- **训练在子进程里**，主进程死了训练照跑，训练死了主进程无感
- **指标不靠推送**，UI 用 `QTimer` 轮询磁盘上的 `results.csv`
- **停止 = 杀整个进程树**，不碰训练进程内部任何对象，因此没有 `trainer.stop` 竞态

---

## 4. 通信协议

X-AnyLabeling 有**两条并行的 worker 协议**，分别对应"同环境"和"外部环境"，两者前缀不同，不能混用。

### 4.1 协议 A：`train-worker` 子命令（同环境，非 external）

- 触发条件：`Env` 字段为空，用启动主程序的同一个 Python
- 命令：
  ```
  python -m anylabeling.app --work-dir <dir> train-worker --payload <file>
  # 打包后：python --work-dir <dir> train-worker --payload <file>
  ```
- payload：**裸 train_args JSON**，没有 version/action 包装
  ```json
  { "data": "...", "model": "...", "project": "...", "name": "...", "epochs": 100 }
  ```
- 入口：`app.py:276` → `trainer.run_training_worker_command(args)`
- 事件前缀：**`__XANYLABELING_TRAIN_EVENT__=`**
- 解析器：`trainer.handle_training_worker_output()`

### 4.2 协议 B：`worker.py` 脚本（外部环境，external）

- 触发条件：`Env` 指定了别的 Python 可执行文件
- 命令：
  ```
  <python> -u <anylabeling>/services/auto_training/ultralytics/worker.py <file>
  ```
- payload：**带版本号的结构化包**
  ```json
  {
    "version": 1,
    "action": "train",              // probe | train | export
    "args": { ... },
    "paths": { "data_directory": "...", "weights_directory": "..." },
    "options": { "auto_install_packages": true }
  }
  ```
- 入口：`worker.main()` → `load_payload()` 严格校验 → `run_train()`
- 事件前缀：**`__XANYLABELING_WORKER_EVENT__=`**
- 解析器：`environment.parse_worker_output()`

### 4.3 选择逻辑（`trainer.py`）

```python
def _create_payload(self, train_args):
    if not self.external_environment:
        return create_training_payload(train_args)      # 协议 A：裸参数
    return create_worker_payload("train", args=..., paths=..., options=...)  # 协议 B

output_handler = parse_worker_output if self.external_environment \
                 else handle_training_worker_output
```

`environment.build_worker_command()` 负责把协议 B 的命令拼出来，`trainer.build_training_worker_command()` 负责协议 A。

### 4.4 事件流格式

子进程 stdout 上混着两类内容：

```
（普通行，ultralytics 的日志）
Epoch    1/100:   12%|...

（带前缀的结构化事件）
__XANYLABELING_TRAIN_EVENT__={"version":1,"event":"training_log","message":"..."}
__XANYLABELING_TRAIN_EVENT__={"version":1,"event":"training_completed","results":"..."}
```

父进程逐行读，按前缀分流：

```python
def handle_training_worker_output(output, notify_callbacks) -> bool:
    cleaned = output.strip()
    if not cleaned.startswith(TRAINING_WORKER_EVENT_PREFIX):
        notify_callbacks("training_log", {"message": cleaned})   # 裸行降级为日志
        return False
    payload = json.loads(cleaned[len(PREFIX):])                  # 有前缀则解析成事件
    event_type = payload.pop("event", "")
    notify_callbacks(event_type, payload)
    return event_type in {"training_completed", "training_error"}  # 返回值 = 是否终态
```

**事件类型全集**

| 事件 | 发出方 | 含义 |
|---|---|---|
| `environment_detected` | worker `run_probe()` | 环境探测成功 |
| `environment_error` | worker / EnvironmentManager | 环境探测失败（带 `error_type`） |
| `training_started` | 见下方"注意" | 训练开始（带 `total_epochs`） |
| `training_log` | 任意 | 单行日志 |
| `training_completed` | worker | 正常结束（**终态**） |
| `training_error` | worker / 父进程兜底 | 异常结束，带 `traceback`（**终态**） |
| `training_stopped` | 父进程 | 用户主动停止（**终态**） |
| `export_started` / `export_log` / `export_completed` / `export_error` | 导出链路 | 同构 |

> **注意（两处不对称）**
> 1. 协议 A 的 `training_started` 由**父进程**在 `_run_training()` 里 emit（因为裸 payload 没有 action，子进程不认得"这是训练"）；协议 B 的 `training_started` 由 **worker 自己**在 `run_train()` 里 emit。
> 2. `training_stopped` **不在** `handle_training_worker_output` 的终态集合里——它根本不走这条解析路径，是父进程在 `stop_event` 分支里直接 `notify_callbacks` 的。

### 4.5 日志重定向的两路殊途同归

worker 侧把 `sys.stdout` / `sys.stderr` 换成 `EventLogStream` / `TrainingWorkerLogStream`，按 `\n` 切行后逐行 emit 成 `training_log`。

但 ultralytics 的 `LOGGER` 是在 import 时创建的：

```python
# ultralytics/utils/__init__.py:504
stream_handler = logging.StreamHandler(sys.stdout)   # 绑定的是当时的原始 stdout
```

之后替换 `sys.stdout` **对这个 handler 无效**。所以实际存在两条日志通路：

| 通路 | 来源 | 路径 |
|---|---|---|
| ① | `print()`、tqdm 进度条（运行时求值 `sys.stderr`） | 被 log_stream 捕获 → 带前缀 JSON 事件 |
| ② | ultralytics `LOGGER`（logging） | 写真实 stdout → 父进程读到裸行 → 降级为 `training_log` |

**两条路最终都是 `training_log` 事件**，所以即使重定向失效日志也不会丢——这是这套协议稳健的地方。

> 副作用：`EventLogStream` 只按 `\n` 切行，tqdm 用 `\r` 刷新进度条，内容会堆在 buffer 里，直到出现 `\n` 或 `flush()` 才一次性 emit。

---

## 5. 训练生命周期（完整时序）

### 5.1 启动

```
用户点 Start Training
  └─► UltralyticsDialog.start_training_from_train_tab()
        ├─ 检查 environment_ready
        ├─ get_current_config()                    # 从控件收集 9 个分节的配置
        ├─ get_training_args(config)
        │    ├─ 非已有数据集 → create_yolo_dataset() 生成时间戳快照
        │    ├─ 已有数据集   → resolve_prepared_dataset() 解析 YAML
        │    └─ 组装 train_args（排除 X-AnyLabeling 专属参数）
        └─► TrainingManager.start_training(train_args)
              ├─ is_training 守卫（已在训练则拒绝）
              ├─ stop_event.clear()
              ├─ is_training = True
              ├─ _create_payload()  → 临时 JSON 文件
              ├─ Thread(_run_training)  daemon
              └─ Thread(_save_settings_config)  daemon
                    └─ 轮询等 save_dir 出现（1s 间隔）
                       然后把 settings.json 拷进 <project>/<name>/settings.json
```

### 5.2 运行（父进程侧 `_run_training`）

```python
self.training_process = subprocess.Popen(
    command,
    stdout=PIPE, stderr=STDOUT,          # 合并流，逐行读
    text=True, encoding="utf-8", errors="replace",
    bufsize=1,                            # 行缓冲
    env=build_worker_environment(...),    # PYTHONIOENCODING=utf-8, YOLO_AUTOINSTALL=...
    creationflags=CREATE_NEW_PROCESS_GROUP | CREATE_NO_WINDOW,   # Windows
)

terminal_event_seen = False
while True:
    if stop_event.is_set():                # 读之前检查
        terminate_process_tree(...)
        notify("training_stopped"); return
    output = stdout.readline()             # 阻塞读一行
    if stop_event.is_set():                # 读之后再检查一次
        terminate_process_tree(...)
        notify("training_stopped"); return
    if output == "" and process.poll() is not None:
        break                              # EOF 且进程已退出 → 正常跳出
    if not output: continue
    if output_handler(output, notify):     # 收到终态事件
        terminal_event_seen = True

# 循环结束：终态兜底
if terminal_event_seen: return             # 子进程已发过终态 → 不重复
if return_code == 0:
    notify("training_completed")           # 静默成功 → 补发成功
else:
    notify("training_error", code=...)     # 崩了但没发事件 → 补发错误
```

**这段是整套设计里最关键的部分**：

- `terminal_event_seen` 保证**恰好一个终态事件**，不多不少
- 退出码非 0 但没收到 `training_error`（例如子进程被 OOM killer 干掉、`os._exit()`、C 层崩溃）时，父进程**主动补发错误事件**
- 这正是"训练异常静默"问题的解法

### 5.3 停止

```python
def stop_training(self, wait=False):
    if not self.is_training: return False
    self.stop_event.set()
    threading.Thread(target=terminate_process_tree, args=(self.training_process,)).start()
    return True
```

`terminate_process_tree()`（`environment.py:175`）：

| 平台 | 第一步（温和） | 等待 | 第二步（强制） |
|---|---|---|---|
| Windows | `taskkill /T /PID <pid>` | 5s | `taskkill /F /T /PID <pid>` |
| POSIX | `os.killpg(..., SIGTERM)` | 5s | `os.killpg(..., SIGKILL)` |
| 兜底 | — | — | `process.kill()` |

注意用的是 **`/T`（杀整个进程树）**，因为 ultralytics 在 Windows 下会开 DataLoader 子进程，只杀父进程会留下孤儿抢 GPU。

停止的完整链路：
1. `stop_event.set()`
2. 后台线程杀进程树
3. 子进程死亡 → stdout EOF → `readline()` 返回 `""`
4. 循环里检查 `stop_event` → emit `training_stopped` → `is_training = False`
5. UI 收到事件 → 状态置 `stop`，停掉轮询定时器

### 5.4 结束后的收尾

`finally` 块：
```python
finally:
    self.training_process = None
    try: os.remove(payload_path)          # 清理临时 payload
    except OSError: pass
```

---

## 6. worker 侧实现

### 6.1 协议 A：`run_training_worker_command()`（`trainer.py:426`）

```python
def run_training_worker_command(args):
    # 1. 统一 stdout/stderr 为 utf-8（Windows 中文路径/日志关键）
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

    # 2. 读裸 payload
    train_args = json.load(open(args.payload, encoding="utf-8"))

    # 3. 接管输出流
    log_stream = TrainingWorkerLogStream(old_stdout)
    sys.stdout = sys.stderr = log_stream

    # 4. 后端无关设置
    matplotlib.use("Agg")                 # 无显示环境下画图

    # 5. 调用 ultralytics（黑盒，不碰内部类）
    model = YOLO(train_args.pop("model"))
    train_args["verbose"] = False         # 关掉 epoch 表格
    train_args["show"] = False
    model.train(**train_args)

    # 6. 终态
    emit("training_completed", ...)
```

异常路径：
```python
except Exception as e:
    emit("training_error", error=str(e), traceback=traceback.format_exc())
    raise SystemExit(1)                   # 退出码非 0，父进程有兜底
```

### 6.2 协议 B：`worker.run_train()`（`worker.py:225`）

比协议 A 多三步：

**① 参数白名单**
```python
allowed = {"data","model","project","name","device","epochs","batch","imgsz",
           "workers", ... , "cache"}     # 约 60 个键
unsupported = set(train_args) - allowed
if unsupported:
    raise ValueError(f"Unsupported training arguments: {sorted(unsupported)}")
```
外部环境的 payload 是外部写进来的，**白名单防止任意参数注入**。

**② 设备预检**
```python
validate_selected_device(torch, train_args.get("device"))
# 逐个 device 跑 1+1 张量运算 + cuda.synchronize，确认卡真的能用
# 而不是等训练到第 100 步才炸
```

**③ 权重目录接管**
```python
configure_weights_directory(weights_directory)
os.chdir(weights_directory)
from ultralytics import settings
settings.update({"weights_dir": weights_directory})
```
让 ultralytics 的下载缓存和默认输出都落到受控目录。

### 6.3 payload 严格校验（`worker.load_payload()`）

```python
if payload.get("version") != PROTOCOL_VERSION: raise   # 协议版本不匹配
if payload.get("action") not in {"probe","train","export"}: raise
# 顶层字段白名单、args/paths/options 类型检查、按 action 的字段白名单、必填 paths
```

**五层校验**，任何不合法 payload 直接拒绝，不会带着脏参数进 `model.train()`。

---

## 7. 环境探测（EnvironmentManager）

打包后的 exe 里没有 torch，必须让用户指定外部 Python，且要**先确认那个环境真能用**。

### 7.1 探测内容（`worker.run_probe()`）

```json
{
  "python_executable": "...",
  "python_version": "3.11.9",
  "torch_version": "2.4.1",
  "ultralytics_version": "8.3.0",
  "cuda_version": "12.1",
  "cpu_available": true,        // 实际跑 1+1 张量验证
  "cuda_available": true,
  "mps_available": false,
  "gpus": [ {"index":0,"name":"NVIDIA RTX 4090","available":true,"error":null} ],
  "cuda_error": null,
  "mps_error": null
}
```

**不是读配置，是实跑张量验证**：`torch.tensor([1.0]) + 1` 必须等于 2，CUDA 还要 `synchronize()`。这样能抓出"驱动坏了""卡被占用""MIG 模式"这类只读 `is_available()` 查不出的问题。

### 7.2 并发控制（`environment.py:219`）

```python
def probe(self, python_executable, ..., timeout=30):
    request_id = uuid.uuid4().hex          # ① 每次探测一个 ID
    with self._lock:
        previous = self._process
        self._request_id = request_id
    if previous: terminate(previous)       # ② 杀掉上一次没结束的探测

    def run_probe():
        process = Popen(...)
        with self._lock:
            if self._request_id != request_id:   # ③ 过期请求直接杀
                terminate(process); return
            self._process = process
        output, _ = process.communicate(timeout=timeout)   # ④ 30s 超时
        ...
    Thread(run_probe, daemon=True).start()
    return request_id
```

**`request_id` + 锁** 解决"用户快速改了三次 Env，前两次的探测结果晚到覆盖了最新结果"的竞态。`_notify_current()` 里也会二次校验 ID。

超时/退出码非 0/无结果，三种情况分别发不同 `error_type`：
`probe_timeout` / `process_exit`（附最后 20 行诊断输出）/ `invalid_output`。

---

## 8. 数据集快照（`general.create_yolo_dataset()`）

训练数据来自标注界面，但**训练期间用户可能还在改标注**。X-AnyLabeling 的做法是训练前做一次快照。

### 8.1 流程

```
image_list（主界面已加载的图片）
   │
   ├─ 逐张找同名 .json 标注
   │    ├─ 无标注        → background_images（背样本）
   │    ├─ 有有效 shape  → valid_images
   │    ├─ only_checked_files 且未勾选 → 跳过
   │    └─ 解析失败      → 按背样本处理
   │
   ├─ random.sample 打乱（保证切分随机）
   ├─ 按 dataset_ratio 切 train / val（默认 0.8）
   │
   ├─ 建目录：<root>/datasets/<task>/<name>_<YYYYmmdd_HHMMSS>/
   │     images/train  images/val  labels/train  labels/val
   │
   ├─ 图片：os.symlink（POSIX）/ _link_or_copy_image（Windows，先试 symlink 失败则 copy2）
   ├─ 标签：LabelConverter.custom_to_yolo() 转 YOLO 格式写盘
   │
   ├─ 写 dataset_info.txt（数量、比例、开关，供排查）
   └─ 写 data.yaml（path/train/val/names/nc）
```

### 8.2 关键决策

| 决策 | 说明 |
|---|---|
| **时间戳目录** | 每次训练一份新快照，历史 run 可回溯 |
| **图片链接而非拷贝** | 快照几乎零 IO 开销、不占空间；Windows 下 symlink 常因权限失败，**自动降级为 `shutil.copy2`** |
| **标注是拷贝不是链接** | `custom_to_yolo` 是格式转换，本来就产出新文件；**原标注改动不影响已生成的快照** |
| **背景样本进训练集** | 无标注图片默认加入 train（除非 `skip_empty_files`），负样本对检测很重要 |
| **`only_checked_files`** | 只用界面里勾选过的图，支持"先粗标一批、只训精选的" |

**这与 ROI 方案的对比**：X-AnyLabeling 是"改数据的副本"，你的 `train_service` 是"不改文件、内存里裁"。前者简单可靠但有 IO/空间成本，后者零成本但必须重写 ultralytics 内部读取逻辑（这正是你 ROI 一堆已知问题的来源）。

---

## 9. 配置系统

### 9.1 九个分节（`get_current_config()`）

| 分节 | 字段 |
|---|---|
| `basic` | project, name, env, model, data, device, dataset_ratio, pose_config |
| `train` | epochs, batch, imgsz, workers, single_cls, classes |
| `strategy` | time, patience, close_mosaic, optimizer, cos_lr, amp, multi_scale |
| `learning_rate` | lr0, lrf, momentum, weight_decay |
| `warmup` | warmup_epochs, warmup_momentum, warmup_bias_lr |
| `augment` | hsv_h/s/v, degrees, translate, scale, shear, perspective |
| `regularization` | dropout, fraction, rect |
| `loss_weights` | box, cls, dfl, pose, kobj |
| `checkpoint` | save_period, val, plots, save, resume, cache, skip_empty_files, only_checked_files |

分节的意义：`get_training_args()` 时把 8 个训练相关分节 `update()` 合并成一个 dict，再**剔除 X-AnyLabeling 专属参数**（`skip_empty_files`、`only_checked_files`）后透传给 ultralytics。

### 9.2 落盘位置

| 文件 | 路径 | 作用 |
|---|---|---|
| `settings.json` | `<trainer_root>/settings.json` | UI 配置（可导入导出 JSON） |
| `settings.json`（副本） | `<project>/<name>/settings.json` | 随 run 归档，`_save_settings_config` 拷贝 |
| `data.yaml` | 快照目录 | ultralytics 数据集描述 |
| `dataset_info.txt` | 快照目录 | 生成统计，供排查 |
| `training_log_*.txt` | `<project>/<name>/logs/` | 关窗时把 UI 日志存盘 |
| `results.csv` | `<project>/<name>/` | **进度的事实来源** |

根目录由 `config.get_trainer_root_dir()` 决定：
```
<work_dir>/xanylabeling_data/trainer/ultralytics/
├── datasets/<task>/<name>_<timestamp>/
├── weights/          # 外部环境下载的 .pt 缓存
├── runs/             # 默认 project 目录
└── settings.json
```

### 9.3 校验（`validators.py`）

| 函数 | 校验内容 | 特殊返回值 |
|---|---|---|
| `validate_basic_config` | project/name 非空、model 存在（外部环境允许裸模型名 `yolo11n.pt`）、data 存在 | `("directory_exists", path)` → 触发"目录已存在"对话框 |
| `validate_data_file` | YAML 能解析、`names` 非空，返回类别列表 | `(True, [names])` |
| `validate_classes` | 类别索引不越界 | |
| `validate_task_requirements` | 任务与图片数量 | |

`validate_basic_config` 的**三态返回**（True / False / "directory_exists"）让 UI 能区分"配置错误"和"需要用户确认覆盖"，是把校验和交互解耦的干净做法。

---

## 10. UI 层（`ultralytics_dialog.py`）

### 10.1 三 Tab 结构

```
Tab 0  Data    → 选任务类型（Classify/Detect/OBB/Segment/Pose）、数据源、类别统计
Tab 1  Config  → Project/Name/Env/Model/Data/Device + Train Settings + Advanced Settings
Tab 2  Train   → 进度条、日志区、结果图网格、Start/Stop/Export 按钮
```

### 10.2 线程安全：Qt 信号跨线程

```python
# __init__ 里
self.log_redirector = TrainingLogRedirector()
self.log_redirector.log_signal.connect(
    self.append_training_log, Qt.ConnectionType.QueuedConnection   # 关键：队列连接
)
self.event_redirector = TrainingEventRedirector()
self.event_redirector.training_event_signal.connect(
    self.on_training_event, Qt.ConnectionType.QueuedConnection
)
self.training_manager.callbacks = [self.event_redirector.emit_training_event]
```

链路：**读 stdout 的工作线程** → `notify_callbacks()` → `emit_training_event()` → `pyqtSignal.emit()` → **Qt 主线程槽函数**。

`QueuedConnection` 保证槽一定在主线程执行，因此 `append_training_log` 里直接改 UI 是安全的。这是 X-AnyLabeling 处理"子线程回 UI"的标准姿势（等价于你的 `_publish`，但**连接是强的、不会丢**）。

### 10.3 状态机

```python
self.training_status = "idle"   # idle | training | completed | error | stop
```

由 `on_training_event()` 驱动迁移：

| 事件 | 目标状态 | UI 动作 |
|---|---|---|
| `training_started` | `training` | 按钮切换、`progress_timer.start(1000)`、`image_timer.start(5000)` |
| `training_completed` | `completed` | 停定时器、刷新进度与图、显示 Export |
| `training_error` | `error` | 停定时器、显示 `ERROR: ...` |
| `training_stopped` | `stop` | 停定时器、显示"用户停止" |
| `training_log` | — | 追加一行日志（带 ANSI 转义清洗） |

状态文案与颜色在 `config.TRAINING_STATUS_TEXTS / COLORS` 里集中定义。

### 10.4 指标：轮询磁盘，不是推送

```python
self.progress_timer = QTimer()
self.progress_timer.timeout.connect(self.update_training_progress)   # 1s
self.image_timer = QTimer()
self.image_timer.timeout.connect(self.update_training_images)        # 5s
```

**进度**（`update_training_progress`）：
```python
results_file = os.path.join(self.current_project_path, "results.csv")
rows = list(csv.reader(f))
self.current_epochs = len(rows) - 1                    # 行数 = 已完成 epoch 数
progress = min(100, int(current_epochs / total_epochs * 100))
```

**结果图**（`update_training_images`）：按 pattern 扫目录
```
train_batch*.jpg                       （前 3 张）
*PR_curve.png / *F1_curve.png / results.png
val_batch*_labels.jpg / val_batch*_pred.jpg
其余图片补足到至少 6 个槽位
```
每 5s 重新 `QPixmap` 加载、缩放到 150×150。

**为什么轮询比推送好**（在这个场景下）：
- 磁盘是**事实来源**，UI 重启、日志漏收都能从 `results.csv` 恢复
- 不依赖子进程"记得发"，少一类丢消息 bug
- 代价是 1s 延迟、拿不到分项 loss——对桌面工具可接受

### 10.5 窗口生命周期

```python
def _prepare_to_close(self, event):
    if background_task_active and not self._application_closing:
        self.hide()          # 训练中关窗 = 只隐藏，不停止
        event.ignore()
        return False
    if training_status in ("completed","error","stop"):
        self.save_training_logs_to_file()   # 日志落盘到 <run>/logs/
    self.clear_cache()
    self.environment_manager.cancel()
    return True
```

| 场景 | 行为 |
|---|---|
| 训练中关窗口 | `hide()`，训练继续，菜单可重新打开 |
| 关主程序且训练中 | `prepare_for_application_close()` 弹确认框 |
| 确认退出 | 杀进程树 |
| 训练结束关窗 | 日志存 `training_log_<status>_<timestamp>.txt` |
| 重新打开 | `load_latest_training_log()` 按 mtime 取最新日志恢复显示 |

### 10.6 已有实验的复用

启动训练前检查 `<project>/<name>/weights/best.pt`：
- 存在 → 询问"直接导出 / 重新训练（覆盖）"
- 选导出 → 状态置 `completed`，跳到 Tab 2，恢复历史日志与结果图，**不启动训练**
- 选重训 → 再询问是否 `shutil.rmtree` 清空目录（默认 No）

---

## 11. 导出与应用模型

### 11.1 ExportManager（`exporter.py`）

**与 TrainingManager 完全同构**：`subprocess.Popen` + 读 stdout + `stop_event` + `terminate_process_tree` + `terminal_event_seen` 兜底。区别只是 action 是 `export`。

### 11.2 依赖自动安装

```python
EXPORT_DEPENDENCIES = {
    "onnx":      (("onnx","onnx>=1.15.0"), ("onnxslim",...), ("onnxruntime",...)),
    "engine":    (("tensorrt","tensorrt>7.0.0,!=10.1.0"),),
    "coreml":    (("coremltools","coremltools>=8.0"),),
    "paddle":    (("paddle","paddlepaddle"), ("x2paddle",...)),
    ...
}
```

缺包时 `install_export_packages()` 起 `pip install` 子进程，**30 分钟超时**，输出逐行转成 `export_log`，装完再 `importlib.util.find_spec()` 复查一遍，仍缺则报错。`auto_install_packages=false` 时改为直接给出可复制的安装命令。

### 11.3 应用模型（`apply_exported_model`）

ONNX 导出完成后：
1. 生成 `<project>/<name>/<name>.yaml`，指向 `weights/best.onnx`
2. `create_auto_labeling_config()` 把它注册成自定义模型
3. 加载进主界面的自动标注面板

**训练 → 导出 → 直接用于自动标注**，闭环在这里完成。

---

## 12. 关键设计决策汇总

| # | 决策 | 解决的问题 |
|---|---|---|
| 1 | 训练跑独立子进程 | GUI 不卡、崩溃隔离、可用外部 Python |
| 2 | payload JSON 文件传参 | 避免命令行转义地狱（中文路径、空格、大量超参） |
| 3 | stdout 事件前缀协议 | 单管道同时传日志和结构化事件，无需第二个 socket |
| 4 | 终态事件去重 + 退出码兜底 | **保证恰好一个终态事件**，杜绝"训练异常静默" |
| 5 | 杀进程树而非设标志位 | 无竞态、无孤儿 DataLoader 进程 |
| 6 | 进度读 `results.csv` | 磁盘是事实来源，可恢复、不丢消息 |
| 7 | 环境探测实跑张量 | 抓出 `is_available()` 查不出的坏驱动 |
| 8 | `request_id` + 锁 | 快速切换 Env 时的结果覆盖竞态 |
| 9 | worker 参数白名单 | 外部 payload 不能注入任意训练参数 |
| 10 | 数据集时间戳快照 | 训练中改标注不影响本次 run |
| 11 | 图片 symlink、标注转换 | 快照零 IO 开销，原数据零污染 |
| 12 | `QueuedConnection` 回 UI | 子线程安全更新界面 |
| 13 | 日志双通路（重定向 + 裸行降级） | 即使 logging handler 绑定原始流也不丢日志 |
| 14 | 协议版本号 | payload 格式演进时不静默出错 |

---

## 13. 局限性（不能照搬的部分）

| 局限 | 说明 |
|---|---|
| **没有暂停/续训** | 只有 start / stop，停止即丢失。长训练误触 Stop 全白跑 |
| **没有多客户端** | 单机单 UI，无法跨机器观测 |
| **指标粒度粗** | 轮询 `results.csv`，拿不到分项 loss（box/cls/dfl），延迟 1s |
| **强耦合 PyQt6** | `pyqtSignal` / `QTimer` 绑死 Qt，headless 服务用不了，需换成别的跨线程机制 |
| **快照有 IO 成本** | 大数据集要拷标注、建链接，Windows symlink 失败会退化成全量拷贝 |
| **单文件巨型 UI** | `ultralytics_dialog.py` 103KB / 2600 行，9 个配置分节全在一个 `get_current_config()` 里，维护成本高 |
| **无鉴权/多租户** | 本地工具不需要，但搬到网络服务必须补 |
| **`training_stopped` 不在解析器终态集合** | 依赖父进程单独分支，两套逻辑要同步维护 |

---

## 14. 对 `train_service` 的可借鉴点

按优先级排序（针对你提到的断链类问题）：

**P0 — 直接消除断链类问题**

1. **终态事件兜底**：你的 `_train_loop` 只在 `except` 里发 `error`，若训练被外部 kill 或 C 层崩溃，什么都没有。加"`status` 为 training 但线程已结束 → 补发错误事件"的兜底。
2. **状态落盘**：把 `training/paused/save_dir/last_ckpt/异常原因` 写进 `output/<alias>/service_state.json`，服务重启后恢复，`status` 也能返回上次的失败原因（解决你的已知问题 11、14）。
3. **`_publish` 加锁 + try/except**（你的已知问题 12），关服期间训练线程推送不再冒泡进 ultralytics callback 链。

**P1 — 去掉竞态**

4. **用 checkpoint 路径代替 `resume=True`**（你的已知问题 2）：`holdon` 直接传 `resume=self.last_ckpt`。
5. **join 后检查 `is_alive()`**（你的已知问题 3），活着就拒绝，绝不起第二个线程。
6. **`pause` 时若 `trainer` 还没赋值就返回错误**（你的已知问题 4），不要先置 `paused=True`。

**P2 — 架构层面（需要你决策）**

7. **是否改成子进程**：X-AnyLabeling 的核心收益来自进程隔离。改的话 ZMQ 服务退化成"父进程"，训练跑子进程，`pause` 语义要么丢弃（学它只有 stop），要么仍走 `trainer.stop` 但**在子进程内实现**（子进程自己管，父进程只发指令），竞态范围缩小到子进程内部。
8. **是否加磁盘轮询作为第二通道**：PUB 推送保留（实时、多客户端），同时 `status` 也读 `results.csv` 兜底——推送丢了还能从磁盘补。

---

## 15. 一图总结

```
X-AnyLabeling                          你的 train_service
─────────────────                      ─────────────────
子进程 + payload JSON                  同进程 daemon 线程
stdout 前缀事件流                      ZMQ PUB/SUB + REQ/REP
终态去重 + 退出码兜底                   只有 except 发 error
进度读 results.csv（磁盘为事实来源）     每 epoch callback 推送
stop = 杀进程树（无竞态）               pause = trainer.stop + resume（多处竞态）
黑盒调 YOLO().train()                  monkey-patch 内部类
数据集时间戳快照                        内存 ROI 裁剪
状态可从磁盘恢复                        状态只在内存
─────────────────                      ─────────────────
无网络 → 无断链                         有网络 → 需要自己保证不丢
无 pause → 无续训竞态                    有 pause → 竞态要一个个堵
```
