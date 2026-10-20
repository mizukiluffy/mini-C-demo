# mini_c_studio.cpp 精简方案

## Context（为什么做）

用户希望在**不影响此前所有功能与界面行为**的前提下，精简单文件程序 `mini_c_studio.cpp`（约 2060 行，C++/EasyX）。该文件经多轮功能叠加，存在死代码、重复样板和大段结构重复。

原则：采用**补丁式、忠实抽取**的精简——不整文件重写、不改 UI 布局、不改交互顺序；每个函数抽取都保证与原逻辑逐分支等价，每步独立编译验证。不追求"为减行数而改"，只做能降低重复且行为可证明不变的改动。

必须保留的功能（回归基准）：
1. 长按 Backspace/Delete 连续删除（KeyRepeat 400ms/50ms）
2. 鼠标拖拽选区 + Shift+方向键选区（含跨行、反向）
3. 右键上下文菜单（剪切/复制/粘贴/删除/全选/查找替换，悬停高亮，点外部关闭）
4. 行号栏；点击行号 = 选中整行；再按 Delete 删整行
5. 当前光标行浅蓝底；编译报错行整行红底 + 行号红；诊断面板逐条列错误
6. 括号匹配高亮（() [] {}，前/后括号、跨行、嵌套）
7. `#include` 行整行绿色；中文不乱码（-fexec-charset=GBK）；无终端（-mwindows）

## 精简项（本次执行）

### 1. 删除死变量 `moved`
`HandleKeyboard()` 中 L1394 声明 `bool moved`，6 处赋值但从不被读取（函数返回 `edited`）。删除声明与全部赋值。

### 2. 提取重复颜色为命名常量
顶部常量区新增并替换所有硬编码处：
- `COLOR_ERR_BG = RGB(255,220,220)`（错误行底）
- `COLOR_LINE_BG = RGB(220,235,250)`（当前行底）
- `COLOR_GUTTER  = RGB(238,238,242)`（行号栏底）
- `COLOR_SEL_BG  = RGB(135,206,250)`（选区蓝）
- `COLOR_INCLUDE = RGB(0,128,0)`（include 绿）
- `COLOR_MATCH   = RGB(0,120,215)`（括号匹配蓝）
颜色值完全不变。

### 3. 合并 `DrawEditArea()` 重复底色判断
每行循环先算一次高亮色 `hlBg`（错误行→COLOR_ERR_BG；否则当前行→COLOR_LINE_BG；否则无），bar 与行号/正文 `setbkcolor` 统一引用，去掉重复三元。

### 4. 抽取方向键逻辑为两个小函数（核心减量项）
现状：`HandleKeyboard()` 中 VK_UP/DOWN/LEFT/RIGHT 四个分支（约 L1476–1548，共 70+ 行），每块三段：
(a) Shift 且无选区 → 锚点设为当前光标并 `active=true`；
(b) 方向专属的移动/越界处理；
(c) Shift 且有选区 → `end=光标`，否则若非 Shift 且有选区 → `active=false`。
其中 (a)(c) 四处**逐字相同**，仅 (b) 不同。

忠实抽取：
- `bool CursorMoveBy(int dr, int dc)`：只做 (b) 的移动。
  - dr=-1/1：行越界判断后移动行、列 clamp 到新行长度（UP/DOWN）。
  - dc=-1：列>0 列--；否则行>0 行--、列=上行长度（LEFT，含跨行）。
  - dc=1：列<当前行长度 列++；否则行<rows-1 行++、列=0（RIGHT，含跨行）。
  - 返回是否实际移动。逻辑与原四分支完全一致。
- `void HandleArrow(int vk, int dr, int dc, bool shiftDown)`：先 `if(!KeyJustPressed(vk)) return;`，再做 (a)、调用 `CursorMoveBy`、再做 (c)。
调用处替换为 4 行：
```
HandleArrow(VK_UP,-1,0,shiftDown); HandleArrow(VK_DOWN,1,0,shiftDown);
HandleArrow(VK_LEFT,0,-1,shiftDown); HandleArrow(VK_RIGHT,0,1,shiftDown);
```
净减约 30–35 行，并消除选区记账的四份重复。抽取后原顶部局部变量 `rows`/`curCols` 若不再被读取则一并清理（先 grep 确认）。

## 不做项（风险高于收益，排除）

- 不动菜单点击 if-else 分发链、`OutText` 字符模型、`IntToStr`、`ContextMenuItem.enabled`（承载禁用变灰机制，改动面大收益小）。
- 不改任何窗口/面板布局常量与绘制顺序。

## 预期结果

- 净减约 55–70 行，消除四份方向键重复与多处颜色样板，可读性明显提升；功能、颜色、交互逐项等价。

## 验证

每项补丁后用既有命令编译，确保 `BUILD_EXIT=0`（-std=c++11 -fexec-charset=GBK -mwindows，链 easyx 等）。全部完成后启动程序，对照上方 7 条功能清单人工回归：
- include 绿、当前行浅蓝；改错编译→报错行红+诊断逐条；
- 点行号选整行→Delete 删整行；鼠标拖拽选区；右键菜单；
- 括号（含后括号、跨行）蓝色高亮；长按退格连删；
- 中文不乱码、启动无黑框。
