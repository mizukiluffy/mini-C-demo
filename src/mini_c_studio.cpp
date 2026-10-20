// ============================================================
// Mini-C Studio 主界面
// 基于 C++ + EasyX 图形库
// 整合 TextBuffer 文本缓冲区 + 文本渲染视图 + 光标移动
// 兼容 GCC 4.7+（Dev-Cpp 自带 MinGW）
// ============================================================

#include <algorithm> // std::find / std::remove（代码折叠集合维护）
#include <cctype>    // isdigit / isalpha / isalnum
#include <cstdio>
#include <cstdlib> // system
#include <cstring>
#include <fstream> // 文件读写
#include <string>
#include <vector> // 撤销/重做栈
#include <set>    // 断点行号集合
#include <windows.h>
#include <commdlg.h> // GetSaveFileName 标准"保存为"对话框
#include <graphics.h>
#include <imm.h> // 输入法状态查询（ImmGetContext/ImmGetConversionStatus）

// ===== 布局变量（窗口可拉伸/全屏，运行时变化）=====
int WINDOW_WIDTH = 1280;          // 当前客户区宽度
int WINDOW_HEIGHT = 800;          // 当前客户区高度
const int INIT_WIDTH = 1280;      // 初始客户区宽度
const int INIT_HEIGHT = 800;      // 初始客户区高度
const int MIN_WIDTH = 800;        // 最小客户区宽度（防止过窄破坏布局）
const int MIN_HEIGHT = 400;       // 最小客户区高度
const int MENU_BAR_HEIGHT = 28;   // 顶部菜单栏高度
const int STATUS_BAR_HEIGHT = 26; // 底部状态栏高度
const int SUB_ITEM_HEIGHT = 22;   // 子菜单每项高度
const int SUB_MENU_WIDTH = 130;   // 子菜单宽度

// 编辑区参数
const int BREAKPOINT_GUTTER_W = 16; // 断点栏宽度（最左侧红点区）
const int LINE_NUM_WIDTH = 56;      // 行号栏宽度（含断点栏 16 + 原行号 40）
const int TEXT_ORIGIN_X = 66;       // 文本起始 x（行号栏右侧 + 边距）
const int TEXT_ORIGIN_Y = 50;       // 文本起始 y
const int LINE_HEIGHT = 20;    // 行高
const int CHAR_WIDTH = 8;      // 等宽字符宽度（16 号宋体 ASCII 约 8px）
const int FONT_SIZE = 16;      // 字体大小

// 诊断面板参数（窗口右侧）
const int DIAG_PANEL_WIDTH = 300;
int DIAG_PANEL_X = WINDOW_WIDTH - DIAG_PANEL_WIDTH; // 运行时变化
const int DIAG_PANEL_Y = MENU_BAR_HEIGHT;           // = 28
int DIAG_PANEL_HEIGHT = WINDOW_HEIGHT - MENU_BAR_HEIGHT - STATUS_BAR_HEIGHT;
const int DIAG_ITEM_HEIGHT = 22;
const int DIAG_TITLE_H = 24;
const int DIAG_MAX_ITEMS = 100; // 防止内存爆炸

// 编辑区实际可用宽度（减去右侧面板）
int EDIT_AREA_WIDTH = DIAG_PANEL_X; // 运行时变化

// ============================================================
// 界面主题（亮色 / 暗色）
// 全部界面颜色集中于此，切换主题只换一份调色板
// ============================================================
struct Theme {
  // 菜单栏与菜单
  COLORREF menuBarBg;     // 菜单栏背景
  COLORREF menuBarHover;  // 菜单栏悬停
  COLORREF highlight;     // 选中/悬停高亮（菜单栏展开、下拉悬停、诊断选中）
  COLORREF menuBorder;    // 菜单栏下边线
  COLORREF dropBg;        // 下拉/右键菜单背景
  COLORREF dropBorder;    // 下拉菜单边框
  COLORREF dropText;      // 菜单文字
  COLORREF dropDisabled;  // 菜单禁用文字
  COLORREF dropSeparator; // 菜单分隔线
  COLORREF dropHover;     // 下拉子菜单项悬停高亮
  // 标签栏（多文档标签）
  COLORREF tabBarBg;      // 标签栏背景（未占用区）
  COLORREF tabBg;         // 非活动标签背景
  COLORREF tabActiveBg;   // 活动标签背景
  COLORREF tabText;       // 非活动标签文字
  COLORREF tabActiveText; // 活动标签文字
  COLORREF tabBorder;     // 标签边框/分隔线
  COLORREF tabCloseText;  // 关闭按钮 ×
  // 编辑区
  COLORREF editBg;         // 编辑区背景
  COLORREF editText;       // 普通文字
  COLORREF lineNumBg;      // 行号栏背景
  COLORREF lineNumText;    // 行号文字
  COLORREF lineNumSep;     // 行号栏分隔线
  COLORREF curLineBg;      // 当前行高亮
  COLORREF errLineBg;      // 错误行背景
  COLORREF errLineNumText; // 错误行行号
  COLORREF selBg;          // 选区背景（深蓝）
  COLORREF selText;        // 选区文字颜色（白）
  COLORREF findHl;         // 查找匹配高亮
  COLORREF cursorColor;    // 光标
  COLORREF bracketColor;   // 括号匹配边框
  COLORREF gutterBoxBg;    // 折叠标记框填充
  COLORREF gutterBoxLine;  // 折叠标记框边框
  COLORREF gutterBoxText;  // 折叠 +/- 符号
  COLORREF foldHint;       // 折叠提示 { ... }
  COLORREF scrollThumb;    // 滚动条滑块
  // 诊断面板
  COLORREF diagBg;      // 面板背景
  COLORREF diagTitleBg; // 标题栏背景
  COLORREF diagBorder;  // 面板边框线
  COLORREF diagMuted;   // 次要文字（无信息/总数）
  COLORREF diagError;   // 错误
  COLORREF diagWarning; // 警告
  COLORREF diagSuccess; // 成功
  COLORREF diagInfo;    // 信息
  // 状态栏
  COLORREF statusBg;
  COLORREF statusBorder;
  COLORREF statusText;
  // 文件路径气泡
  COLORREF tipShadow;
  COLORREF tipBg;
  COLORREF tipBorder;
  COLORREF tipText;
  // 语法着色
  COLORREF keyword;
  COLORREF number;
  COLORREF string;
  COLORREF comment;
  COLORREF preproc;
};

// 亮色主题（默认，仿 VS Code Light）
static Theme MakeLightTheme() {
  Theme t;
  t.menuBarBg = RGB(240, 240, 240);
  t.menuBarHover = RGB(220, 220, 220);
  t.highlight = RGB(200, 220, 245);
  t.menuBorder = RGB(200, 200, 200);
  t.dropBg = RGB(245, 245, 245);
  t.dropBorder = RGB(150, 150, 150);
  t.dropText = RGB(0, 0, 0);
  t.dropDisabled = RGB(160, 160, 160);
  t.dropSeparator = RGB(180, 180, 180);
  t.dropHover = RGB(200, 220, 255);
  t.tabBarBg = RGB(225, 228, 232);
  t.tabBg = RGB(206, 210, 216);
  t.tabActiveBg = RGB(255, 255, 255);
  t.tabText = RGB(80, 80, 80);
  t.tabActiveText = RGB(0, 0, 0);
  t.tabBorder = RGB(180, 180, 180);
  t.tabCloseText = RGB(110, 110, 110);
  t.editBg = RGB(255, 255, 255);
  t.editText = RGB(0, 0, 0);
  t.lineNumBg = RGB(238, 238, 242);
  t.lineNumText = RGB(90, 90, 90);
  t.lineNumSep = RGB(180, 180, 180);
  t.curLineBg = RGB(220, 235, 250);
  t.errLineBg = RGB(255, 220, 220);
  t.errLineNumText = RGB(200, 0, 0);
  t.selBg = RGB(0, 90, 180);      // 深蓝底（暗色主题下也醒目）
  t.selText = RGB(255, 255, 255); // 白字
  t.findHl = RGB(255, 230, 120);
  t.cursorColor = RGB(0, 0, 0);
  t.bracketColor = RGB(0, 120, 215);
  t.gutterBoxBg = RGB(236, 236, 240);
  t.gutterBoxLine = RGB(120, 120, 120);
  t.gutterBoxText = RGB(50, 50, 50);
  t.foldHint = RGB(120, 120, 120);
  t.scrollThumb = RGB(180, 180, 180);
  t.diagBg = RGB(250, 250, 245);
  t.diagTitleBg = RGB(230, 230, 220);
  t.diagBorder = RGB(150, 150, 150);
  t.diagMuted = RGB(120, 120, 120);
  t.diagError = RGB(200, 0, 0);
  t.diagWarning = RGB(180, 120, 0);
  t.diagSuccess = RGB(0, 128, 0);
  t.diagInfo = RGB(60, 60, 200);
  t.statusBg = RGB(225, 225, 225);
  t.statusBorder = RGB(180, 180, 180);
  t.statusText = RGB(0, 0, 0);
  t.tipShadow = RGB(170, 170, 170);
  t.tipBg = RGB(255, 255, 231);
  t.tipBorder = RGB(120, 120, 120);
  t.tipText = RGB(30, 30, 30);
  t.keyword = RGB(0, 0, 200);
  t.number = RGB(180, 0, 0);
  t.string = RGB(180, 0, 0);
  t.comment = RGB(0, 128, 0);
  t.preproc = RGB(0, 128, 0);
  return t;
}

// 暗色主题（仿 VS Code Dark+）
static Theme MakeDarkTheme() {
  Theme t;
  t.menuBarBg = RGB(45, 45, 48);
  t.menuBarHover = RGB(62, 62, 66);
  t.highlight = RGB(38, 79, 120);
  t.menuBorder = RGB(30, 30, 30);
  t.dropBg = RGB(52, 52, 56);
  t.dropBorder = RGB(90, 90, 95);
  t.dropText = RGB(220, 220, 220);
  t.dropDisabled = RGB(110, 110, 110);
  t.dropSeparator = RGB(80, 80, 84);
  t.dropHover = RGB(55, 90, 140);
  t.tabBarBg = RGB(37, 37, 38);
  t.tabBg = RGB(45, 45, 48);
  t.tabActiveBg = RGB(30, 30, 30);
  t.tabText = RGB(150, 150, 155);
  t.tabActiveText = RGB(230, 230, 230);
  t.tabBorder = RGB(60, 60, 62);
  t.tabCloseText = RGB(140, 140, 140);
  t.editBg = RGB(30, 30, 30);
  t.editText = RGB(212, 212, 212);
  t.lineNumBg = RGB(37, 37, 38);
  t.lineNumText = RGB(130, 130, 140);
  t.lineNumSep = RGB(70, 70, 75);
  t.curLineBg = RGB(40, 48, 60);
  t.errLineBg = RGB(80, 40, 42);
  t.errLineNumText = RGB(240, 120, 120);
  t.selBg = RGB(0, 90, 180);
  t.selText = RGB(255, 255, 255);
  t.findHl = RGB(130, 108, 40);
  t.cursorColor = RGB(220, 220, 220);
  t.bracketColor = RGB(0, 150, 255);
  t.gutterBoxBg = RGB(50, 50, 54);
  t.gutterBoxLine = RGB(120, 120, 125);
  t.gutterBoxText = RGB(200, 200, 200);
  t.foldHint = RGB(130, 130, 135);
  t.scrollThumb = RGB(110, 110, 115);
  t.diagBg = RGB(37, 37, 38);
  t.diagTitleBg = RGB(52, 52, 56);
  t.diagBorder = RGB(90, 90, 95);
  t.diagMuted = RGB(140, 140, 145);
  t.diagError = RGB(244, 96, 96);
  t.diagWarning = RGB(230, 170, 70);
  t.diagSuccess = RGB(110, 200, 110);
  t.diagInfo = RGB(120, 150, 240);
  t.statusBg = RGB(45, 45, 48);
  t.statusBorder = RGB(30, 30, 30);
  t.statusText = RGB(200, 200, 200);
  t.tipShadow = RGB(90, 90, 90);
  t.tipBg = RGB(75, 75, 68);
  t.tipBorder = RGB(130, 130, 125);
  t.tipText = RGB(235, 235, 225);
  t.keyword = RGB(86, 156, 214);
  t.number = RGB(181, 206, 168);
  t.string = RGB(206, 145, 120);
  t.comment = RGB(106, 153, 85);
  t.preproc = RGB(197, 134, 192);
  return t;
}

static const Theme LIGHT_PALETTE = MakeLightTheme(); // 亮色调色板（常驻）
static const Theme DARK_PALETTE = MakeDarkTheme();   // 暗色调色板（常驻）
int g_themeIdx = 0;                                  // 0 = 亮色，1 = 暗色
Theme g_theme = LIGHT_PALETTE;

// 应用指定主题（0 亮 / 1 暗）
void ApplyTheme(int idx) {
  if (idx != 0 && idx != 1)
    idx = 0;
  g_themeIdx = idx;
  g_theme = (idx == 0) ? LIGHT_PALETTE : DARK_PALETTE;
}

// 切换亮/暗主题
void ToggleTheme() { ApplyTheme(1 - g_themeIdx); }

// ============================================================
// 布局更新：根据当前窗口客户区尺寸刷新派生量
// 在 initgraph 后、WM_SIZE、F11 全屏切换时调用
// ============================================================
void UpdateLayout(int newW, int newH) {
  if (newW < MIN_WIDTH)
    newW = MIN_WIDTH;
  if (newH < MIN_HEIGHT)
    newH = MIN_HEIGHT;
  WINDOW_WIDTH = newW;
  WINDOW_HEIGHT = newH;
  DIAG_PANEL_X = WINDOW_WIDTH - DIAG_PANEL_WIDTH;
  DIAG_PANEL_HEIGHT = WINDOW_HEIGHT - MENU_BAR_HEIGHT - STATUS_BAR_HEIGHT;
  EDIT_AREA_WIDTH = DIAG_PANEL_X;
  // 滚动偏移的夹紧在 DrawEditArea 每帧绘制时完成（此处不调 ClampScroll：
  // 启动早期 g_buffer 尚未载入内容，提前 EnsureFolds 会把 g_foldsDirty
  // 复位，反而干扰后续文档载入时的折叠重建）
}

// ============================================================
// TextBuffer - 文本缓冲区（行链表结构）
// ============================================================
struct LineNode {
  char *data;
  int length;
  int capacity;
  LineNode *prev;
  LineNode *next;

  LineNode(int cap = 16) : length(0), capacity(cap), prev(NULL), next(NULL) {
    data = new char[capacity];
  }
  ~LineNode() { delete[] data; }

  void ensureCapacity(int need) {
    if (need <= capacity)
      return;
    int newCap = capacity;
    while (newCap < need)
      newCap *= 2;
    char *nd = new char[newCap];
    memcpy(nd, data, length);
    delete[] data;
    data = nd;
    capacity = newCap;
  }
};

class TextBuffer {
private:
  LineNode *head;
  LineNode *tail;
  int rowCount;

  LineNode *getLineNode(int row) {
    if (row < 0 || row >= rowCount)
      return NULL;
    LineNode *p = head;
    for (int i = 0; i < row && p; i++)
      p = p->next;
    return p;
  }

public:
  TextBuffer() : head(NULL), tail(NULL), rowCount(0) {
    head = tail = new LineNode();
    rowCount = 1;
  }

  ~TextBuffer() {
    LineNode *p = head;
    while (p) {
      LineNode *nx = p->next;
      delete p;
      p = nx;
    }
  }

  int getRowCount() { return rowCount; }

  int getColCount(int row) {
    LineNode *n = getLineNode(row);
    return n ? n->length : 0;
  }

  char getChar(int row, int col) {
    LineNode *n = getLineNode(row);
    if (!n || col < 0 || col >= n->length)
      return '\0';
    return n->data[col];
  }

  std::string getLine(int row) {
    LineNode *n = getLineNode(row);
    if (!n)
      return "";
    return std::string(n->data, n->length);
  }

  std::string getText() {
    std::string text;
    LineNode *p = head;
    bool first = true;
    while (p) {
      if (!first)
        text += '\n';
      text.append(p->data, p->length);
      first = false;
      p = p->next;
    }
    return text;
  }

  void insertChar(int row, int col, char ch) {
    LineNode *n = getLineNode(row);
    if (!n)
      return;
    if (col < 0)
      col = 0;
    if (col > n->length)
      col = n->length;
    n->ensureCapacity(n->length + 1);
    memmove(n->data + col + 1, n->data + col, n->length - col);
    n->data[col] = ch;
    n->length++;
  }

  void deleteChar(int row, int col) {
    LineNode *n = getLineNode(row);
    if (!n || col < 0 || col >= n->length)
      return;
    memmove(n->data + col, n->data + col + 1, n->length - col - 1);
    n->length--;
  }

  void insertLineBreak(int row, int col) {
    LineNode *n = getLineNode(row);
    if (!n)
      return;
    if (col < 0)
      col = 0;
    if (col > n->length)
      col = n->length;

    LineNode *newNode = new LineNode(16);
    int rightLen = n->length - col;
    if (rightLen > 0) {
      newNode->ensureCapacity(rightLen);
      memcpy(newNode->data, n->data + col, rightLen);
      newNode->length = rightLen;
    }
    n->length = col;

    newNode->prev = n;
    newNode->next = n->next;
    if (n->next)
      n->next->prev = newNode;
    else
      tail = newNode;
    n->next = newNode;
    rowCount++;
  }

  void deleteLine(int row) {
    LineNode *n = getLineNode(row);
    if (!n)
      return;
    if (rowCount == 1) {
      n->length = 0;
      return;
    }
    if (n->prev)
      n->prev->next = n->next;
    else
      head = n->next;
    if (n->next)
      n->next->prev = n->prev;
    else
      tail = n->prev;
    delete n;
    rowCount--;
  }

  void setText(const std::string &text) {
    LineNode *p = head;
    while (p) {
      LineNode *nx = p->next;
      delete p;
      p = nx;
    }
    head = tail = NULL;
    rowCount = 0;

    size_t start = 0;
    size_t len = text.size();
    while (start <= len) {
      size_t pos = text.find('\n', start);
      size_t end = (pos == std::string::npos) ? len : pos;

      LineNode *node = new LineNode(16);
      int lineLen = (int)(end - start);
      if (lineLen > 0) {
        node->ensureCapacity(lineLen);
        memcpy(node->data, text.c_str() + start, lineLen);
        node->length = lineLen;
      }
      node->prev = tail;
      node->next = NULL;
      if (tail)
        tail->next = node;
      else
        head = node;
      tail = node;
      rowCount++;

      if (pos == std::string::npos)
        break;
      start = pos + 1;
    }

    if (rowCount == 0) {
      head = tail = new LineNode();
      rowCount = 1;
    }
  }
};

// ============================================================
// 全局状态
// ============================================================
std::string g_fileName = "未命名"; // 显示用文件名（不含路径）
std::string g_filePath = "";       // 完整路径（空表示尚未保存到磁盘）
bool g_modified = false;           // false = "已保存"

TextBuffer g_buffer; // 文本缓冲区

// 临时文件名（编译时用）
const char *const TEMP_C = "temp.c";
const char *const TEMP_EXE = "temp.exe";
const char *const ERR_TXT = "error.txt";

// gcc 路径（编译时优先使用，避免 PATH 环境变量缺失导致找不到编译器）
// 候选位置：Dev-Cpp 自带 MinGW64
std::string g_gccPath = ""; // 空表示尚未确定，编译时动态检测

// ===== 诊断信息结构 =====
struct DiagEntry {
  int line;         // 行号（1-based，0 表示无法解析）
  int col;          // 起始列号（1-based，0 表示未知；字节列）
  int colEnd;       // 结束列号（1-based 排他列，0 表示未知 —— 波浪线终点）
  std::string file; // 诊断所属文件（空表示当前文件/命令行级诊断）
  std::string type; // "error" / "warning" / "note" 等
  std::string desc; // 错误描述
};

// 诊断来源：0 = 构建输出（gcc 编译错误，整行标红）
//           1 = 静态诊断（clang JSON 或 gcc -fsyntax-only，画波浪线）
int g_diagSource = 0;

// 简易动态数组（不使用 std::vector，自管理内存）
DiagEntry *g_diagList = NULL;
int g_diagCount = 0;
int g_diagCap = 0;
int g_diagSelIdx = -1;  // 鼠标选中的错误索引（-1 表示未选中）
bool g_buildOk = false; // 上次编译是否成功（初始未编译）

// 诊断面板双击检测（不依赖窗口类 CS_DBLCLKS，自行计时）
DWORD g_diagClickTick = 0; // 上次在诊断项上按下的时刻
int g_diagClickIdx = -1;   // 上次按下的诊断项索引
int g_diagClickX = 0, g_diagClickY = 0; // 上次按下的鼠标位置

// 高亮目标行（点击错误后定位的行，-1 表示不高亮）
int g_highlightLine = -1;

// 查找状态（区分大小写）：当前展示的匹配位置；切换标签时清除
bool g_findActive = false; // 是否有正在展示的查找结果
std::string g_findText;    // 当前查找关键词
int g_findRow = -1;        // 当前匹配起始行（0-based）
int g_findCol = -1;        // 当前匹配起始列（0-based，按字节）

void CloseFindBox(); // 定义在"查找"函数区（供 LoadDocState 提前调用）

void DiagReserve(int cap) {
  if (cap <= g_diagCap)
    return;
  DiagEntry *nd = new DiagEntry[cap];
  for (int i = 0; i < g_diagCount; i++)
    nd[i] = g_diagList[i];
  delete[] g_diagList;
  g_diagList = nd;
  g_diagCap = cap;
}

void DiagClear() {
  delete[] g_diagList;
  g_diagList = NULL;
  g_diagCount = 0;
  g_diagCap = 0;
  g_diagSelIdx = -1;
  g_highlightLine = -1;
}

void DiagAdd(const DiagEntry &e) {
  if (g_diagCount >= DIAG_MAX_ITEMS)
    return;
  if (g_diagCount >= g_diagCap)
    DiagReserve((g_diagCap == 0) ? 16 : g_diagCap * 2);
  g_diagList[g_diagCount++] = e;
}

// 光标对象
struct Cursor {
  int row;
  int col;
};
Cursor g_cursor = {0, 0};

// ===== 滚动偏移（视图行/列）=====
// g_topLine：当前编辑区最顶端显示的"视图行"号（折叠后映射过的）
// g_leftCol：当前编辑区最左侧显示的字节列号
// g_wheelPending / g_hWheelPending：鼠标滚轮累计 delta（主线程消费）
//   垂直滚轮每 WHEEL_DELTA(120) 一档，滚动 3 行
//   水平滚轮（Shift+Wheel 或侧键）每档滚动 8 列
int g_topLine = 0;
int g_leftCol = 0;
int g_wheelPending = 0;
int g_hWheelPending = 0;

// 滚动条拖拽状态
// g_vScrollDrag：0=未拖 1=拖垂直 2=拖水平
// 拖拽时记录鼠标按下时滑块顶/左像素位置与鼠标坐标之差，
// 移动时用 "鼠标坐标 - 偏移" 反算 g_topLine / g_leftCol
int g_scrollDrag = 0;
int g_scrollDragOff = 0;   // 鼠标到滑块顶/左端的像素差

// ===== 选区（用于复制/剪切/粘贴）=====
// 选区用起止位置表示，start/end 自动排序（start < end）
struct Selection {
  int startRow, startCol; // 起点
  int endRow, endCol;     // 终点
  bool active;            // 是否有选区
};
Selection g_sel = {0, 0, 0, 0, false};

// 鼠标拖拽选区状态
bool g_mouseSelecting = false; // 是否正在鼠标拖拽选区
int g_selAnchorRow = 0;        // 选区锚点（鼠标按下处的行列）
int g_selAnchorCol = 0;

// ===== 代码折叠（影子全文模型：g_buffer 永不删行，只做视图映射）=====
// 一个可折叠块 = 以 '{' 开块的行 start 到匹配 '}' 所在行 end
struct Fold {
  int start; // 含开块 '{' 的文档行
  int end;   // 含匹配 '}' 的文档行
};
std::vector<Fold> g_folds;          // 当前文档的全部可折叠块（按 start 升序）
std::vector<int> g_collapsedStarts; // 处于折叠状态的块起点行集合
std::vector<int> g_viewToDoc;       // 视图行 -> 文档行
std::vector<int> g_docToView;       // 文档行 -> 视图行（-1 = 被折叠隐藏）
bool g_foldsDirty = true;           // 文档变化后需重算折叠与映射
bool g_foldsNeedRedraw = false;     // 折叠状态变化（快捷键触发）请求重绘

// ============================================================
// 断点管理（简易调试）
// ============================================================
std::set<int> g_breakpoints; // 断点行号集合（0-based 文档行）

// 切换断点：有则移除，无则添加
void toggleBreakpoint(int row) {
  if (row < 0 || row >= g_buffer.getRowCount())
    return;
  if (g_breakpoints.count(row))
    g_breakpoints.erase(row);
  else
    g_breakpoints.insert(row);
}

// 查询某行是否有断点
bool hasBreakpoint(int row) {
  return g_breakpoints.count(row) > 0;
}

// 获取所有断点行号（升序）
std::vector<int> getBreakpoints() {
  std::vector<int> v(g_breakpoints.begin(), g_breakpoints.end());
  return v;
}

// 词法感知地扫描一行的花括号深度（跳过字符串/字符常量/注释里的括号）。
// depth 为进入本行时深度，inBlock 为跨行 /* */ 状态，两者均按引用更新。
void ScanLineBraces(const std::string &line, int &depth, bool &inBlock) {
  size_t n = line.size();
  for (size_t i = 0; i < n;) {
    if (inBlock) {
      size_t e = line.find("*/", i);
      if (e == std::string::npos)
        return; // 本行余下都在块注释里
      inBlock = false;
      i = e + 2;
      continue;
    }
    // 行注释：之后全部忽略
    if (i + 1 < n && line[i] == '/' && line[i + 1] == '/')
      return;
    // 块注释开始
    if (i + 1 < n && line[i] == '/' && line[i + 1] == '*') {
      size_t e = line.find("*/", i + 2);
      if (e == std::string::npos) {
        inBlock = true;
        return;
      }
      i = e + 2;
      continue;
    }
    // 字符串 / 字符常量：整体跳过（含转义）
    if (line[i] == '"' || line[i] == '\'') {
      char q = line[i];
      size_t j = i + 1;
      while (j < n) {
        if (line[j] == '\\' && j + 1 < n) {
          j += 2;
          continue;
        }
        if (line[j] == q) {
          j++;
          break;
        }
        j++;
      }
      i = j;
      continue;
    }
    if (line[i] == '{')
      depth++;
    else if (line[i] == '}' && depth > 0)
      depth--;
    i++;
  }
}

// 根据 g_buffer 重算可折叠块；尽量保留原有折叠（按起点行），随后重建映射
void RecalcFolds() {
  int rows = g_buffer.getRowCount();
  std::vector<int> before(rows, 0), after(rows, 0);
  int depth = 0;
  bool inBlock = false;
  for (int r = 0; r < rows; r++) {
    before[r] = depth;
    ScanLineBraces(g_buffer.getLine(r), depth, inBlock);
    after[r] = depth;
  }

  std::vector<Fold> nf;
  for (int r = 0; r < rows; r++) {
    if (after[r] <= before[r])
      continue; // 本行未开新块
    int d0 = before[r];
    for (int e = r + 1; e < rows; e++) {
      // 深度首次回到 d0 的闭合行即匹配 '}' 所在行
      if (after[e] == d0 && before[e] > d0) {
        Fold fld;
        fld.start = r;
        fld.end = e;
        nf.push_back(fld);
        break;
      }
    }
  }

  // 旧折叠中起点仍是可折叠块的才保留
  std::vector<int> kept;
  for (size_t i = 0; i < g_collapsedStarts.size(); i++) {
    int s = g_collapsedStarts[i];
    for (size_t j = 0; j < nf.size(); j++)
      if (nf[j].start == s) {
        kept.push_back(s);
        break;
      }
  }
  g_folds = nf;
  g_collapsedStarts = kept;

  // 重建视图映射
  g_docToView.assign(rows, -1);
  g_viewToDoc.clear();
  g_viewToDoc.reserve(rows);
  for (int r = 0; r < rows; r++) {
    bool hidden = false;
    for (size_t k = 0; k < g_folds.size(); k++) {
      if (std::find(g_collapsedStarts.begin(), g_collapsedStarts.end(),
                    g_folds[k].start) == g_collapsedStarts.end())
        continue;
      if (r > g_folds[k].start && r <= g_folds[k].end) {
        hidden = true;
        break;
      }
    }
    if (!hidden) {
      g_docToView[r] = (int)g_viewToDoc.size();
      g_viewToDoc.push_back(r);
    }
  }

  // 光标若落在隐藏行，移到包含它的折叠块起点
  if (rows > 0 && (g_cursor.row >= rows || g_docToView[g_cursor.row] < 0)) {
    for (size_t k = 0; k < g_folds.size(); k++) {
      const Fold &f = g_folds[k];
      if (g_cursor.row > f.start && g_cursor.row <= f.end) {
        g_cursor.row = f.start;
        int cc = g_buffer.getColCount(f.start);
        if (g_cursor.col > cc)
          g_cursor.col = cc;
        break;
      }
    }
  }
  g_foldsDirty = false;
}

// 确保折叠/映射为最新（绘制与坐标转换前调用）
void EnsureFolds() {
  if (g_foldsDirty)
    RecalcFolds();
}

// ============================================================
// 滚动辅助：可见行/列数、夹紧、保证光标可见
// ============================================================

// 编辑区可见行数（不含顶部菜单和底部状态栏所占）
int GetVisibleLineCount() {
  int top = TEXT_ORIGIN_Y;
  int bottom = WINDOW_HEIGHT - STATUS_BAR_HEIGHT;
  int n = (bottom - top) / LINE_HEIGHT;
  return n > 0 ? n : 1;
}

// 编辑区可见列数（从 TEXT_ORIGIN_X 到 EDIT_AREA_WIDTH）
int GetVisibleColCount() {
  int w = EDIT_AREA_WIDTH - TEXT_ORIGIN_X;
  int n = w / CHAR_WIDTH;
  return n > 0 ? n : 1;
}

// 夹紧滚动偏移到合法范围（防止滚过头）
void ClampScroll() {
  EnsureFolds();
  int viewCnt = (int)g_viewToDoc.size();
  int maxTop = viewCnt - GetVisibleLineCount();
  if (maxTop < 0)
    maxTop = 0;
  if (g_topLine < 0)
    g_topLine = 0;
  if (g_topLine > maxTop)
    g_topLine = maxTop;

  // 水平：以本文件最长行作为最大列
  int maxCol = 0;
  int rows = g_buffer.getRowCount();
  for (int r = 0; r < rows; r++) {
    int cc = g_buffer.getColCount(r);
    if (cc > maxCol)
      maxCol = cc;
  }
  int maxLeft = maxCol - GetVisibleColCount() + 1; // +1 让末尾能露出
  if (maxLeft < 0)
    maxLeft = 0;
  if (g_leftCol < 0)
    g_leftCol = 0;
  if (g_leftCol > maxLeft)
    g_leftCol = maxLeft;
}

// 调整滚动让光标进入可见区域（光标移动/查找后调用）
// 行：若光标视图行在 [g_topLine, g_topLine + visibleLines) 外则滚动
// 列：同理
void ExpandFoldsContaining(int row); // 前向声明（定义在折叠区）
void EnsureCursorVisible() {
  EnsureFolds();
  int vr = g_docToView[g_cursor.row];
  if (vr < 0) {
    // 光标被折叠隐藏：展开包含它的折叠块
    ExpandFoldsContaining(g_cursor.row);
    EnsureFolds();
    vr = g_docToView[g_cursor.row];
    if (vr < 0)
      return;
  }

  int visLines = GetVisibleLineCount();
  if (vr < g_topLine)
    g_topLine = vr;
  if (vr >= g_topLine + visLines)
    g_topLine = vr - visLines + 1;

  int visCols = GetVisibleColCount();
  if (g_cursor.col < g_leftCol)
    g_leftCol = g_cursor.col;
  if (g_cursor.col >= g_leftCol + visCols)
    g_leftCol = g_cursor.col - visCols + 1;

  ClampScroll();
}

// 标记文档已修改。structural=true（行增删：回车/合并/粘贴/剪切/撤销）
// 时行号会位移，折叠无法可靠跟随，全部展开；普通打字保留折叠。
void MarkFoldsDirty(bool structural) {
  if (structural)
    g_collapsedStarts.clear();
  g_foldsDirty = true;
}

bool IsFoldCollapsed(int startRow) {
  return std::find(g_collapsedStarts.begin(), g_collapsedStarts.end(),
                   startRow) != g_collapsedStarts.end();
}

// 切换某个块的折叠/展开
void ToggleFold(int startRow) {
  if (IsFoldCollapsed(startRow)) {
    g_collapsedStarts.erase(std::remove(g_collapsedStarts.begin(),
                                        g_collapsedStarts.end(), startRow),
                            g_collapsedStarts.end());
  } else {
    g_collapsedStarts.push_back(startRow);
  }
  g_foldsDirty = true;
  g_foldsNeedRedraw = true;
  EnsureFolds();
}

// 展开所有包含指定文档行的折叠块（用于诊断跳转）
void ExpandFoldsContaining(int docRow) {
  bool changed = false;
  for (size_t k = 0; k < g_folds.size(); k++) {
    const Fold &f = g_folds[k];
    if (docRow > f.start && docRow <= f.end && IsFoldCollapsed(f.start)) {
      g_collapsedStarts.erase(std::remove(g_collapsedStarts.begin(),
                                          g_collapsedStarts.end(), f.start),
                              g_collapsedStarts.end());
      changed = true;
    }
  }
  if (changed) {
    g_foldsDirty = true;
    EnsureFolds();
  }
}

// 折叠所有可折叠块
void FoldAll() {
  EnsureFolds();
  g_collapsedStarts.clear();
  for (size_t k = 0; k < g_folds.size(); k++)
    g_collapsedStarts.push_back(g_folds[k].start);
  g_foldsDirty = true;
  g_foldsNeedRedraw = true;
  EnsureFolds();
}

// 展开所有折叠块
void ExpandAll() {
  g_collapsedStarts.clear();
  g_foldsDirty = true;
  g_foldsNeedRedraw = true;
  EnsureFolds();
}

// ===== 右键上下文菜单（仿 Dev-C++）=====
struct ContextMenuItem {
  const char *label;
  int action; // 0=分隔线, 1=剪切, 2=复制, 3=粘贴, 4=删除, 5=全选, 6=查找,
              // 7=撤销, 8=重做
  bool enabled;
};
ContextMenuItem g_ctxItems[] = {
    {"撤销", 7, false},     {"重做", 8, false}, {"", 0, false}, // 分隔线
    {"剪切", 1, true},      {"复制", 2, true},  {"粘贴", 3, true},
    {"删除", 4, true},      {"", 0, false}, // 分隔线
    {"全选", 5, true},      {"", 0, false}, // 分隔线
    {"查找与替换", 6, true}};
const int g_ctxItemCount = sizeof(g_ctxItems) / sizeof(g_ctxItems[0]);
const int g_ctxMenuW = 130;
const int g_ctxMenuItemH = 22;

bool g_ctxMenuOpen = false;
int g_ctxMenuX = 0;
int g_ctxMenuY = 0;
int g_ctxMenuHoverIdx = -1; // 鼠标悬停的项索引（-1 表示无）

// ===== 撤销 / 重做（文档快照双栈）=====
// 对课程作业规模的文件，每次编辑前保存"全文 + 光标位置"快照最简单可靠，
// 无需为插入/删除/换行分别设计反向操作。
struct DocSnapshot {
  std::string text; // 该时刻的完整文本
  int row, col;     // 该时刻的光标位置
};
std::vector<DocSnapshot> g_undoStack; // 撤销栈：栈顶 = 最近一次编辑前
std::vector<DocSnapshot> g_redoStack; // 重做栈
const size_t MAX_UNDO_STEPS = 100;    // 最多保留 100 步
const DWORD TYPING_MERGE_MS = 800;    // 连续打字/删除间隔 < 800ms 合并为一步
bool g_undoTyping = false;            // 上一个入栈动作是否"连续输入类"
DWORD g_undoLastTick = 0;             // 上一个入栈动作的时刻

// 清空撤销/重做历史（新建/打开文件后调用）
void UndoReset() {
  g_undoStack.clear();
  g_redoStack.clear();
  g_undoTyping = false;
  g_undoLastTick = 0;
}

// 在任何修改文本的动作之前调用：
//  typing=true：连续输入类（打字、长按退格/删除），短间隔内合并为一步；
//  typing=false：离散动作（回车、粘贴、剪切、菜单删除），总是独立一步。
// 新动作发生时清空 redo 栈，避免历史分叉。
void UndoBeginAction(bool typing) {
  DWORD now = GetTickCount();
  bool merge =
      typing && g_undoTyping && (now - g_undoLastTick < TYPING_MERGE_MS);
  if (!merge) {
    std::string cur = g_buffer.getText();
    // 栈顶快照与当前文本相同（常见于撤销/重做后立刻继续编辑）：
    // 不入冗余快照，但仍清空 redo，保证新编辑产生正确的历史分叉
    if (g_undoStack.empty() || g_undoStack.back().text != cur) {
      DocSnapshot s;
      s.text = cur;
      s.row = g_cursor.row;
      s.col = g_cursor.col;
      g_undoStack.push_back(s);
      if (g_undoStack.size() > MAX_UNDO_STEPS) {
        g_undoStack.erase(g_undoStack.begin());
      }
    }
    g_redoStack.clear();
  }
  g_undoTyping = typing;
  g_undoLastTick = now;
}

// 撤销：当前状态压入 redo，恢复 undo 栈顶快照。返回 true 表示执行了撤销
bool EditUndo() {
  if (g_undoStack.empty())
    return false;

  DocSnapshot cur;
  cur.text = g_buffer.getText();
  cur.row = g_cursor.row;
  cur.col = g_cursor.col;
  g_redoStack.push_back(cur);

  DocSnapshot s = g_undoStack.back();
  g_undoStack.pop_back();
  g_buffer.setText(s.text);
  g_cursor.row = s.row;
  g_cursor.col = s.col;
  g_sel.active = false;
  g_modified = true;
  g_undoTyping = false; // 撤销后打断连续输入合并
  MarkFoldsDirty(true); // 全文恢复可能改变行数，统一展开重算
  return true;
}

// 重做：当前状态压入 undo，恢复 redo 栈顶快照。返回 true 表示执行了重做
bool EditRedo() {
  if (g_redoStack.empty())
    return false;

  DocSnapshot cur;
  cur.text = g_buffer.getText();
  cur.row = g_cursor.row;
  cur.col = g_cursor.col;
  g_undoStack.push_back(cur);
  if (g_undoStack.size() > MAX_UNDO_STEPS) {
    g_undoStack.erase(g_undoStack.begin());
  }

  DocSnapshot s = g_redoStack.back();
  g_redoStack.pop_back();
  g_buffer.setText(s.text);
  g_cursor.row = s.row;
  g_cursor.col = s.col;
  g_sel.active = false;
  g_modified = true;
  g_undoTyping = false;
  MarkFoldsDirty(true); // 全文恢复可能改变行数，统一展开重算
  return true;
}

// 排序选区，保证 start 在 end 之前
void NormalizeSel() {
  if (!g_sel.active)
    return;
  // 比较 (startRow, startCol) 和 (endRow, endCol)
  bool swap = false;
  if (g_sel.startRow > g_sel.endRow)
    swap = true;
  else if (g_sel.startRow == g_sel.endRow && g_sel.startCol > g_sel.endCol)
    swap = true;
  if (swap) {
    int tr = g_sel.startRow, tc = g_sel.startCol;
    g_sel.startRow = g_sel.endRow;
    g_sel.startCol = g_sel.endCol;
    g_sel.endRow = tr;
    g_sel.endCol = tc;
  }
}

// 获取选区文本（含跨行）
std::string GetSelText() {
  if (!g_sel.active)
    return "";
  NormalizeSel();
  std::string result;
  int r = g_sel.startRow, c = g_sel.startCol;
  while (r < g_sel.endRow || (r == g_sel.endRow && c < g_sel.endCol)) {
    int cols = g_buffer.getColCount(r);
    if (c < cols) {
      result += g_buffer.getChar(r, c);
      c++;
    } else {
      // 行尾：换行
      result += '\n';
      r++;
      c = 0;
    }
  }
  return result;
}

// 删除选区内容（光标定位到 start）
void DeleteSelContent() {
  if (!g_sel.active)
    return;
  NormalizeSel();
  int r = g_sel.startRow, c = g_sel.startCol;
  while (r < g_sel.endRow || (r == g_sel.endRow && c < g_sel.endCol)) {
    int cols = g_buffer.getColCount(r);
    if (c < cols) {
      g_buffer.deleteChar(r, c);
      // 删除后列号不变（字符前移），但 endCol 要 -1
      if (r == g_sel.endRow)
        g_sel.endCol--;
    } else {
      // 行尾：删除换行（合并到下一行）
      if (r < g_buffer.getRowCount() - 1) {
        int nextLen = g_buffer.getColCount(r + 1);
        for (int i = 0; i < nextLen; i++) {
          g_buffer.insertChar(r, c + i, g_buffer.getChar(r + 1, i));
        }
        g_buffer.deleteLine(r + 1);
        if (r == g_sel.endRow) {
          g_sel.endCol = c + nextLen; // 同行，endCol 增加合并的字符数
          g_sel.endRow = r;
        }
      } else {
        break; // 最后一行，无法合并
      }
    }
  }
  g_cursor.row = g_sel.startRow;
  g_cursor.col = g_sel.startCol;
}

// ===== Win32 剪贴板操作 =====
// 复制字符串到剪贴板
bool CopyToClipboard(const std::string &text) {
  if (!OpenClipboard(GetHWnd()))
    return false;
  EmptyClipboard();
  HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
  if (!hMem) {
    CloseClipboard();
    return false;
  }
  char *p = (char *)GlobalLock(hMem);
  if (p) {
    memcpy(p, text.c_str(), text.size());
    p[text.size()] = '\0';
    GlobalUnlock(hMem);
    SetClipboardData(CF_TEXT, hMem);
  }
  CloseClipboard();
  return true;
}

// 从剪贴板读取字符串（优先 CF_UNICODETEXT 并按 GBK 转换，
// 兼容各来源；失败再退回 CF_TEXT）
std::string PasteFromClipboard() {
  std::string result;
  if (!OpenClipboard(GetHWnd()))
    return result;
  HANDLE hMem = GetClipboardData(CF_UNICODETEXT);
  if (hMem) {
    wchar_t *p = (wchar_t *)GlobalLock(hMem);
    if (p) {
      int need = WideCharToMultiByte(CP_ACP, 0, p, -1, NULL, 0, NULL, NULL);
      if (need > 1) {
        std::vector<char> tmp(need);
        WideCharToMultiByte(CP_ACP, 0, p, -1, &tmp[0], need, NULL, NULL);
        result.assign(&tmp[0]); // assign 在首个 \0 处截断
      }
      GlobalUnlock(hMem);
    }
  }
  if (result.empty()) {
    HANDLE hMem2 = GetClipboardData(CF_TEXT);
    if (hMem2) {
      char *p = (char *)GlobalLock(hMem2);
      if (p) {
        result = p;
        GlobalUnlock(hMem2);
      }
    }
  }
  CloseClipboard();
  return result;
}

// 整数转字符串辅助函数
std::string IntToStr(int n) {
  char buf[32];
  sprintf(buf, "%d", n);
  return std::string(buf);
}

// 文本输出辅助函数（兼容 Unicode/多字节字符集）
void OutText(int x, int y, const std::string &s) {
#ifdef UNICODE
  int len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, NULL, 0);
  wchar_t *buf = new wchar_t[len];
  MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, buf, len);
  outtextxy(x, y, buf);
  delete[] buf;
#else
  outtextxy(x, y, s.c_str());
#endif
}

// ============================================================
// 菜单数据结构
// ============================================================
struct MenuItem {
  const char *name;
  const char **subItems;
  int subCount;
  int x;
  int width;
};

static const char *g_fileItems[] = {"新建", "打开", "保存", "另存为", "退出"};
static const char *g_editItems[] = {
    "撤销",      "重做",        "剪切",     "复制",     "粘贴",    "查找与替换",
    "转到行...", "转到函数...", "全部折叠", "全部展开", "切换主题"};
static const char *g_buildItems[] = {"语法检测",     "编译",   "运行",
                                     "编译并运行",   "调试运行 (F5)",
                                     "诊断配置..."};
static const char *g_helpItems[] = {"使用说明", "关于"};

MenuItem g_menus[] = {{"文件", g_fileItems, 5, 0, 70},
                      {"编辑", g_editItems, 11, 0, 70},
                      {"运行", g_buildItems, 6, 0, 70},
                      {"帮助", g_helpItems, 2, 0, 70}};
const int g_menuCount = sizeof(g_menus) / sizeof(g_menus[0]);
int g_activeMenu = -1;
int g_menuBarHover = -1; // 顶部菜单栏悬停项索引（-1 表示无）
int g_subMenuHover = -1; // 下拉子菜单悬停项索引（-1 表示无）

// 状态栏"文件:"区悬停 → 文件完整路径预览气泡
bool g_statusFileHover = false;       // 鼠标是否悬停在状态栏文件名区
DWORD g_statusHoverTick = 0;          // 进入悬停的时刻（GetTickCount）
const int STATUS_FILE_REGION_W = 440; // 文件名信息区的右边界（"行:"在 450）

// ============================================================
// 全屏 / 拉伸：状态与窗口子类化
// ============================================================
// 全屏切换：保存窗口ed 进入全屏前的窗口位置/尺寸与样式，
// 切换为无边框全屏；再次触发恢复
bool g_fullScreen = false;
RECT g_savedWindowRect = {0}; // 还原用
LONG g_savedStyle = 0;
WNDPROC g_oldWndProc = NULL; // EasyX 原始窗口过程
bool g_layoutDirty = false;  // 由 subclass 设置，主循环消费
volatile bool g_closeRequested =
    false;                   // 收到 WM_CLOSE（点 X / Alt+F4），主循环消费
bool g_inSizeMove = false;   // 是否正处于边框拉伸/移动的模态过程
bool g_sizeChanged = false;  // 本轮拉伸中客户区尺寸是否变化
int g_pendingW = 0;          // WM_SIZE 记录的待应用客户区宽
int g_pendingH = 0;          // WM_SIZE 记录的待应用客户区高
int g_targetW = 0;           // 松手时捕获的最终目标客户区宽
int g_targetH = 0;           // 松手时捕获的最终目标客户区高
DWORD g_ignoreSizeUntil = 0; // 此前忽略非拖拽 WM_SIZE（防 Resize 自产消息回灌）

// 计算窗口客户区尺寸对应的窗口外框尺寸
void ClientToWindowSize(int clientW, int clientH, int &winW, int &winH) {
  RECT rc = {0, 0, clientW, clientH};
  DWORD style = (DWORD)GetWindowLongPtr(GetHWnd(), GWL_STYLE);
  DWORD exStyle = (DWORD)GetWindowLongPtr(GetHWnd(), GWL_EXSTYLE);
  BOOL hasMenu = GetMenu(GetHWnd()) != NULL; // EasyX 无菜单
  AdjustWindowRectEx(&rc, style, hasMenu, exStyle);
  winW = rc.right - rc.left;
  winH = rc.bottom - rc.top;
}

// 切换全屏（F11 触发）
void ToggleFullScreen() {
  HWND hWnd = GetHWnd();
  if (!g_fullScreen) {
    // 进入全屏：保存当前窗口样式 + 外框位置
    g_savedStyle = GetWindowLongPtr(hWnd, GWL_STYLE);
    GetWindowRect(hWnd, &g_savedWindowRect);
    LONG fs = WS_POPUP | WS_SYSMENU | WS_VISIBLE;
    SetWindowLongPtr(hWnd, GWL_STYLE, fs);
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    SetWindowPos(hWnd, HWND_TOP, 0, 0, sw, sh,
                 SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    g_fullScreen = true;
  } else {
    // 退出全屏：恢复样式与外框位置
    SetWindowLongPtr(hWnd, GWL_STYLE, g_savedStyle);
    SetWindowPos(hWnd, HWND_NOTOPMOST, g_savedWindowRect.left,
                 g_savedWindowRect.top,
                 g_savedWindowRect.right - g_savedWindowRect.left,
                 g_savedWindowRect.bottom - g_savedWindowRect.top,
                 SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    g_fullScreen = false;
  }
  // 进入/退出全屏后，客户区尺寸改变 → 由 subclass 的 WM_SIZE 同步布局
  // 也立即主动同步一次（防止某些情况下 WM_SIZE 不触发）
  RECT cr;
  GetClientRect(hWnd, &cr);
  if (cr.right > 0 && cr.bottom > 0) {
    UpdateLayout(cr.right, cr.bottom);
    g_ignoreSizeUntil = GetTickCount() + 250; // 忽略 Resize 自产 WM_SIZE
    Resize(NULL, cr.right, cr.bottom);        // 同步绘图缓冲，避免黑屏
    g_pendingW = cr.right;
    g_pendingH = cr.bottom;
    g_layoutDirty = true;
  }
}

// 拉伸模态中：直接在屏幕 DC 上画最终布局的色块（纯 GDI FillRect，
// 每次仅 5 个矩形 + 4 条分隔线，开销可忽略），避免触碰 EasyX 帧缓冲。
// 色块颜色与 DrawEditArea/DrawMenuBar/DrawDiagPanel/DrawStatusBar 一致，
// 松手渲染完整内容时视觉无缝衔接。
void PaintSizeMoveBands(HDC hdc, int w, int h) {
  // 画刷随主题惰性重建（主题索引变化才重建）
  static int lastTheme = -1;
  static HBRUSH brMenu = NULL;
  static HBRUSH brEdit = NULL;
  static HBRUSH brLineN = NULL;
  static HBRUSH brDiag = NULL;
  static HBRUSH brStatus = NULL;
  static HBRUSH brSep1 = NULL;
  static HBRUSH brSep2 = NULL;
  static HBRUSH brSep3 = NULL;
  if (lastTheme != g_themeIdx) {
    if (brMenu)
      DeleteObject(brMenu);
    if (brEdit)
      DeleteObject(brEdit);
    if (brLineN)
      DeleteObject(brLineN);
    if (brDiag)
      DeleteObject(brDiag);
    if (brStatus)
      DeleteObject(brStatus);
    if (brSep1)
      DeleteObject(brSep1);
    if (brSep2)
      DeleteObject(brSep2);
    if (brSep3)
      DeleteObject(brSep3);
    brMenu = CreateSolidBrush(g_theme.menuBarBg);
    brEdit = CreateSolidBrush(g_theme.editBg);
    brLineN = CreateSolidBrush(g_theme.lineNumBg);
    brDiag = CreateSolidBrush(g_theme.diagBg);
    brStatus = CreateSolidBrush(g_theme.statusBg);
    brSep1 = CreateSolidBrush(g_theme.menuBorder);
    brSep2 = CreateSolidBrush(g_theme.lineNumSep);
    brSep3 = CreateSolidBrush(g_theme.diagBorder);
    lastTheme = g_themeIdx;
  }

  RECT rc;
  // 整窗白底
  SetRect(&rc, 0, 0, w, h);
  FillRect(hdc, &rc, brEdit);
  // 顶部菜单栏
  SetRect(&rc, 0, 0, w, MENU_BAR_HEIGHT);
  FillRect(hdc, &rc, brMenu);
  // 行号栏
  SetRect(&rc, 0, MENU_BAR_HEIGHT, LINE_NUM_WIDTH, h - STATUS_BAR_HEIGHT);
  FillRect(hdc, &rc, brLineN);
  // 右侧诊断面板
  SetRect(&rc, w - DIAG_PANEL_WIDTH, MENU_BAR_HEIGHT, w, h - STATUS_BAR_HEIGHT);
  FillRect(hdc, &rc, brDiag);
  // 底部状态栏
  SetRect(&rc, 0, h - STATUS_BAR_HEIGHT, w, h);
  FillRect(hdc, &rc, brStatus);

  // 分隔线（1px）
  SetRect(&rc, 0, MENU_BAR_HEIGHT, w, MENU_BAR_HEIGHT + 1);
  FillRect(hdc, &rc, brSep1);
  SetRect(&rc, LINE_NUM_WIDTH, MENU_BAR_HEIGHT, LINE_NUM_WIDTH + 1,
          h - STATUS_BAR_HEIGHT);
  FillRect(hdc, &rc, brSep2);
  SetRect(&rc, w - DIAG_PANEL_WIDTH, MENU_BAR_HEIGHT, w - DIAG_PANEL_WIDTH + 1,
          h - STATUS_BAR_HEIGHT);
  FillRect(hdc, &rc, brSep3);
  SetRect(&rc, 0, h - STATUS_BAR_HEIGHT, w, h - STATUS_BAR_HEIGHT + 1);
  FillRect(hdc, &rc, brSep2);
}

// 子类化窗口过程：
//  - 拉伸模态中：WM_PAINT 直接画色块，WM_ERASEBKGND 吞掉防闪烁；
//    不调用任何 EasyX 图形函数
//  - 退出模态时：在本线程（与窗口绘制同线程）做唯一一次 Resize，
//    此时主线程处于静默状态，无跨线程竞争
LRESULT CALLBACK SubclassedWndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
  // 进入边框拉伸 / 标题栏移动 / 最大化的模态过程
  case WM_ENTERSIZEMOVE:
    g_inSizeMove = true;
    g_sizeChanged = false;
    break;
  case WM_SIZE: {
    // 最小化时客户区尺寸无意义，跳过
    if (wp == SIZE_MINIMIZED)
      break;
    int w = LOWORD(lp);
    int h = HIWORD(lp);
    // 主线程正在 Resize+校正：这些 WM_SIZE 是 Resize 自身产生的，
    // 全部忽略，避免被当成新目标回灌导致逐帧漂移/塌缩
    if (!g_inSizeMove && GetTickCount() < g_ignoreSizeUntil)
      break;
    if (w > 0 && h > 0 && (w != WINDOW_WIDTH || h != WINDOW_HEIGHT)) {
      g_pendingW = w;
      g_pendingH = h;
      g_sizeChanged = true;
      // 非拉伸模态（外部程序触发的尺寸变化）：立即通知主循环
      if (!g_inSizeMove)
        g_layoutDirty = true;
    }
    break;
  }
  // 拉伸中（以及松手后到主线程完成重绘前的短暂间隙）：
  // 色块布局，不交给 EasyX（否则它会贴旧帧）
  case WM_PAINT:
    if (g_inSizeMove || g_layoutDirty) {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hWnd, &ps);
      int w = g_pendingW, h = g_pendingH;
      if (w <= 0 || h <= 0) {
        RECT cr;
        GetClientRect(hWnd, &cr);
        w = cr.right;
        h = cr.bottom;
      }
      PaintSizeMoveBands(hdc, w, h);
      EndPaint(hWnd, &ps);
      return 0;
    }
    break;
  // 上述期间禁止背景擦除（色块已覆盖全窗，擦除只会闪烁）
  case WM_ERASEBKGND:
    if (g_inSizeMove || g_layoutDirty)
      return 1;
    break;
  // 模态结束：只清状态、置脏。绝不在此消息内调用 Resize/
  // SetWindowPos —— 模态循环尚未完全退出，内部若有窗口操作
  // 会被重入解释，可能导致窗口回弹缩小。Resize 交由主线程在
  // 模态完全退出后的下一帧执行。
  case WM_EXITSIZEMOVE:
    // 捕获松手瞬间的客户区尺寸 = 用户最终意图
    {
      RECT c;
      GetClientRect(hWnd, &c);
      g_targetW = c.right;
      g_targetH = c.bottom;
    }
    g_sizeChanged = false;
    g_inSizeMove = false;
    g_layoutDirty = true;
    break;
  case WM_GETMINMAXINFO: {
    // 拉伸时限制最小客户区尺寸
    MINMAXINFO *mmi = (MINMAXINFO *)lp;
    int winW = 0, winH = 0;
    ClientToWindowSize(MIN_WIDTH, MIN_HEIGHT, winW, winH);
    mmi->ptMinTrackSize.x = winW;
    mmi->ptMinTrackSize.y = winH;
    break;
  }
  // 点窗口右上角 X / 按 Alt+F4：不直接关闭（否则未保存代码丢失）。
  // 这里运行在 EasyX 内部窗口线程，不能弹模态对话框，因此只置标志，
  // 由主线程下一帧弹"是/否/取消"；返回 0 阻止默认的 DestroyWindow。
  case WM_CLOSE:
    g_closeRequested = true;
    return 0;

  // 鼠标滚轮：累计 delta 到全局，由主线程消费转换为行/列滚动。
  // 这里运行在 EasyX 内部线程，不能直接修改 g_topLine（会与主线程
  // 的 ClampScroll/EnsureCursorVisible 竞争）；只累加，主线程统一处理。
  // Shift+滚轮 → 水平滚动（写到 g_hWheelPending）
  case WM_MOUSEWHEEL: {
    short delta = (short)HIWORD(wp);
    short zflags = (short)LOWORD(wp);
    bool shift = (zflags & MK_SHIFT) != 0;
    if (shift)
      g_hWheelPending += delta;
    else
      g_wheelPending += delta;
    return 0; // 不让 EasyX 默认处理（默认会滚动焦点控件，无意义）
  }
  }
  return CallWindowProc(g_oldWndProc, hWnd, msg, wp, lp);
}

// ============================================================
// 判断某行（0-based）是否为编译错误行
// 遍历诊断列表，若有 diag.line == row+1 则为错误行
// ============================================================
bool IsErrorLine(int row) {
  if (g_diagSource == 1)
    return false; // 静态诊断只画波浪线，不整行标红
  if (g_buildOk)
    return false; // 编译成功则无错误行
  for (int i = 0; i < g_diagCount; i++) {
    if (g_diagList[i].type == "error" && g_diagList[i].line == row + 1)
      return true;
  }
  return false;
}

// ============================================================
// 逐行词法着色：将单行按 token 类型分段，用不同颜色绘制
// 支持：关键字 / 数字 / 字符串 / 字符常量 / 行注释 / 块注释 / 预处理指令
// 参数 inBlock 同时用于跨行块注释状态推进；不可见行只推进状态不绘制
// ============================================================

// 词法着色用配色随主题变化（见
// g_theme.keyword/number/string/comment/preproc/selBg）

// 输出一个词法段 seg（已确定字色 fg，起始于本行字节列 col0，屏幕 x）。
// 与本行选区列区间 [selC1, selC2)（字节列，半开）求交：
// 落在选区内的片段用选区蓝底，其余用正常底色 normalBg；字色（语法高亮）
// 在两种背景下都保持不变。这样只高亮真正选中的字符，且不丢失词法着色。
//
// 水平滚动裁剪：实测本机 EasyX 的 setcliprgn 一旦调用会永久破坏后续所有
// 文字输出（即使 clearcliprgn 之后也不恢复），因此不能用 GDI 裁剪区。
// 改为在这里按可见列窗口 [g_leftCol, g_leftCol+visCols) 手工切子串：
// 完全在窗外不输出，跨边界只输出窗内部分，防止左滚时文字盖住行号栏
// （右溢出由后绘制的诊断面板/滚动条轨道覆盖）。
// GBK/CP936 双字节前导字节判定（0x81-0xFE 开头，后随一个尾字节）
static inline bool IsGbkLeadByte(unsigned char b) {
  return b >= 0x81 && b <= 0xFE;
}

// seg 的起点必须是字符边界（词法器对此有保证）。判定 idx 处字节是否
// 为一个 GBK 双字节字符的首字节。注意尾字节范围（0x40-0xFE，除 0x7F）
// 与首字节范围有重叠，不能只看单个字节，必须从段首按配对状态扫描。
static bool SegByteIsPairLead(const std::string &seg, int idx) {
  int n = (int)seg.size();
  if (idx < 0 || idx + 1 >= n)
    return false; // 首字节后面必须还有尾字节
  int k = 0;
  while (k < idx) {
    if (IsGbkLeadByte((unsigned char)seg[k]) && k + 1 < n)
      k += 2;
    else
      k += 1;
  }
  return k == idx && IsGbkLeadByte((unsigned char)seg[idx]);
}

// 段尾是否挂着一个没有尾字节配对的孤空前导字节（畸形数据）
static bool SegEndsWithLoneLead(const std::string &seg) {
  int n = (int)seg.size();
  if (n <= 0 || !IsGbkLeadByte((unsigned char)seg[n - 1]))
    return false;
  return !SegByteIsPairLead(seg, n - 1);
}

void PaintSeg(int x, int y, const std::string &seg, COLORREF fg,
              COLORREF normalBg, int col0, int selC1, int selC2) {
  int len = (int)seg.size();

  // —— 水平可见窗口求交（字节列，半开区间）——
  int vL = g_leftCol;
  int vR = g_leftCol + GetVisibleColCount();
  int lo = 0;   // 段内可见起始下标
  int hi = len; // 段内可见结束下标（不含）
  if (col0 + len <= vL || col0 >= vR)
    return; // 整段在窗外
  if (col0 < vL)
    lo = vL - col0;
  if (col0 + len > vR)
    hi = vR - col0;

  // —— 裁剪边界必须对齐 GBK 双字节字符边界 ——
  // 词法器保证 seg 起点 col0 是字符边界，段内字节配对确定。
  // 硬切半个汉字交给 GDI 输出会产生乱码字形，且残缺字节的文字推进
  // 宽度不等于 CHAR_WIDTH，会把本行后续字符整体挤歪（水平滚动时的
  // "压缩乱码"即由此而来）。
  // 左缘切在某汉字中间（lo-1 是首字节）：整个汉字跳过不画，它已有
  //       一半在窗外，缺的格由预先铺好的行底色填充，不留残字；
  if (lo > 0 && SegByteIsPairLead(seg, lo - 1))
    ++lo;
  // 右缘只切到某汉字的首字节：把整字纳入，多出的最多 8px 由后绘制
  //       的垂直滚动条轨道覆盖，不会侵入诊断面板；
  if (hi < len && SegByteIsPairLead(seg, hi - 1))
    ++hi;
  // 畸形数据（行尾孤立的前导字节）：丢弃，避免输出残缺字形。
  if (hi == len && SegEndsWithLoneLead(seg))
    --hi;
  if (lo >= hi)
    return;

  int xOff = x + lo * CHAR_WIDTH; // 可见部分的屏幕起始 x

  bool hasSel = (selC2 > selC1) && (col0 + hi > selC1) && (col0 + lo < selC2);
  settextcolor(fg);
  if (!hasSel) {
    setbkcolor(normalBg);
    OutText(xOff, y, seg.substr(lo, hi - lo));
    return;
  }
  // 按选区边界把可见部分切成若干同底色子串，逐片输出（x 按字节宽推进）
  int idx = lo, cx = xOff;
  while (idx < hi) {
    int absCol = col0 + idx;
    bool inSel = (absCol >= selC1 && absCol < selC2);
    int j = idx;
    while (j < hi) {
      int ac = col0 + j;
      bool js = (ac >= selC1 && ac < selC2);
      if (js != inSel) {
        // 选区边界恰好落在一个汉字中间（边界为字节列，可能为奇数）：
        // 整个汉字跟随其首字节的选中状态，尾字节一并纳入本段，
        // 保证 substr 永不切在双字节字符中间。
        if (j > idx && SegByteIsPairLead(seg, j - 1)) {
          j++;
          continue;
        }
        break;
      }
      j++;
    }
    std::string sub = seg.substr(idx, j - idx);
    if (inSel) {
      // 关键修复：程序全局 setbkmode(TRANSPARENT)，outtextxy 不再绘制
      // 文字背景，setbkcolor 对选区底色完全无效。底色必须用 bar() 铺出。
      setfillcolor(g_theme.selBg);
      bar(cx, y - 2, cx + (j - idx) * CHAR_WIDTH, y + LINE_HEIGHT - 2);
      settextcolor(g_theme.selText); // 选中→白字，语法色让位
    } else {
      settextcolor(fg);
    }
    OutText(cx, y, sub);
    cx += (j - idx) * CHAR_WIDTH;
    idx = j;
  }
  setbkcolor(normalBg);
}

// C89/C99 关键字集合
bool IsCKeyword(const std::string &s) {
  static const char *kw[] = {
      "auto",       "break",    "case",     "char",   "const",   "continue",
      "default",    "do",       "double",   "else",   "enum",    "extern",
      "float",      "for",      "goto",     "if",     "inline",  "int",
      "long",       "register", "restrict", "return", "short",   "signed",
      "sizeof",     "static",   "struct",   "switch", "typedef", "union",
      "unsigned",   "void",     "volatile", "while",  "_Bool",   "_Complex",
      "_Imaginary", NULL};
  for (int i = 0; kw[i] != NULL; i++) {
    if (s == kw[i])
      return true;
  }
  return false;
}

// 对一行文本做词法分析并分段着色输出
// visible=false 时仅更新 inBlock 状态（用于不可见行），不调用 OutText
// selC1/selC2：本行被选中的字节列区间（半开）；selC2<=selC1 表示本行无选中
void DrawLineSyntaxColored(int x, int y, const std::string &line, bool &inBlock,
                           COLORREF bg, bool visible, int selC1 = -1,
                           int selC2 = -1) {
  size_t i = 0;
  size_t n = line.size();
  if (visible)
    setbkcolor(bg);

  while (i < n) {
    // —— 处于块注释中：寻找 */ 结束 ——
    if (inBlock) {
      size_t end = line.find("*/", i);
      std::string seg = (end == std::string::npos)
                            ? line.substr(i)
                            : line.substr(i, end + 2 - i);
      if (visible)
        PaintSeg(x, y, seg, g_theme.comment, bg, (int)i, selC1, selC2);
      x += (int)seg.size() * CHAR_WIDTH;
      if (end == std::string::npos)
        return; // 整行余下都是块注释
      i = end + 2;
      inBlock = false;
      continue;
    }

    // —— 行注释 //... 到行尾 ——
    if (i + 1 < n && line[i] == '/' && line[i + 1] == '/') {
      std::string seg = line.substr(i);
      if (visible)
        PaintSeg(x, y, seg, g_theme.comment, bg, (int)i, selC1, selC2);
      return;
    }

    // —— 块注释 /* ... */ 开始（可能跨行） ——
    if (i + 1 < n && line[i] == '/' && line[i + 1] == '*') {
      size_t end = line.find("*/", i + 2);
      std::string seg2 = (end == std::string::npos)
                             ? line.substr(i)
                             : line.substr(i, end + 2 - i);
      if (visible)
        PaintSeg(x, y, seg2, g_theme.comment, bg, (int)i, selC1, selC2);
      x += (int)seg2.size() * CHAR_WIDTH;
      if (end == std::string::npos) {
        inBlock = true;
        return;
      }
      i = end + 2;
      continue;
    }

    // —— 字符串 "..."（支持转义） ——
    if (line[i] == '"') {
      size_t j = i + 1;
      while (j < n) {
        if (line[j] == '\\' && j + 1 < n) {
          j += 2;
          continue;
        }
        if (line[j] == '"') {
          j++;
          break;
        }
        j++;
      }
      std::string seg = line.substr(i, j - i);
      if (visible)
        PaintSeg(x, y, seg, g_theme.string, bg, (int)i, selC1, selC2);
      x += (int)seg.size() * CHAR_WIDTH;
      i = j;
      continue;
    }

    // —— 字符常量 '...'（支持转义） ——
    if (line[i] == '\'') {
      size_t j = i + 1;
      while (j < n) {
        if (line[j] == '\\' && j + 1 < n) {
          j += 2;
          continue;
        }
        if (line[j] == '\'') {
          j++;
          break;
        }
        j++;
      }
      std::string seg = line.substr(i, j - i);
      if (visible)
        PaintSeg(x, y, seg, g_theme.string, bg, (int)i, selC1, selC2);
      x += (int)seg.size() * CHAR_WIDTH;
      i = j;
      continue;
    }

    // —— GBK 双字节中文字符（高位为 1 的字节，2 字节整体输出） ——
    // 避免被 isalpha 等误判，同时保证中文按 2 个 ASCII 宽度推进 x
    if (((unsigned char)line[i]) & 0x80) {
      size_t len = (i + 1 < n) ? 2 : 1;
      std::string seg = line.substr(i, len);
      if (visible)
        PaintSeg(x, y, seg, g_theme.editText, bg, (int)i, selC1, selC2);
      x += (int)len * CHAR_WIDTH;
      i += len;
      continue;
    }

    // —— 预处理指令：从 '#' 到行尾整体绿色（如 #include/#define） ——
    if (line[i] == '#') {
      std::string seg = line.substr(i);
      if (visible)
        PaintSeg(x, y, seg, g_theme.preproc, bg, (int)i, selC1, selC2);
      return;
    }

    // —— 数字字面量 ——
    if (isdigit((unsigned char)line[i])) {
      size_t j = i;
      while (j < n && (isalnum((unsigned char)line[j]) || line[j] == '.'))
        j++;
      std::string seg = line.substr(i, j - i);
      if (visible)
        PaintSeg(x, y, seg, g_theme.number, bg, (int)i, selC1, selC2);
      x += (int)seg.size() * CHAR_WIDTH;
      i = j;
      continue;
    }

    // —— 标识符 / 关键字 ——
    if (isalpha((unsigned char)line[i]) || line[i] == '_') {
      size_t j = i;
      while (j < n && (isalnum((unsigned char)line[j]) || line[j] == '_'))
        j++;
      std::string seg = line.substr(i, j - i);
      COLORREF kwCol = IsCKeyword(seg) ? g_theme.keyword : g_theme.editText;
      if (visible)
        PaintSeg(x, y, seg, kwCol, bg, (int)i, selC1, selC2);
      x += (int)seg.size() * CHAR_WIDTH;
      i = j;
      continue;
    }

    // —— 其他单字符（标点、空白、操作符） ——
    {
      std::string seg(1, line[i]);
      if (visible)
        PaintSeg(x, y, seg, g_theme.editText, bg, (int)i, selC1, selC2);
      x += CHAR_WIDTH;
      i++;
    }
  }
}

// ============================================================
// 绘制：行号栏右侧 gutter 的折叠标记（可折叠块起点显示 [-]/[+]）
// 标记位于 x ∈ [LINE_NUM_WIDTH, TEXT_ORIGIN_X) 的窄条内
// ============================================================
void DrawFoldGutter() {
  EnsureFolds();
  int bottom = WINDOW_HEIGHT - STATUS_BAR_HEIGHT;
  for (size_t k = 0; k < g_folds.size(); k++) {
    int sr = g_folds[k].start;
    int vr = g_docToView[sr];
    if (vr < 0)
      continue; // 起点被外层折叠
    int y = TEXT_ORIGIN_Y + vr * LINE_HEIGHT;
    if (y > bottom - LINE_HEIGHT)
      continue;

    int bx = LINE_NUM_WIDTH + 1; // 41
    int by = y + 4;
    int br = TEXT_ORIGIN_X - 2; // 48
    bool collapsed = IsFoldCollapsed(sr);

    setfillcolor(g_theme.gutterBoxBg);
    setlinecolor(g_theme.gutterBoxLine);
    bar(bx, by, br, by + (br - bx));
    rectangle(bx, by, br, by + (br - bx));

    // + / - 符号
    settextstyle(11, 0, _T("宋体"));
    setbkcolor(g_theme.gutterBoxBg);
    settextcolor(g_theme.gutterBoxText);
    OutText(bx + 2, by - 2, collapsed ? "+" : "-");
    settextstyle(FONT_SIZE, 0, _T("宋体"));
  }
}

// ============================================================
// 绘制：垂直/水平滚动条（编辑区右侧和底部）
// 给用户"可滚动且当前位置在哪"的视觉反馈；不响应拖拽（保持极简）
// 滚动条布局常量（绘制与命中测试共用）
static const int SCROLLBAR_W = 12;
static const int HSCROLLBAR_H = 12;

// 垂直滚动条命中测试
// 返回：0=未命中 1=滑块 2=轨道上方(上翻) 3=轨道下方(下翻)
int HitTestVScrollBar(int mx, int my) {
  int sbW = SCROLLBAR_W;
  int sbX = EDIT_AREA_WIDTH - sbW;
  int trackTop = MENU_BAR_HEIGHT;
  int bottom = WINDOW_HEIGHT - STATUS_BAR_HEIGHT;
  if (mx < sbX || mx >= sbX + sbW || my < trackTop || my >= bottom)
    return 0;
  EnsureFolds();
  int viewCnt = (int)g_viewToDoc.size();
  int visLines = GetVisibleLineCount();
  if (viewCnt <= visLines)
    return 2; // 内容不足一屏，点轨道无意义但仍算命中
  int trackH = bottom - trackTop;
  int thumbH = (visLines * trackH) / viewCnt;
  if (thumbH < 14) thumbH = 14;
  int maxTop = viewCnt - visLines;
  int thumbY = trackTop + (g_topLine * (trackH - thumbH)) / maxTop;
  if (my >= thumbY && my < thumbY + thumbH)
    return 1; // 命中滑块
  if (my < thumbY)
    return 2; // 滑块上方 = 上翻一页
  return 3;   // 滑块下方 = 下翻一页
}

// 水平滚动条命中测试
// 返回：0=未命中 1=滑块 2=轨道左侧(左翻) 3=轨道右侧(右翻)
int HitTestHScrollBar(int mx, int my) {
  int sbW = SCROLLBAR_W;
  int sbX = EDIT_AREA_WIDTH - sbW;
  int hbH = HSCROLLBAR_H;
  int hbY = WINDOW_HEIGHT - STATUS_BAR_HEIGHT - hbH;
  int hbLeft = LINE_NUM_WIDTH;
  int hbW = sbX - hbLeft;
  if (mx < hbLeft || mx >= sbX || my < hbY || my >= hbY + hbH)
    return 0;
  int visCols = GetVisibleColCount();
  int maxCol = 0;
  int rows = g_buffer.getRowCount();
  for (int r = 0; r < rows; r++) {
    int cc = g_buffer.getColCount(r);
    if (cc > maxCol) maxCol = cc;
  }
  if (maxCol <= visCols)
    return 2;
  int thumbW = (visCols * hbW) / maxCol;
  if (thumbW < 14) thumbW = 14;
  int maxLeft = maxCol - visCols;
  int thumbX = hbLeft + (g_leftCol * (hbW - thumbW)) / maxLeft;
  if (mx >= thumbX && mx < thumbX + thumbW)
    return 1;
  if (mx < thumbX)
    return 2;
  return 3;
}

// ============================================================
void DrawScrollBars() {
  EnsureFolds();
  int viewCnt = (int)g_viewToDoc.size();
  int visLines = GetVisibleLineCount();
  int visCols = GetVisibleColCount();
  int bottom = WINDOW_HEIGHT - STATUS_BAR_HEIGHT;
  int sbW = SCROLLBAR_W;                    // 滚动条宽度
  int sbX = EDIT_AREA_WIDTH - sbW; // 垂直条 x
  int trackTop = MENU_BAR_HEIGHT;
  int trackH = bottom - trackTop;

  // 计算最长行作为水平最大列
  int maxCol = 0;
  int rows = g_buffer.getRowCount();
  for (int r = 0; r < rows; r++) {
    int cc = g_buffer.getColCount(r);
    if (cc > maxCol)
      maxCol = cc;
  }

  // —— 垂直滚动条 ——
  // 背景轨道
  setfillcolor(g_theme.lineNumBg);
  bar(sbX, trackTop, sbX + sbW, bottom);
  setlinecolor(g_theme.lineNumSep);
  line(sbX, trackTop, sbX, bottom);
  // 滑块（仅当内容超过可见范围时才有意义）
  if (viewCnt > visLines) {
    int thumbH = (visLines * trackH) / viewCnt;
    if (thumbH < 14)
      thumbH = 14;
    int maxTop = viewCnt - visLines;
    int thumbY = trackTop + (g_topLine * (trackH - thumbH)) / maxTop;
    setfillcolor(g_theme.scrollThumb);
    bar(sbX + 1, thumbY, sbX + sbW - 1, thumbY + thumbH);
  }

  // —— 水平滚动条 ——
  int hbH = HSCROLLBAR_H;
  int hbY = bottom - hbH;
  int hbLeft = LINE_NUM_WIDTH;
  int hbW = sbX - hbLeft;
  setfillcolor(g_theme.lineNumBg);
  bar(hbLeft, hbY, sbX, hbY + hbH);
  setlinecolor(g_theme.lineNumSep);
  line(hbLeft, hbY, sbX, hbY);
  if (maxCol > visCols) {
    int thumbW = (visCols * hbW) / maxCol;
    if (thumbW < 14)
      thumbW = 14;
    int maxLeft = maxCol - visCols;
    int thumbX = hbLeft + (g_leftCol * (hbW - thumbW)) / maxLeft;
    setfillcolor(g_theme.scrollThumb);
    bar(thumbX, hbY + 1, thumbX + thumbW, hbY + hbH - 1);
  }
}

// 前置声明：静态诊断模块的函数定义在文件后部
void DrawLineDiagnosticSquiggles(int r, int y);
bool RunStaticDiagnostics(bool quiet);
void JumpToDiagnostic(int idx, bool preciseCol);
void OpenDiagConfig();
bool IsCSourceFile(const std::string &path);

// ============================================================
// 绘制：编辑区（文本渲染视图 + 光标）
// ============================================================
void DrawEditArea() {
  // 编辑区背景，宽度限制在右侧诊断面板左侧
  setfillcolor(g_theme.editBg);
  bar(0, MENU_BAR_HEIGHT, EDIT_AREA_WIDTH, WINDOW_HEIGHT - STATUS_BAR_HEIGHT);

  // 行号栏背景（浅灰）
  setfillcolor(g_theme.lineNumBg);
  bar(0, MENU_BAR_HEIGHT, LINE_NUM_WIDTH, WINDOW_HEIGHT - STATUS_BAR_HEIGHT);
  // 断点栏背景（比行号栏略深，区分）
  setfillcolor(g_theme.lineNumBg == RGB(238, 238, 242)
                   ? RGB(228, 228, 232)   // 亮色主题
                   : RGB(30, 30, 32));     // 暗色主题
  bar(0, MENU_BAR_HEIGHT, BREAKPOINT_GUTTER_W, WINDOW_HEIGHT - STATUS_BAR_HEIGHT);
  // 断点栏右边分隔线
  setlinecolor(g_theme.lineNumSep);
  line(BREAKPOINT_GUTTER_W, MENU_BAR_HEIGHT, BREAKPOINT_GUTTER_W,
       WINDOW_HEIGHT - STATUS_BAR_HEIGHT);
  // 行号栏右边分隔线
  setlinecolor(g_theme.lineNumSep);
  line(LINE_NUM_WIDTH, MENU_BAR_HEIGHT, LINE_NUM_WIDTH,
       WINDOW_HEIGHT - STATUS_BAR_HEIGHT);

  // 设置文本样式
  setbkcolor(g_theme.editBg);
  settextcolor(g_theme.editText);
  settextstyle(FONT_SIZE, 0, _T("宋体"));

  // 折叠与视图映射须为最新；选区先规范化（保证 startRow <= endRow）
  EnsureFolds();
  if (g_sel.active)
    NormalizeSel();
  ClampScroll(); // 防止折叠/删除后 g_topLine/g_leftCol 越界

  int bottom = WINDOW_HEIGHT - STATUS_BAR_HEIGHT;
  int rows = g_buffer.getRowCount();
  bool inBlock = false; // 是否处于 /* */ 块注释中（按文档行从首行维护）
  int visLines = GetVisibleLineCount();
  int visCols = GetVisibleColCount();
  int xBase =
      TEXT_ORIGIN_X - g_leftCol * CHAR_WIDTH; // 行首屏幕 x（含左滚偏移）

  // 遍历的是"文档行"：被折叠隐藏的行仍需推进块注释状态，只是不绘制；
  // 可见行的屏幕 y 由其"视图行号 - g_topLine"决定。
  for (int r = 0; r < rows; r++) {
    std::string line = g_buffer.getLine(r);
    int vr = g_docToView[r];

    // 隐藏行（折叠块内部）或屏幕外可见行：仅推进 inBlock，不输出
    bool onScreen = (vr >= 0);
    int y = onScreen ? (TEXT_ORIGIN_Y + (vr - g_topLine) * LINE_HEIGHT) : 0;
    // 顶部滚出可见区（vr < g_topLine）也不绘制
    if (onScreen && (vr < g_topLine))
      onScreen = false;
    if (onScreen && (y > bottom - LINE_HEIGHT))
      onScreen = false;
    if (!onScreen) {
      if (!line.empty())
        DrawLineSyntaxColored(xBase, TEXT_ORIGIN_Y, line, inBlock,
                              g_theme.editBg, false);
      continue;
    }

    // 错误行优先红色背景（编译失败时报错行），覆盖当前行浅蓝
    bool errLine = IsErrorLine(r);
    if (errLine) {
      setfillcolor(g_theme.errLineBg);
      bar(0, y - 2, EDIT_AREA_WIDTH, y + LINE_HEIGHT - 2);
    }
    // 当前行（光标所在行）浅蓝底色（错误行不画浅蓝）
    else if (r == g_cursor.row) {
      setfillcolor(g_theme.curLineBg);
      bar(0, y - 2, EDIT_AREA_WIDTH, y + LINE_HEIGHT - 2);
    }

    // 行号显示真实文档行号（折叠时中间行号自然跳过、不连续）
    settextcolor(errLine ? g_theme.errLineNumText
                         : g_theme.lineNumText); // 错误行行号红色
    settextstyle(13, 0, _T("宋体"));
    std::string numStr = IntToStr(r + 1);
    int numW = (int)numStr.size() * 7; // 13号字近似7px宽
    setbkcolor(errLine               ? g_theme.errLineBg
               : (r == g_cursor.row) ? g_theme.curLineBg
                                     : g_theme.lineNumBg);
    OutText(LINE_NUM_WIDTH - 6 - numW, y + 2, numStr);

    // 断点红点：在断点栏中央画实心圆
    if (hasBreakpoint(r)) {
      int cx = BREAKPOINT_GUTTER_W / 2;  // 水平居中于断点栏
      int cy = y + LINE_HEIGHT / 2 - 1;  // 垂直居中于行
      setfillcolor(RGB(220, 40, 40));     // 红色
      fillcircle(cx, cy, 5);              // 半径 5px
    }

    // 文本内容：逐 token 词法着色
    settextstyle(FONT_SIZE, 0, _T("宋体"));
    COLORREF bg = errLine               ? g_theme.errLineBg
                  : (r == g_cursor.row) ? g_theme.curLineBg
                                        : g_theme.editBg;

    // 本行被选中的字节列区间 [selC1, selC2)；无选中为零宽区间。
    // 必须限定行范围：单行选区时其他行不满足 r==startRow/endRow，
    // 若不设防会被误判为"跨行中间行"而整行变色。
    int selC1 = 0, selC2 = -1;
    if (g_sel.active && r >= g_sel.startRow && r <= g_sel.endRow) {
      selC1 = (r == g_sel.startRow) ? g_sel.startCol : 0;
      selC2 = (r == g_sel.endRow) ? g_sel.endCol : g_buffer.getColCount(r);
      if (selC2 < selC1)
        selC2 = selC1;
    }

    if (line.empty()) {
      // 跨多行选区经过的可见空行：画通栏选区底色
      if (g_sel.active && r > g_sel.startRow && r < g_sel.endRow) {
        setfillcolor(g_theme.selBg);
        bar(TEXT_ORIGIN_X, y, EDIT_AREA_WIDTH, y + LINE_HEIGHT - 4);
      }
      // 空行不影响 inBlock 状态
    } else {
      // 注意：不能用 setcliprgn 做横向裁剪 —— 实测本机 EasyX 一旦调用
      // setcliprgn，之后所有文字输出永久失效。水平裁剪在 PaintSeg 内
      // 按可见列窗口切子串完成；右溢出由后绘制的诊断面板/滚动条覆盖。
      DrawLineSyntaxColored(xBase, y, line, inBlock, bg, true, selC1, selC2);

      // 查找匹配高亮：黄色矩形覆盖匹配区，再重绘该段文字保证清晰。
      // 列->像素换算与光标/选区一致（等宽 CHAR_WIDTH）；越界（编辑后
      // 匹配位置失效）则不画，等待下次查找刷新。匹配整段滚出可见列
      // 窗口时也跳过（避免黄色块盖住行号栏）。
      int kwLen = (int)g_findText.size();
      // 仅当匹配整体落在可见列窗口内才绘制：部分滚出左缘时直接输出会
      // 盖住行号栏，且按字节切中文匹配同样会产生乱码。
      if (g_findActive && r == g_findRow && !g_findText.empty() &&
          g_findCol >= 0 && g_findCol + kwLen <= (int)line.size() &&
          g_findCol >= g_leftCol &&
          g_findCol + kwLen <= g_leftCol + visCols) {
        int fx = xBase + g_findCol * CHAR_WIDTH;
        int fw = kwLen * CHAR_WIDTH;
        setfillcolor(g_theme.findHl);
        bar(fx, y - 2, fx + fw, y + LINE_HEIGHT - 2);
        setbkcolor(g_theme.findHl);
        settextcolor(g_theme.editText);
        settextstyle(FONT_SIZE, 0, _T("宋体"));
        OutText(fx, y, g_findText);
      }

      // 折叠中的块：在起点行末尾显示灰色折叠提示（仅在可见列内绘制）
      if (IsFoldCollapsed(r)) {
        // 起点行本身以 '{' 结尾时提示 " ... }"，否则 "{ ... }"，避免 "{ {" 重复
        const char *hint = "{ ... }";
        for (int q = (int)line.size() - 1; q >= 0; q--) {
          if (line[q] == ' ' || line[q] == '\t')
            continue;
          if (line[q] == '{')
            hint = " ... }";
          break;
        }
        int hintCol = g_buffer.getColCount(r) + (line.empty() ? 0 : 1);
        int hx = xBase + hintCol * CHAR_WIDTH;
        // 提示起点滚出左缘（会盖住行号栏）或右边界之外则不画
        if (hintCol >= g_leftCol &&
            hintCol < g_leftCol + visCols + 2) {
          settextstyle(FONT_SIZE, 0, _T("宋体"));
          setbkcolor(bg);
          settextcolor(g_theme.foldHint);
          OutText(hx, y, hint);
        }
      }
    }

    // 静态诊断波浪线（红=error / 黄=warning），画在文本下方
    DrawLineDiagnosticSquiggles(r, y);
  }

  // gutter 折叠标记（[-]/[+]）画在最上层
  DrawFoldGutter();
  DrawScrollBars(); // 滚动条视觉反馈（最上层）
}

// ============================================================
// 绘制：光标（闪烁竖线）
// ============================================================
void DrawCursor() {
  // 基于 GetTickCount 实现闪烁（500ms 周期）
  DWORD t = GetTickCount() / 500;
  bool show = (t % 2 == 0);
  if (!show)
    return;

  EnsureFolds();
  int vr = g_docToView[g_cursor.row];
  if (vr < 0)
    return; // 光标行被折叠隐藏（理论上 RecalcFolds 已保证可见）

  int x = TEXT_ORIGIN_X + (g_cursor.col - g_leftCol) * CHAR_WIDTH;
  int y = TEXT_ORIGIN_Y + (vr - g_topLine) * LINE_HEIGHT;

  // 光标在可见区外时不绘制（滚动后光标可能暂离屏）
  if (vr < g_topLine || vr >= g_topLine + GetVisibleLineCount())
    return;
  if (x < TEXT_ORIGIN_X - CHAR_WIDTH || x >= EDIT_AREA_WIDTH)
    return;

  // 竖线光标：从行顶到行底
  setlinecolor(g_theme.cursorColor);
  setlinestyle(PS_SOLID, 2);
  line(x, y, x, y + LINE_HEIGHT - 4);
  setlinestyle(PS_SOLID, 1);
}

// ============================================================
// 绘制：括号匹配高亮
// 当光标紧邻括号（光标右侧是括号，或光标左侧是括号）时，
// 找到对应匹配括号，两个括号位置画蓝色高亮背景。
// 支持 () [] {} 三种括号。
// ============================================================
void DrawBracketMatch() {
  int rows = g_buffer.getRowCount();
  int curCols = g_buffer.getColCount(g_cursor.row);

  // 确定待匹配的括号位置：(brRow, brCol) 与方向
  // 优先看光标右侧；其次看左侧
  int brRow = g_cursor.row;
  int brCol = -1;
  char brCh = '\0';

  // 右侧（光标所在列的字符）
  if (g_cursor.col < curCols) {
    char ch = g_buffer.getChar(g_cursor.row, g_cursor.col);
    if (ch == '(' || ch == ')' || ch == '[' || ch == ']' || ch == '{' ||
        ch == '}') {
      brCol = g_cursor.col;
      brCh = ch;
    }
  }
  // 左侧（光标前一字符）
  if (brCol < 0 && g_cursor.col > 0) {
    char ch = g_buffer.getChar(g_cursor.row, g_cursor.col - 1);
    if (ch == '(' || ch == ')' || ch == '[' || ch == ']' || ch == '{' ||
        ch == '}') {
      brCol = g_cursor.col - 1;
      brCh = ch;
    }
  }
  if (brCol < 0)
    return; // 附近无括号

  // 搜索匹配括号
  int matchRow = -1, matchCol = -1;
  char openCh, closeCh;
  int dir; // 搜索方向：+1 向后，-1 向前
  switch (brCh) {
  case '(':
    openCh = '(';
    closeCh = ')';
    dir = 1;
    break;
  case ')':
    openCh = '(';
    closeCh = ')';
    dir = -1;
    break;
  case '[':
    openCh = '[';
    closeCh = ']';
    dir = 1;
    break;
  case ']':
    openCh = '[';
    closeCh = ']';
    dir = -1;
    break;
  case '{':
    openCh = '{';
    closeCh = '}';
    dir = 1;
    break;
  case '}':
    openCh = '{';
    closeCh = '}';
    dir = -1;
    break;
  default:
    return;
  }

  // 跨行扫描，depth 计数
  // 按方向确定：targetCh = 要找的匹配括号；startCh =
  // 与起点同类的括号（嵌套计数）
  char targetCh = (dir > 0) ? closeCh : openCh;
  char startCh = (dir > 0) ? openCh : closeCh;
  int depth = 0;
  int r = brRow, c = brCol;
  int maxIter = rows * 200; // 防止无限循环
  while (maxIter-- > 0) {
    // 步进
    if (dir > 0) {
      c++;
    } else {
      c--;
    }
    // 跨行调整
    if (c < 0) {
      r--;
      if (r < 0)
        break;
      c = g_buffer.getColCount(r) - 1;
      if (c < 0) {
        continue;
      } // 空行，继续跨
    } else if (c >= g_buffer.getColCount(r)) {
      // 到行尾：若还有下一行，跨到下一行行首
      if (r < rows - 1) {
        r++;
        c = 0;
      } else {
        break;
      }
    }
    char ch = g_buffer.getChar(r, c);
    if (ch == startCh)
      depth++; // 嵌套同类括号
    else if (ch == targetCh) {
      if (depth == 0) { // 找到匹配
        matchRow = r;
        matchCol = c;
        break;
      }
      depth--;
    }
  }
  if (matchRow < 0)
    return; // 未找到匹配

  EnsureFolds();
  // 折叠隐藏行上的括号不参与高亮（光标行本身一定可见）
  if (g_docToView[brRow] < 0 || g_docToView[matchRow] < 0)
    return;
  // 滚动到可见区外的括号也不画（避免画到错误位置）
  int brVr = g_docToView[brRow];
  int matchVr = g_docToView[matchRow];
  int visLines = GetVisibleLineCount();
  if (brVr < g_topLine || brVr >= g_topLine + visLines)
    return;
  if (matchVr < g_topLine || matchVr >= g_topLine + visLines)
    return;

  // 高亮两个括号：加粗边框
  setlinecolor(g_theme.bracketColor); // 蓝色边框
  setlinestyle(PS_SOLID, 2);

  // 待匹配括号位置
  int x1 = TEXT_ORIGIN_X + (brCol - g_leftCol) * CHAR_WIDTH;
  int y1 = TEXT_ORIGIN_Y + (brVr - g_topLine) * LINE_HEIGHT;
  rectangle(x1 - 1, y1 - 1, x1 + CHAR_WIDTH + 1, y1 + LINE_HEIGHT - 3);

  // 匹配括号位置
  int x2 = TEXT_ORIGIN_X + (matchCol - g_leftCol) * CHAR_WIDTH;
  int y2 = TEXT_ORIGIN_Y + (matchVr - g_topLine) * LINE_HEIGHT;
  rectangle(x2 - 1, y2 - 1, x2 + CHAR_WIDTH + 1, y2 + LINE_HEIGHT - 3);

  setlinestyle(PS_SOLID, 1);
}

// ============================================================
// 绘制：顶部菜单栏
// ============================================================
void DrawMenuBar() {
  setfillcolor(g_theme.menuBarBg);
  bar(0, 0, WINDOW_WIDTH, MENU_BAR_HEIGHT);

  setlinecolor(g_theme.menuBorder);
  line(0, MENU_BAR_HEIGHT, WINDOW_WIDTH, MENU_BAR_HEIGHT);

  setbkcolor(g_theme.menuBarBg);
  settextcolor(g_theme.dropText);
  settextstyle(16, 0, _T("宋体"));

  for (int i = 0; i < g_menuCount; i++) {
    if (i == g_activeMenu) {
      // 已展开的菜单：高亮（更明显）
      setfillcolor(g_theme.highlight);
      bar(g_menus[i].x, 0, g_menus[i].x + g_menus[i].width, MENU_BAR_HEIGHT);
    } else if (i == g_menuBarHover) {
      // 仅悬停（未展开）：颜色加深，给出明显提示
      setfillcolor(g_theme.menuBarHover);
      bar(g_menus[i].x, 0, g_menus[i].x + g_menus[i].width, MENU_BAR_HEIGHT);
    }
    OutText(g_menus[i].x + 14, 6, g_menus[i].name);
  }
}

// ============================================================
// 绘制：子菜单下拉列表
// ============================================================
void DrawSubMenu() {
  if (g_activeMenu < 0)
    return;

  MenuItem &m = g_menus[g_activeMenu];
  int top = MENU_BAR_HEIGHT;
  int h = m.subCount * SUB_ITEM_HEIGHT;

  setfillcolor(g_theme.dropBg);
  setlinecolor(g_theme.dropBorder);
  bar(m.x, top, m.x + SUB_MENU_WIDTH, top + h);
  rectangle(m.x, top, m.x + SUB_MENU_WIDTH, top + h);

  setbkcolor(g_theme.dropBg);
  settextcolor(g_theme.dropText);
  settextstyle(14, 0, _T("宋体"));

  for (int i = 0; i < m.subCount; i++) {
    int y = top + i * SUB_ITEM_HEIGHT;

    // 悬停高亮：鼠标所在项铺浅蓝色背景（左右各缩 1px 避开边框），
    // 文字底色同步切换，保证高亮底色上文字依然清晰
    if (i == g_subMenuHover) {
      setfillcolor(g_theme.dropHover);
      bar(m.x + 1, y, m.x + SUB_MENU_WIDTH - 1, y + SUB_ITEM_HEIGHT);
      setbkcolor(g_theme.dropHover);
    } else {
      setbkcolor(g_theme.dropBg);
    }

    OutText(m.x + 14, y + 4, m.subItems[i]);
  }
}

// ============================================================
// 绘制：底部状态栏（显示文件名 + 光标位置 + 修改状态）
// ============================================================
void DrawStatusBar() {
  int top = WINDOW_HEIGHT - STATUS_BAR_HEIGHT;

  setfillcolor(g_theme.statusBg);
  bar(0, top, WINDOW_WIDTH, WINDOW_HEIGHT);

  setlinecolor(g_theme.statusBorder);
  line(0, top, WINDOW_WIDTH, top);

  setbkcolor(g_theme.statusBg);
  settextcolor(g_theme.statusText);
  settextstyle(13, 0, _T("宋体"));

  // 注意：显示给用户的行号/列号从 1 开始
  OutText(10, top + 6, "文件: " + g_fileName);
  OutText(450, top + 6, "行: " + IntToStr(g_cursor.row + 1));
  OutText(540, top + 6, "列: " + IntToStr(g_cursor.col + 1));
  OutText(630, top + 6, g_modified ? "未保存" : "已保存");
}

// ============================================================
// 绘制：文件完整路径预览气泡（鼠标悬停状态栏"文件:"区时）
// ============================================================
// 超长路径中间省略：C:\dir\sub\...\name.c，保证盘符前缀与文件名可见
static std::string MiddleEllipsizePath(const std::string &path, int maxW) {
  if (textwidth(path.c_str()) <= maxW)
    return path;

  size_t slash = path.find_last_of("\\/");
  std::string dir =
      (slash == std::string::npos) ? "" : path.substr(0, slash + 1);
  std::string name =
      (slash == std::string::npos) ? path : path.substr(slash + 1);

  // 逐级删去目录的最后一级，直到 "dir...name" 放得下或目录耗尽
  std::string disp = dir + "..." + name;
  while (textwidth(disp.c_str()) > maxW && dir.size() >= 3) {
    std::string d = dir.substr(0, dir.size() - 1); // 去掉末尾分隔符
    size_t p = d.find_last_of("\\/");
    if (p == std::string::npos) {
      dir = "";
      break;
    }
    dir = d.substr(0, p + 1);
    disp = dir + "..." + name;
  }
  // 文件名本身也过长：尾部省略
  if (textwidth(disp.c_str()) > maxW) {
    disp = path;
    while (disp.size() > 3 &&
           textwidth((disp.substr(0, disp.size() - 1) + "...").c_str()) >
               maxW) {
      disp.erase(disp.size() - 1);
    }
    disp = disp.substr(0, disp.size() - 1) + "...";
  }
  return disp;
}

void DrawFilePathTooltip() {
  if (!g_statusFileHover)
    return;
  if (GetTickCount() - g_statusHoverTick < 500)
    return; // 悬停 500ms 后才弹出

  std::string text =
      g_filePath.empty() ? std::string("（尚未保存到磁盘）") : g_filePath;

  settextstyle(13, 0, _T("宋体"));
  int maxW = WINDOW_WIDTH - 40;
  if (maxW < 100)
    maxW = 100;
  std::string disp = MiddleEllipsizePath(text, maxW);

  const int padX = 8, padY = 5;
  int tw = textwidth(disp.c_str());
  int th = 13;
  int bw = tw + padX * 2;
  int bh = th + padY * 2;

  int x = 8;
  int y = WINDOW_HEIGHT - STATUS_BAR_HEIGHT - bh - 6;
  if (x + bw > WINDOW_WIDTH - 8)
    x = WINDOW_WIDTH - 8 - bw;
  if (x < 8)
    x = 8;
  if (y < MENU_BAR_HEIGHT + 4)
    y = MENU_BAR_HEIGHT + 4; // 小窗口时退到菜单栏下方

  // 阴影
  setfillcolor(g_theme.tipShadow);
  bar(x + 2, y + 2, x + bw + 2, y + bh + 2);
  // 主体（系统 tooltip 风格）
  setfillcolor(g_theme.tipBg);
  bar(x, y, x + bw, y + bh);
  // 边框
  setlinecolor(g_theme.tipBorder);
  rectangle(x, y, x + bw - 1, y + bh - 1);

  setbkcolor(g_theme.tipBg);
  settextcolor(g_theme.tipText);
  OutText(x + padX, y + padY, disp);
}

// ============================================================
// 帮助窗口：使用说明（软件介绍 + 基础操作 + 快捷键一览）
// 模态弹窗：标题栏可拖动，× / Esc / 点击外部关闭，滚轮滚动内容
// ============================================================
bool g_helpOpen = false;
int g_helpX = 0, g_helpY = 0;  // 窗口左上角（打开时居中，可拖动）
int g_helpScroll = 0;          // 已滚过的行数
bool g_helpHoverClose = false; // × 按钮悬停
bool g_helpDragging = false;   // 正在拖动标题栏
int g_helpDragOffX = 0, g_helpDragOffY = 0;

const int HELP_W = 640;      // 窗口宽度
const int HELP_TITLE_H = 28; // 标题栏高度
const int HELP_LINE_H = 20;  // 每行文本高度

// 帮助文本行
// style: 0=正文(text) 1=小节标题(text) 2=快捷键(text=按键 desc=功能说明)
struct HelpLine {
  const char *text;
  const char *desc;
  int style;
};

static const HelpLine g_helpLines[] = {
    {"软件简介", "", 1},
    {"    Mini C Studio 是一款基于 C++ 与 EasyX 图形库开发的", "", 0},
    {"  轻量级 C 语言集成开发环境（IDE），提供源码编辑、语法", "", 0},
    {"  着色、语法检测、编译、运行与断点调试等一站式功能，", "", 0},
    {"  适合 C 语言入门学习与小型程序开发。", "", 0},
    {"", "", 0},
    {"界面布局", "", 1},
    {"- 顶部菜单栏：文件 / 编辑 / 运行 / 帮助，点击展开下拉菜单", "", 0},
    {"- 标签栏：同时编辑多个文件，点击标签切换，点击 × 关闭", "", 0},
    {"- 编辑区：行号、断点栏、代码折叠、括号匹配、自动补全", "", 0},
    {"- 右侧诊断面板：编译错误 / 警告列表，单击跳到行首，", "", 0},
    {"    双击精确定位到行列", "", 0},
    {"- 底部状态栏：光标位置、编码、文件路径与修改状态", "", 0},
    {"", "", 0},
    {"基础操作", "", 1},
    {"- 新建 / 打开：菜单\"文件\"，或 Ctrl+N / Ctrl+O", "", 0},
    {"- 保存 / 另存为：Ctrl+S / Ctrl+Shift+S；关闭未保存的", "", 0},
    {"    文件会逐个弹出确认，防止代码丢失", "", 0},
    {"- 撤销 / 重做：Ctrl+Z / Ctrl+Y（或 Ctrl+Shift+Z）", "", 0},
    {"- 剪切 / 复制 / 粘贴 / 全选：Ctrl+X / C / V / A", "", 0},
    {"- 查找与替换：Ctrl+F，输入关键词回车查找，结果列表", "", 0},
    {"    支持勾选后逐项或全部替换", "", 0},
    {"- 转到行 / 转到函数：Ctrl+G / Ctrl+Shift+G", "", 0},
    {"- 代码折叠：Ctrl+Shift+[ 全部折叠，Ctrl+Shift+] 全部展开", "", 0},
    {"- 语法检测：菜单\"运行\"，无需编译即可提示常见错误", "", 0},
    {"- 编译：Ctrl+F9；编译并运行：Ctrl+F5 或 F10", "", 0},
    {"- 调试运行：F5，配合断点单步观察变量", "", 0},
    {"- 断点：单击编辑区最左侧断点栏切换，调试时命中即暂停", "", 0},
    {"- 变量监视：调试暂停后可添加变量名实时查看取值", "", 0},
    {"- 切换亮色 / 暗色主题：Ctrl+T；全屏切换：F11", "", 0},
    {"", "", 0},
    {"快捷键一览", "", 1},
    {"Ctrl+N", "新建文件", 2},
    {"Ctrl+O", "打开文件", 2},
    {"Ctrl+S", "保存", 2},
    {"Ctrl+Shift+S", "另存为", 2},
    {"Ctrl+Z", "撤销", 2},
    {"Ctrl+Y / Ctrl+Shift+Z", "重做", 2},
    {"Ctrl+X / Ctrl+C / Ctrl+V", "剪切 / 复制 / 粘贴", 2},
    {"Ctrl+A", "全选", 2},
    {"Ctrl+F", "查找与替换", 2},
    {"Ctrl+G", "转到行", 2},
    {"Ctrl+Shift+G", "转到函数", 2},
    {"Ctrl+F9", "编译", 2},
    {"Ctrl+F5 / F10", "编译并运行", 2},
    {"F5", "调试运行", 2},
    {"Ctrl+Shift+[ / ]", "全部折叠 / 全部展开", 2},
    {"Ctrl+Tab", "切换标签", 2},
    {"Ctrl+T", "切换主题", 2},
    {"F11", "全屏", 2},
    {"Esc", "关闭弹窗 / 菜单", 2},
    {"", "", 0},
    {"小贴士", "", 1},
    {"- 鼠标悬停状态栏文件名，可预览文件完整路径", "", 0},
    {"- 输入代码时自动弹出补全列表，Esc 或点击别处关闭", "", 0},
    {"- 拖动窗口边缘可自由缩放，界面布局自适应", "", 0},
    {"- 右键编辑区可呼出快捷菜单，常用操作一键直达", "", 0},
};
static const int g_helpLineCount =
    sizeof(g_helpLines) / sizeof(g_helpLines[0]);

// 帮助窗口矩形（高度随主窗口自适应，超出上限则固定）
static void GetHelpRect(int &rx, int &ry, int &rw, int &rh) {
  rx = g_helpX;
  ry = g_helpY;
  rw = HELP_W;
  rh = WINDOW_HEIGHT - MENU_BAR_HEIGHT - STATUS_BAR_HEIGHT - 60;
  if (rh > 580) rh = 580;
  if (rh < 320) rh = 320;
}

// 内容区可容纳的行数
static int HelpVisibleLines() {
  int rx, ry, rw, rh;
  GetHelpRect(rx, ry, rw, rh);
  return (rh - HELP_TITLE_H - 20) / HELP_LINE_H;
}

// 最大可滚动行数
static int HelpMaxScroll() {
  int m = g_helpLineCount - HelpVisibleLines();
  return m > 0 ? m : 0;
}

// 滚动夹紧（窗口缩放后调用）
static void ClampHelpScroll() {
  if (g_helpScroll < 0) g_helpScroll = 0;
  if (g_helpScroll > HelpMaxScroll()) g_helpScroll = HelpMaxScroll();
}

// 位置夹紧（拖动后 / 窗口缩放后保持可见）
static void ClampHelpPos() {
  int rx, ry, rw, rh;
  GetHelpRect(rx, ry, rw, rh);
  if (g_helpX < 0) g_helpX = 0;
  if (g_helpX > WINDOW_WIDTH - rw) g_helpX = WINDOW_WIDTH - rw;
  if (g_helpY < MENU_BAR_HEIGHT) g_helpY = MENU_BAR_HEIGHT;
  if (g_helpY > WINDOW_HEIGHT - STATUS_BAR_HEIGHT - rh)
    g_helpY = WINDOW_HEIGHT - STATUS_BAR_HEIGHT - rh;
}

bool PtInHelpWindow(int mx, int my) {
  if (!g_helpOpen) return false;
  int rx, ry, rw, rh;
  GetHelpRect(rx, ry, rw, rh);
  return mx >= rx && mx < rx + rw && my >= ry && my < ry + rh;
}

// × 关闭按钮命中
bool HitHelpClose(int mx, int my) {
  if (!g_helpOpen) return false;
  int rx, ry, rw, rh;
  GetHelpRect(rx, ry, rw, rh);
  int cx = rx + rw - 24, cy = ry + 4;
  return mx >= cx && mx < cx + 20 && my >= cy && my < cy + 20;
}

// 标题栏命中（用于拖动，不含 × 按钮）
bool HitHelpTitle(int mx, int my) {
  if (!g_helpOpen) return false;
  int rx, ry, rw, rh;
  GetHelpRect(rx, ry, rw, rh);
  if (HitHelpClose(mx, my)) return false;
  return mx >= rx && mx < rx + rw && my >= ry && my < ry + HELP_TITLE_H;
}

void OpenHelpWindow() {
  g_helpOpen = true;
  g_helpScroll = 0;
  g_helpHoverClose = false;
  g_helpDragging = false;
  int rx, ry, rw, rh;
  GetHelpRect(rx, ry, rw, rh);
  g_helpX = (WINDOW_WIDTH - rw) / 2;
  g_helpY = MENU_BAR_HEIGHT + 20;
  ClampHelpPos();
}

void DrawHelpWindow() {
  if (!g_helpOpen) return;

  ClampHelpPos();
  ClampHelpScroll();
  int rx = g_helpX, ry = g_helpY, rw, rh;
  GetHelpRect(rx, ry, rw, rh);
  int rx2 = rx + rw, ry2 = ry + rh;

  // 阴影 + 主体 + 边框
  setfillcolor(RGB(0, 0, 0));
  bar(rx + 3, ry + 3, rx2 + 3, ry2 + 3);
  setfillcolor(g_theme.dropBg);
  setlinecolor(g_theme.dropBorder);
  bar(rx, ry, rx2, ry2);
  rectangle(rx, ry, rx2, ry2);

  // 标题栏
  setfillcolor(g_theme.menuBarHover);
  bar(rx + 1, ry + 1, rx2 - 1, ry + HELP_TITLE_H);
  setbkcolor(g_theme.menuBarHover);
  settextcolor(g_theme.dropText);
  OutText(rx + 10, ry + 6, "使用说明 - Mini C Studio");

  // × 关闭按钮（悬停变红）
  {
    int bx = rx + rw - 24, by = ry + 4, bs = 20;
    setfillcolor(g_helpHoverClose ? g_theme.errLineBg : g_theme.menuBarHover);
    bar(bx, by, bx + bs, by + bs);
    setbkcolor(g_helpHoverClose ? g_theme.errLineBg : g_theme.menuBarHover);
    settextcolor(g_helpHoverClose ? RGB(255, 255, 255) : g_theme.dropText);
    OutText(bx + 6, by + 2, "x");
  }

  // 内容区
  int contentTop = ry + HELP_TITLE_H + 6;
  int contentBot = ry2 - 22; // 底部提示条上方
  int visLines = (contentBot - contentTop) / HELP_LINE_H;
  settextstyle(14, 0, _T("宋体"));
  setbkcolor(g_theme.dropBg);
  for (int i = 0; i < visLines; i++) {
    int idx = g_helpScroll + i;
    if (idx >= g_helpLineCount) break;
    const HelpLine &hl = g_helpLines[idx];
    int y = contentTop + i * HELP_LINE_H;
    if (hl.style == 1) {
      // 小节标题：主题蓝 + 下划分隔线
      settextcolor(g_theme.keyword);
      std::string s = std::string("\xA1\xF0 ") + hl.text; // "◆ "
      OutText(rx + 12, y, s);
      setlinecolor(g_theme.dropSeparator);
      line(rx + 12, y + HELP_LINE_H - 4, rx + rw - 40, y + HELP_LINE_H - 4);
    } else if (hl.style == 2) {
      // 快捷键行：按键高亮 + 功能说明
      settextcolor(g_theme.keyword);
      OutText(rx + 34, y, hl.text);
      settextcolor(g_theme.dropText);
      OutText(rx + 250, y, hl.desc);
    } else if (hl.text[0]) {
      settextcolor(g_theme.dropText);
      OutText(rx + 12, y, hl.text);
    }
  }

  // 滚动条（内容超出时显示）
  int maxScroll = HelpMaxScroll();
  if (maxScroll > 0) {
    int sbX = rx2 - 12;
    int sbH = contentBot - contentTop;
    setfillcolor(g_theme.lineNumBg);
    bar(sbX, contentTop, sbX + 6, contentBot);
    int thumbH = sbH * visLines / g_helpLineCount;
    if (thumbH < 24) thumbH = 24;
    int thumbY = contentTop +
                 (sbH - thumbH) * g_helpScroll / maxScroll;
    setfillcolor(g_theme.scrollThumb);
    bar(sbX, thumbY, sbX + 6, thumbY + thumbH);
  }

  // 底部提示条
  setfillcolor(g_theme.lineNumBg);
  bar(rx + 1, ry2 - 21, rx2 - 1, ry2 - 1);
  setbkcolor(g_theme.lineNumBg);
  settextcolor(g_theme.diagMuted);
  OutText(rx + 10, ry2 - 19, "滚轮滚动查看更多内容，Esc 或点击窗口外关闭");
  {
    char page[64];
    int cur = g_helpScroll + visLines;
    if (cur > g_helpLineCount) cur = g_helpLineCount;
    sprintf(page, "%d / %d 行", cur, g_helpLineCount);
    OutText(rx2 - 10 - textwidth(page), ry2 - 19, page);
  }
}

// ============================================================
// 关于窗口：软件版本信息（模态弹窗）
// ============================================================
bool g_aboutOpen = false;
int g_aboutX = 0, g_aboutY = 0;
bool g_aboutHoverClose = false;
bool g_aboutDragging = false;
int g_aboutDragOffX = 0, g_aboutDragOffY = 0;

const int ABOUT_W = 480;
const int ABOUT_H = 300;

static void GetAboutRect(int &rx, int &ry, int &rw, int &rh) {
  rx = g_aboutX;
  ry = g_aboutY;
  rw = ABOUT_W;
  rh = ABOUT_H;
}

static void ClampAboutPos() {
  if (g_aboutX < 0) g_aboutX = 0;
  if (g_aboutX > WINDOW_WIDTH - ABOUT_W) g_aboutX = WINDOW_WIDTH - ABOUT_W;
  if (g_aboutY < MENU_BAR_HEIGHT) g_aboutY = MENU_BAR_HEIGHT;
  if (g_aboutY > WINDOW_HEIGHT - STATUS_BAR_HEIGHT - ABOUT_H)
    g_aboutY = WINDOW_HEIGHT - STATUS_BAR_HEIGHT - ABOUT_H;
}

bool PtInAboutWindow(int mx, int my) {
  if (!g_aboutOpen) return false;
  int rx, ry, rw, rh;
  GetAboutRect(rx, ry, rw, rh);
  return mx >= rx && mx < rx + rw && my >= ry && my < ry + rh;
}

bool HitAboutClose(int mx, int my) {
  if (!g_aboutOpen) return false;
  int rx, ry, rw, rh;
  GetAboutRect(rx, ry, rw, rh);
  int cx = rx + rw - 24, cy = ry + 4;
  return mx >= cx && mx < cx + 20 && my >= cy && my < cy + 20;
}

// 标题栏命中（用于拖动，不含 × 按钮）
bool HitAboutTitle(int mx, int my) {
  if (!g_aboutOpen) return false;
  int rx, ry, rw, rh;
  GetAboutRect(rx, ry, rw, rh);
  if (HitAboutClose(mx, my)) return false;
  return mx >= rx && mx < rx + rw && my >= ry && my < ry + HELP_TITLE_H;
}

void OpenAboutWindow() {
  g_aboutOpen = true;
  g_aboutHoverClose = false;
  g_aboutDragging = false;
  g_aboutX = (WINDOW_WIDTH - ABOUT_W) / 2;
  g_aboutY = (WINDOW_HEIGHT - ABOUT_H) / 2;
  ClampAboutPos();
}

void DrawAboutWindow() {
  if (!g_aboutOpen) return;

  ClampAboutPos();
  int rx = g_aboutX, ry = g_aboutY, rw, rh;
  GetAboutRect(rx, ry, rw, rh);
  int rx2 = rx + rw, ry2 = ry + rh;

  // 阴影 + 主体 + 边框
  setfillcolor(RGB(0, 0, 0));
  bar(rx + 3, ry + 3, rx2 + 3, ry2 + 3);
  setfillcolor(g_theme.dropBg);
  setlinecolor(g_theme.dropBorder);
  bar(rx, ry, rx2, ry2);
  rectangle(rx, ry, rx2, ry2);

  settextstyle(14, 0, _T("宋体"));

  // 标题栏
  setfillcolor(g_theme.menuBarHover);
  bar(rx + 1, ry + 1, rx2 - 1, ry + HELP_TITLE_H);
  setbkcolor(g_theme.menuBarHover);
  settextcolor(g_theme.dropText);
  OutText(rx + 10, ry + 6, "关于 Mini C Studio");

  // × 关闭按钮
  {
    int bx = rx + rw - 24, by = ry + 4, bs = 20;
    setfillcolor(g_aboutHoverClose ? g_theme.errLineBg : g_theme.menuBarHover);
    bar(bx, by, bx + bs, by + bs);
    setbkcolor(g_aboutHoverClose ? g_theme.errLineBg : g_theme.menuBarHover);
    settextcolor(g_aboutHoverClose ? RGB(255, 255, 255) : g_theme.dropText);
    OutText(bx + 6, by + 2, "x");
  }

  setbkcolor(g_theme.dropBg);

  // 软件名 + 版本
  settextstyle(22, 0, _T("宋体"));
  settextcolor(g_theme.keyword);
  {
    const char *name = "Mini C Studio";
    OutText(rx + (rw - textwidth(name)) / 2, ry + 44, name);
  }
  settextstyle(14, 0, _T("宋体"));
  settextcolor(g_theme.diagMuted);
  {
    const char *ver = "版本 6.0 · 课程实践项目";
    OutText(rx + (rw - textwidth(ver)) / 2, ry + 76, ver);
  }

  // 分隔线
  setlinecolor(g_theme.dropSeparator);
  line(rx + 30, ry + 108, rx2 - 30, ry + 108);

  // 简介正文（居中多行）
  static const char *aboutBody[] = {
      "一款轻量级 C 语言集成开发环境（IDE）",
      "支持多标签编辑、语法着色、语法检测、编译运行",
      "断点调试、查找替换与代码折叠等功能",
  };
  settextcolor(g_theme.dropText);
  for (int i = 0; i < 3; i++) {
    OutText(rx + (rw - textwidth(aboutBody[i])) / 2, ry + 124 + i * 22,
            aboutBody[i]);
  }

  // 技术信息（居中，次要文字）
  static const char *aboutTech[] = {
      "技术栈：C++ / EasyX / MinGW GCC",
      "运行环境：Windows 7 及以上（32 位）",
  };
  settextcolor(g_theme.diagMuted);
  for (int i = 0; i < 2; i++) {
    OutText(rx + (rw - textwidth(aboutTech[i])) / 2, ry + 196 + i * 20,
            aboutTech[i]);
  }

  // 底部版权条
  setfillcolor(g_theme.lineNumBg);
  bar(rx + 1, ry2 - 25, rx2 - 1, ry2 - 1);
  setbkcolor(g_theme.lineNumBg);
  settextcolor(g_theme.diagMuted);
  {
    const char *cp = "(C) 2026 Mini C Studio · 仅供教学学习使用";
    OutText(rx + (rw - textwidth(cp)) / 2, ry2 - 22, cp);
  }
}

// ============================================================
// 全部重绘
// ============================================================
// 前置声明（DrawDiagPanel 在后面定义）
void DrawDiagPanel();
void DrawContextMenu();  // 右键上下文菜单（最上层）
void DrawTabBar();       // 多文档标签栏
void DrawFuncList();     // 函数跳转列表弹窗
void DrawAutocomplete(); // 代码补全弹窗
void DrawFindBox();      // 查找弹窗（最上层）
void DrawFindResults();  // 查找结果列表（最上层）
void DrawWatchWindow();   // 变量监视窗口（最上层）

void RedrawAll() {
  DrawEditArea();
  DrawTabBar(); // 覆盖菜单栏与编辑区首行之间的空带
  DrawCursor();
  DrawBracketMatch(); // 括号匹配高亮
  DrawDiagPanel();    // 右侧诊断面板
  DrawMenuBar();
  DrawSubMenu();
  DrawStatusBar();
  DrawFilePathTooltip(); // 文件路径预览气泡（状态栏之上）
  DrawContextMenu();     // 右键菜单绘制在最上层
  DrawFuncList();        // 函数跳转列表（最上层）
  DrawAutocomplete();    // 代码补全弹窗
  DrawFindBox();         // 查找弹窗（最上层）
  DrawFindResults();     // 查找结果列表（最上层）
  DrawWatchWindow();      // 变量监视窗口（最上层）
  DrawHelpWindow();      // 帮助窗口（最上层）
  DrawAboutWindow();     // 关于窗口（最上层）
  FlushBatchDraw();
}

// ============================================================
// 点击检测：菜单栏
// ============================================================
int HitTestMenuBar(int mx, int my) {
  if (my < 0 || my >= MENU_BAR_HEIGHT)
    return -1;
  for (int i = 0; i < g_menuCount; i++) {
    if (mx >= g_menus[i].x && mx < g_menus[i].x + g_menus[i].width) {
      return i;
    }
  }
  return -1;
}

// ============================================================
// 点击检测：子菜单
// ============================================================
int HitTestSubMenu(int mx, int my) {
  if (g_activeMenu < 0)
    return -1;

  MenuItem &m = g_menus[g_activeMenu];
  int top = MENU_BAR_HEIGHT;
  int h = m.subCount * SUB_ITEM_HEIGHT;

  if (my < top || my >= top + h)
    return -1;
  if (mx < m.x || mx >= m.x + SUB_MENU_WIDTH)
    return -1;

  return (my - top) / SUB_ITEM_HEIGHT;
}

// ============================================================
// 显示"待实现"弹窗
// ============================================================
void ShowNotImplemented(const char *funcName) {
  std::string msg = std::string(funcName) + " 功能待实现";
  MessageBoxA(GetHWnd(), msg.c_str(), "提示", MB_OK | MB_ICONINFORMATION);
}

// ============================================================
// 从路径中提取文件名（最后一个 \ 或 / 之后部分）
// ============================================================
std::string ExtractFileName(const std::string &path) {
  size_t pos = path.find_last_of("\\/");
  if (pos == std::string::npos)
    return path;
  return path.substr(pos + 1);
}

// 取文件所在目录（含末尾分隔符）；无目录则返回空串
std::string FolderOfFile(const std::string &path) {
  size_t pos = path.find_last_of("\\/");
  if (pos == std::string::npos)
    return std::string();
  return path.substr(0, pos + 1);
}

// 由源文件路径推导同目录、同主名的 exe 路径：
//   "C:\dir\test.c" -> "C:\dir\test.exe"
// 无扩展名时直接追加 .exe
std::string ExePathForSource(const std::string &src) {
  size_t slash = src.find_last_of("\\/");
  size_t dot = src.find_last_of('.');
  if (dot != std::string::npos && (slash == std::string::npos || dot > slash)) {
    return src.substr(0, dot) + ".exe";
  }
  return src + ".exe";
}

// 给命令行参数加双引号（路径含空格/中文时必需）
std::string QuoteArg(const std::string &s) { return "\"" + s + "\""; }

// ============================================================
// 标准"保存为"对话框（DEV-C++ 风格）
// defaultName：文件名输入框的默认值，如 "未命名1.c"
// 返回用户选择的完整路径；取消则返回空
// ============================================================
std::string ShowSaveCFileDialog(const char *defaultName) {
  char file[MAX_PATH] = {0};
  if (defaultName) {
    strncpy(file, defaultName, MAX_PATH - 1);
    file[MAX_PATH - 1] = '\0';
  }

  // 默认打开"我的文档"目录（与 DEV-C++ 一致）；不存在则交系统决定
  char docDir[MAX_PATH] = {0};
  DWORD n = GetEnvironmentVariableA("USERPROFILE", docDir, MAX_PATH);
  const char *initialDir = NULL;
  if (n > 0 && n < MAX_PATH - 16) {
    strcat(docDir, "\\Documents");
    if (GetFileAttributesA(docDir) != INVALID_FILE_ATTRIBUTES)
      initialDir = docDir;
  }

  OPENFILENAMEA ofn;
  memset(&ofn, 0, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = GetHWnd();
  ofn.lpstrFilter = "C source files (*.c)\0*.c\0All files (*.*)\0*.*\0";
  ofn.lpstrFile = file;
  ofn.nMaxFile = MAX_PATH;
  ofn.lpstrTitle = "保存为";
  ofn.lpstrDefExt = "c"; // 用户不写扩展名时自动补 .c
  ofn.lpstrInitialDir = initialDir;
  ofn.Flags = OFN_OVERWRITEPROMPT // 覆盖已有文件时确认
              | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

  if (GetSaveFileNameA(&ofn))
    return std::string(file);
  return "";
}

// ============================================================
// 编译/运行前确保源文件已落盘（DEV-C++ 行为）：
// 新文件（尚无路径）先弹标准"保存为"对话框让用户选择保存位置；
// 随后把当前编辑内容写入源文件（"编译即保存"）。
// 用户在保存对话框点取消则中止本次编译/运行。
// 返回 true 表示可以继续构建。
// ============================================================
bool EnsureSavedForBuild() {
  // 新文件：先选择保存路径
  if (g_filePath.empty()) {
    std::string path = ShowSaveCFileDialog("未命名1.c");
    if (path.empty())
      return false; // 用户取消 → 中止
    g_filePath = path;
    g_fileName = ExtractFileName(path);
  }

  // 把当前编辑内容写入源文件（编译实际保存的文件，而非临时副本）
  std::ofstream fout(g_filePath.c_str(), std::ios::binary);
  if (!fout.is_open()) {
    MessageBoxA(
        GetHWnd(),
        ("无法写入文件：\n" + g_filePath + "\n（权限不足或路径无效）").c_str(),
        "保存失败", MB_OK | MB_ICONERROR);
    return false;
  }
  std::string text = g_buffer.getText();
  fout.write(text.c_str(), text.size());
  fout.close();

  g_modified = false; // 磁盘内容已与编辑器一致
  return true;
}

// ============================================================
// 提示错误（带原因）
// ============================================================
void ShowError(const char *title, const std::string &detail) {
  MessageBoxA(GetHWnd(), detail.c_str(), title, MB_OK | MB_ICONERROR);
}

bool FileSave(); // 前向声明：ConfirmDiscardChanges 中"是"时保存

// ============================================================
// 新建/打开前的未保存修改确认（DEV-C++ 行为）
// 当前文档有未保存修改时弹"是/否/取消"：
//   是 → 保存（新文件会先弹"保存为"），保存成功才继续；
//   否 → 放弃修改，继续；
//   取消 → 中止本次新建/打开。
// 返回 true 表示可以继续，false 表示用户取消。
// ============================================================
bool ConfirmDiscardChanges() {
  if (!g_modified)
    return true;

  std::string name = g_filePath.empty() ? std::string("未命名") : g_fileName;
  std::string msg =
      "文件 \"" + name + "\" 的内容已修改。\n\n是否先保存这些修改？";
  int r = MessageBoxA(GetHWnd(), msg.c_str(), "Mini-C Studio",
                      MB_YESNOCANCEL | MB_ICONQUESTION);
  if (r == IDCANCEL)
    return false;
  if (r == IDNO)
    return true;     // 放弃修改，继续
  return FileSave(); // 保存；若用户在"保存为"中取消则中止
}

// ============================================================
// 多文档标签（快照式：切换时把当前文档全部状态存入 DocState，
// 再把目标标签状态恢复到全局量。对现有编辑逻辑零侵入）
// ============================================================
struct DocState {
  std::string text;     // 全文快照
  std::string fileName; // 显示用文件名
  std::string filePath; // 完整路径（空 = 未保存）
  bool modified;
  Cursor cursor;
  Selection sel;
  int anchorRow, anchorCol; // 鼠标选区锚点
  int highlightLine;        // 诊断错误定位行
  bool buildOk;
  std::vector<Fold> folds;          // 折叠块集合
  std::vector<int> collapsedStarts; // 折叠状态
  std::vector<int> viewToDoc;
  std::vector<int> docToView;
  bool foldsDirty;
  std::vector<DocSnapshot> undoStack; // 撤销/重做历史
  std::vector<DocSnapshot> redoStack;
  bool undoTyping;
  DWORD undoLastTick;
  std::vector<DiagEntry> diag; // 诊断信息
  int diagSelIdx;
  int diagSource; // 诊断来源（0 构建 / 1 静态诊断），随标签保存

  DocState()
      : modified(false), anchorRow(0), anchorCol(0), highlightLine(-1),
        buildOk(false), foldsDirty(true), undoTyping(false), undoLastTick(0),
        diagSelIdx(-1), diagSource(0) {
    fileName = "未命名";
    cursor.row = 0; // POD 成员须显式清零，否则值为不确定
    cursor.col = 0;
    sel.startRow = 0;
    sel.startCol = 0;
    sel.endRow = 0;
    sel.endCol = 0;
    sel.active = false;
  }
};

std::vector<DocState> g_tabs(1); // 初始一个空文档标签（与初始全局量一致）
int g_activeTab = 0;

// 函数跳转列表弹窗状态（定义在文件操作区之后，此处前向声明供 LoadDocState
// 使用）
extern bool g_funcListOpen;
extern bool g_autocompleteOpen;

// 动态标签宽度：数量多时压缩到不越出编辑区（上限 120，下限 60）
int TabWidth() {
  int n = (int)g_tabs.size();
  if (n < 1)
    n = 1;
  int w = EDIT_AREA_WIDTH / n;
  if (w > 120)
    w = 120;
  if (w < 60)
    w = 60;
  return w;
}

// 把当前活动文档的全部状态写入对应标签
void SaveActiveDocState() {
  if (g_activeTab < 0 || g_activeTab >= (int)g_tabs.size())
    return;
  DocState &s = g_tabs[g_activeTab];
  s.text = g_buffer.getText();
  s.fileName = g_fileName;
  s.filePath = g_filePath;
  s.modified = g_modified;
  s.cursor = g_cursor;
  s.sel = g_sel;
  s.anchorRow = g_selAnchorRow;
  s.anchorCol = g_selAnchorCol;
  s.highlightLine = g_highlightLine;
  s.buildOk = g_buildOk;
  s.folds = g_folds;
  s.collapsedStarts = g_collapsedStarts;
  s.viewToDoc = g_viewToDoc;
  s.docToView = g_docToView;
  s.foldsDirty = g_foldsDirty;
  s.undoStack = g_undoStack;
  s.redoStack = g_redoStack;
  s.undoTyping = g_undoTyping;
  s.undoLastTick = g_undoLastTick;
  if (g_diagCount > 0 && g_diagList != NULL)
    s.diag.assign(g_diagList, g_diagList + g_diagCount);
  else
    s.diag.clear();
  s.diagSelIdx = g_diagSelIdx;
  s.diagSource = g_diagSource;
}

// 把标签 idx 的状态恢复到全局量并设为活动标签
void LoadDocState(int idx) {
  if (idx < 0 || idx >= (int)g_tabs.size())
    return;
  g_activeTab = idx;
  DocState &s = g_tabs[idx];
  g_buffer.setText(s.text);
  g_fileName = s.fileName;
  g_filePath = s.filePath;
  g_modified = s.modified;
  g_cursor = s.cursor;
  g_sel = s.sel;
  g_selAnchorRow = s.anchorRow;
  g_selAnchorCol = s.anchorCol;
  g_mouseSelecting = false;   // 切换瞬间不可能处于拖拽中
  g_funcListOpen = false;     // 切换标签时关闭函数跳转列表
  g_autocompleteOpen = false; // 切换标签时关闭补全弹窗
  CloseFindBox();             // 切换标签时关闭查找弹窗
  g_buildOk = s.buildOk;
  g_folds = s.folds;
  g_collapsedStarts = s.collapsedStarts;
  g_viewToDoc = s.viewToDoc;
  g_docToView = s.docToView;
  g_foldsDirty = s.foldsDirty;
  g_undoStack = s.undoStack;
  g_redoStack = s.redoStack;
  g_undoTyping = s.undoTyping;
  g_undoLastTick = s.undoLastTick;
  DiagClear(); // 清空后按快照恢复
  for (size_t k = 0; k < s.diag.size(); k++)
    DiagAdd(s.diag[k]);
  g_diagSelIdx = s.diagSelIdx;
  g_diagSource = s.diagSource;
  g_highlightLine = s.highlightLine;
  // 查找匹配不随标签持久化：换标签后旧位置对新文件无意义，直接清除
  g_findActive = false;
  g_findRow = -1;
  g_findCol = -1;
  // 切标签后滚动归零（不同文档的视图行偏移无对应关系）
  g_topLine = 0;
  g_leftCol = 0;
  EnsureCursorVisible();
}

// 新建一个空文档标签并激活（先保存当前标签状态，避免丢失）
void CreateTab() {
  SaveActiveDocState();
  DocState s;
  g_tabs.push_back(s);
  LoadDocState((int)g_tabs.size() - 1);
}

// 切换到标签 idx（当前文档状态先存回快照）
void SwitchTab(int idx) {
  if (idx < 0 || idx >= (int)g_tabs.size())
    return;
  if (idx == g_activeTab)
    return;
  SaveActiveDocState();
  LoadDocState(idx);
}

// Ctrl+Tab 循环切换（dir = 1 下一个 / -1 上一个）
void CycleTab(int dir) {
  int n = (int)g_tabs.size();
  if (n <= 1)
    return;
  int na = g_activeTab + dir;
  if (na < 0)
    na = n - 1;
  if (na >= n)
    na = 0;
  SwitchTab(na);
}

// 关闭标签 idx：未保存先确认（是/否/取消），取消则中止
void CloseTab(int idx) {
  int n = (int)g_tabs.size();
  if (idx < 0 || idx >= n)
    return;
  int prev = g_activeTab;
  if (idx != prev) {
    SaveActiveDocState(); // 暂存当前活动文档
    LoadDocState(idx);    // 切到目标再做保存确认
  }
  if (!ConfirmDiscardChanges()) {
    if (idx != prev) {
      SaveActiveDocState();
      LoadDocState(prev);
    }
    return;
  }
  SaveActiveDocState(); // 记录确认后的状态（可能已另存）
  g_tabs.erase(g_tabs.begin() + idx);
  n = (int)g_tabs.size();
  if (n == 0) { // 全部关闭：重建一个空文档标签
    g_tabs.push_back(DocState());
    LoadDocState(0);
    return;
  }
  if (idx == prev) {
    // 关闭的是活动标签：优先激活右邻，否则左邻
    LoadDocState(idx < n ? idx : n - 1);
  } else {
    // 关闭的是后台标签：保持原活动标签（索引可能左移）
    LoadDocState(prev > idx ? prev - 1 : prev);
  }
}

// 退出前逐个确认未保存标签；返回 false 表示用户取消退出
bool ConfirmDiscardAllTabs() {
  SaveActiveDocState();
  for (;;) {
    int n = (int)g_tabs.size();
    int target = -1;
    // 从活动标签起顺序找第一个未保存文档
    for (int k = 0; k < n; k++) {
      int i = (g_activeTab + k) % n;
      if (g_tabs[i].modified) {
        target = i;
        break;
      }
    }
    if (target < 0)
      return true; // 全部已保存
    if (target != g_activeTab)
      SwitchTab(target);
    if (!ConfirmDiscardChanges())
      return false;
    g_modified = false;   // "否"分支：放弃修改
    SaveActiveDocState(); // 同步回快照，继续找下一个
  }
}

// 标签命中检测：返回标签索引，-1 表示未命中
int HitTestTab(int mx, int my) {
  if (my < MENU_BAR_HEIGHT || my >= TEXT_ORIGIN_Y)
    return -1;
  if (mx < 0 || mx >= EDIT_AREA_WIDTH)
    return -1;
  int w = TabWidth();
  int i = mx / w;
  if (i >= (int)g_tabs.size())
    return -1;
  return i;
}

// 关闭按钮 × 命中（标签右侧 16px 区域）
bool HitTestTabClose(int mx, int my, int ti) {
  (void)my;
  int w = TabWidth();
  int x0 = ti * w;
  return mx >= x0 + w - 18 && mx < x0 + w - 2;
}

// 按 GBK 双字节宽度截断标签文字（防止溢出关闭按钮区）
std::string TruncateTabLabel(const std::string &s, int maxCols) {
  int cols = 0;
  size_t i = 0;
  while (i < s.size()) {
    bool wide = ((unsigned char)s[i] >= 0x81) && (i + 1 < s.size());
    int w = wide ? 2 : 1;
    if (cols + w > maxCols)
      return s.substr(0, i) + "..";
    cols += w;
    i += wide ? 2 : 1;
  }
  return s;
}

// 标签栏绘制：菜单栏与编辑区首行之间的空带 y ∈ [28, 50)
void DrawTabBar() {
  int y0 = MENU_BAR_HEIGHT;
  int h = TEXT_ORIGIN_Y - MENU_BAR_HEIGHT;
  int w = EDIT_AREA_WIDTH;
  setfillcolor(g_theme.tabBarBg);
  solidrectangle(0, y0, w - 1, y0 + h - 1);
  setlinecolor(g_theme.tabBorder);
  line(0, y0 + h - 1, w - 1, y0 + h - 1); // 与编辑区分隔线
  setbkmode(TRANSPARENT);
  int tw = TabWidth();
  for (int i = 0; i < (int)g_tabs.size(); i++) {
    int x0 = i * tw;
    bool act = (i == g_activeTab);
    setfillcolor(act ? g_theme.tabActiveBg : g_theme.tabBg);
    solidrectangle(x0, y0, x0 + tw - 1, y0 + h - 1);
    setlinecolor(g_theme.tabBorder);
    rectangle(x0, y0, x0 + tw - 1, y0 + h - 1);
    // 文件名 + 未保存标记（活动标签读实时全局量，快照仅在切换时同步）
    std::string label = act ? g_fileName : g_tabs[i].fileName;
    if (act ? g_modified : g_tabs[i].modified)
      label += " *";
    int maxCols = (tw - 26) / 8; // 去掉内边距与关闭区后的可用列数
    if (maxCols < 2)
      maxCols = 2;
    label = TruncateTabLabel(label, maxCols);
    settextcolor(act ? g_theme.tabActiveText : g_theme.tabText);
    OutText(x0 + 6, y0 + (h - textheight(label.c_str())) / 2, label);
    // 关闭按钮
    settextcolor(g_theme.tabCloseText);
    OutText(x0 + tw - 18, y0 + (h - textheight("×")) / 2, "×");
  }
}

// ============================================================
// 文件操作：新建（开一个新空文档标签，当前文档原样保留）
// ============================================================
void FileNew() { CreateTab(); }

// ============================================================
// 文件操作：打开（标准"打开"对话框，支持一次选中多个文件，
// 每个文件开一个标签；同路径已打开则直接切换）
// ============================================================
void FileOpen() {
  // 标准多选"打开"对话框（64KB 缓冲容纳大量选择结果）
  char buf[65536];
  buf[0] = '\0';
  OPENFILENAMEA ofn;
  memset(&ofn, 0, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = GetHWnd();
  ofn.lpstrFilter = "C 源文件 (*.c)\0*.c\0所有文件 (*.*)\0*.*\0";
  ofn.lpstrFile = buf;
  ofn.nMaxFile = sizeof(buf);
  ofn.lpstrTitle = "打开";
  ofn.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_FILEMUSTEXIST |
              OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

  if (!GetOpenFileNameA(&ofn))
    return; // 用户取消

  // 解析选择结果：单选 → buf 即完整路径；
  // 多选 → 第一段是目录，后续各段是文件名（\0 分隔）
  std::vector<std::string> paths;
  std::string first(buf);
  if (first.empty())
    return;
  char *p = buf + first.size() + 1;
  if (*p == '\0') {
    paths.push_back(first);
  } else {
    std::string dir = first;
    if (dir[dir.size() - 1] != '\\' && dir[dir.size() - 1] != '/')
      dir += "\\";
    while (*p) {
      paths.push_back(dir + std::string(p));
      p += strlen(p) + 1;
    }
  }

  // 逐个打开：同路径已开则切换（去重）；否则新开标签。最后停在最后一个
  for (size_t k = 0; k < paths.size(); k++) {
    const std::string &path = paths[k];

    // 同路径标签已存在 → 直接切换
    int exist = -1;
    for (int i = 0; i < (int)g_tabs.size(); i++) {
      if (g_tabs[i].filePath == path) {
        exist = i;
        break;
      }
    }
    // 活动标签的状态可能尚未同步回快照，单独比较
    if (exist < 0 && g_filePath == path)
      exist = g_activeTab;
    if (exist >= 0) {
      SwitchTab(exist);
      continue;
    }

    // 读取文件全部内容
    std::ifstream fin(path.c_str(), std::ios::binary);
    if (!fin.is_open()) {
      ShowError("打开失败",
                "无法打开文件：\n" + path + "\n（文件不存在或权限不足）");
      continue;
    }
    std::string content;
    char fbuf[4096];
    while (fin.read(fbuf, sizeof(fbuf)) || fin.gcount() > 0) {
      content.append(fbuf, fin.gcount());
    }
    fin.close();

    // 活动标签是"未动过"的空文档 → 直接载入它；否则新开标签
    if (!(g_filePath.empty() && !g_modified && g_buffer.getText().empty())) {
      CreateTab();
    }
    g_buffer.setText(content);
    g_filePath = path;
    g_fileName = ExtractFileName(path);
    g_modified = false;
    g_cursor.row = 0;
    g_cursor.col = 0;
    g_sel.active = false;
    g_topLine = 0; // 新文件从头显示
    g_leftCol = 0;
    g_buildOk = false; // 新载入的文件视为未编译
    DiagClear();
    UndoReset(); // 新载入的文件重新开始历史
    g_collapsedStarts.clear();
    MarkFoldsDirty(true);
    SaveActiveDocState(); // 同步回标签快照
  }
}

// ============================================================
// 文件操作：保存（写入当前路径；若无路径则等同另存为）
// 返回 true 表示保存成功
// ============================================================
bool FileSave() {
  if (g_filePath.empty()) {
    std::string path = ShowSaveCFileDialog("未命名1.c");
    if (path.empty())
      return false; // 用户取消
    g_filePath = path;
    g_fileName = ExtractFileName(path);
  }

  std::ofstream fout(g_filePath.c_str(), std::ios::binary);
  if (!fout.is_open()) {
    std::string err =
        "无法写入文件：\n" + g_filePath + "\n（权限不足或路径无效）";
    ShowError("保存失败", err);
    g_filePath = ""; // 复位，下次仍需另存
    return false;
  }
  std::string text = g_buffer.getText();
  fout.write(text.c_str(), text.size());
  fout.close();

  g_modified = false;

  // 保存即诊断：对 C 系源文件在后台跑一次 clang/gcc 静态检查
  if (IsCSourceFile(g_filePath))
    RunStaticDiagnostics(true);
  return true;
}

// ============================================================
// 文件操作：另存为
// ============================================================
void FileSaveAs() {
  // 默认文件名取当前文件名；新文件则给"未命名1.c"
  const char *defName = g_filePath.empty() ? "未命名1.c" : g_fileName.c_str();
  std::string path = ShowSaveCFileDialog(defName);
  if (path.empty())
    return;

  std::ofstream fout(path.c_str(), std::ios::binary);
  if (!fout.is_open()) {
    std::string err = "无法写入文件：\n" + path;
    ShowError("另存失败", err);
    return;
  }
  std::string text = g_buffer.getText();
  fout.write(text.c_str(), text.size());
  fout.close();

  g_filePath = path;
  g_fileName = ExtractFileName(path);
  g_modified = false;
  // 另存后同样对 C 系源文件做一次静态诊断
  if (IsCSourceFile(g_filePath))
    RunStaticDiagnostics(true);
  MessageBoxA(GetHWnd(), "文件已另存", "提示", MB_OK | MB_ICONINFORMATION);
}

// ============================================================
// 跳转：把光标移动到指定文档行（0-based）
// 若目标行被折叠隐藏，先展开包含它的折叠块；并高亮该行。
// ============================================================
void JumpToLine(int target) {
  int rows = g_buffer.getRowCount();
  if (rows < 1)
    rows = 1;
  if (target < 0)
    target = 0;
  if (target >= rows)
    target = rows - 1;
  EnsureFolds();
  if (g_docToView[target] < 0)
    ExpandFoldsContaining(target);
  g_cursor.row = target;
  int cols = g_buffer.getColCount(target);
  if (g_cursor.col > cols)
    g_cursor.col = cols;
  g_sel.active = false;
  g_highlightLine = target; // 高亮目标行，便于定位
  EnsureCursorVisible();    // 滚动到目标行可见
}

// ============================================================
// 转到行：弹输入框让用户输入行号（1-based），回车跳转
// ============================================================
void GoToLine() {
  char buf[32] = {0};
  char def[16];
  sprintf(def, "%d", g_cursor.row + 1); // 默认当前行号
  bool ok = InputBox(buf, sizeof(buf), "请输入要跳转的行号（从 1 开始）：",
                     "转到行", def, 300, 200);
  if (!ok)
    return;
  // 提取首个数字
  int line = 0;
  for (const char *p = buf; *p; p++) {
    if (*p >= '0' && *p <= '9')
      line = line * 10 + (*p - '0');
    else if (line > 0)
      break; // 数字后遇到非数字则停止
  }
  if (line <= 0) {
    MessageBoxA(GetHWnd(), "请输入有效的正整数行号。", "转到行",
                MB_OK | MB_ICONWARNING);
    return;
  }
  JumpToLine(line - 1); // 转为 0-based
}

// ============================================================
// 查找：区分大小写，向下逐行查找关键词并高亮显示
// （状态变量 g_findActive/g_findText/g_findRow/g_findCol 定义在
//   g_highlightLine 附近的全局区，供 LoadDocState 提前引用）
// ============================================================

// 查找结果弹窗（"开始查找"按钮触发：独立自绘弹窗，可拖动/关闭）
struct FindResult {
  int row;          // 0-based 文档行
  int col;          // 0-based 字节列
  std::string text; // 匹配的文本片段（用于列表显示）
};
std::vector<FindResult> g_findResults; // 全部匹配
std::vector<bool> g_findResultChecked; // 每行复选框勾选状态（全选默认）
bool g_findResultOpen = false;         // 结果弹窗是否显示
int g_findResultX = 0, g_findResultY = 0; // 结果弹窗位置（独立于查找框）
const int FIND_RESULT_W = 380;          // 弹窗固定宽度
int FIND_RESULT_H = 0;                  // 弹窗高度（按匹配数动态）
int g_findResultScroll = 0;             // 滚动偏移（行数）
int g_findResultHover = -1;             // 列表项悬停索引
bool g_findResultHoverClose = false;     // 关闭按钮悬停
bool g_findResultDragging = false;      // 正在拖动弹窗
int g_findResultDragOffX = 0, g_findResultDragOffY = 0;
// 替换输入框相关（在结果弹窗顶部）
std::string g_findRepInput;             // 替换为输入框内容
bool g_findRepFocused = false;          // 替换输入框是否有键盘焦点
// 注：替换框的光标/选区与查找框共用 g_findCursor / g_findSelAnchor
const int FIND_RESULT_ITEM_H = 18;
const int FIND_RESULT_MAX_VISIBLE = 10;
const int FIND_RESULT_HEADER_H = 28;    // 标题栏高度
const int FIND_RESULT_REPL_H = 32;      // "替换为"输入框行高度（含 label）
const int FIND_RESULT_BTN_H = 30;       // 底部按钮区高度

// 从 (startRow, startCol) 开始向下、向右查找第一个匹配。
// std::string::find 本身就是区分大小写的精确匹配，跨行由外层循环完成。
// 找到：记录匹配位置并把光标跳到匹配起点（折叠行先展开），返回 true；
// 未找到：返回 false。
bool FindFrom(int startRow, int startCol) {
  int rows = g_buffer.getRowCount();
  for (int r = startRow; r < rows; r++) {
    std::string line = g_buffer.getLine(r);
    // 首行从 startCol 起找，其后各行从头找（实现"之后"语义）
    size_t from = (r == startRow) ? (size_t)startCol : 0;
    if (from > line.size())
      continue;
    size_t pos = line.find(g_findText, from);
    if (pos != std::string::npos) {
      g_findActive = true;
      g_findRow = r;
      g_findCol = (int)pos;

      EnsureFolds();
      if (g_docToView[r] < 0)
        ExpandFoldsContaining(r);
      g_cursor.row = r;
      g_cursor.col = (int)pos;
      g_sel.active = false;
      EnsureCursorVisible(); // 找到的位置可能滚动出可见区
      return true;
    }
  }
  return false;
}

// 收集全文所有匹配（不重叠），存入 g_findResults。
// 每项记录行号、列号、匹配文本片段（截断到 24 字节避免过长）。
void FindAllMatches() {
  g_findResults.clear();
  std::string kw = g_findText; // FindBoxConfirm 先设好 g_findText 再调用
  if (kw.empty())
    return;
  int rows = g_buffer.getRowCount();
  for (int r = 0; r < rows; r++) {
    std::string line = g_buffer.getLine(r);
    size_t pos = 0;
    while ((pos = line.find(kw, pos)) != std::string::npos) {
      FindResult fr;
      fr.row = r;
      fr.col = (int)pos;
      // 截取匹配位置周围最多 24 字节作为预览（含关键词本身）
      int ctxStart = (int)pos;
      if (ctxStart > 8) ctxStart = (int)pos - 8; // 前 8 字节上下文
      int ctxLen = (int)pos - ctxStart + (int)kw.size() + 8;
      if (ctxStart + ctxLen > (int)line.size())
        ctxLen = (int)line.size() - ctxStart;
      fr.text = line.substr(ctxStart, ctxLen);
      g_findResults.push_back(fr);
      pos += kw.size(); // 跳过整个关键词，避免重叠
    }
  }
  g_findResultScroll = 0;
}

// ============================================================
// 查找与替换弹窗（自绘）：标题行（右上角 × 关闭）、查找输入框、
// 替换输入框、按钮行（开始查找 / 替换 / 全部替换）。
// 菜单 / Ctrl+F / 右键菜单打开。
// ============================================================
const int FIND_BOX_W = 300; // 弹窗宽度
const int FIND_BOX_H = 76;  // 弹窗高度（标题 + 一个输入框 + 开始查找按钮）
bool g_findBoxOpen = false;
std::string g_findInput;                    // 查找输入框内容（字节串，GBK）
int g_findBoxField = 0;                     // 键盘焦点字段：固定 0=查找框
int g_findBoxX = 0, g_findBoxY = 0;         // 弹窗位置（打开时居中，可拖动）
bool g_findBoxHoverClose = false;           // × 按钮悬停
bool g_findBoxHoverBtn = false;             // 开始查找按钮悬停
bool g_findBoxDragging = false;             // 正在拖动弹窗
int g_findDragOffX = 0, g_findDragOffY = 0; // 拖动起点相对弹窗的偏移
bool g_findBoxDirty = false;                // 键盘路径的重绘请求（主循环消费）
int g_findCursor = 0;             // 焦点字段输入光标（字节偏移，GBK 边界）
int g_findSelAnchor = -1;         // 焦点字段拖选锚点（-1 = 无选区）
bool g_findInputDragging = false; // 正在输入框内拖选文本

// 当前键盘焦点的输入框内容（光标/选区状态归焦点字段所有，切换字段时重置）
static std::string &FieldText() {
  // 键盘焦点路由：替换输入框 > 查找输入框
  if (g_findRepFocused) return g_findRepInput;
  return g_findInput;
}

// 打开弹窗期间强制 IME 英文模式的记忆（关闭时恢复）
static DWORD g_findSavedConv = 0;
static DWORD g_findSavedSent = 0;
static bool g_findImeSaved = false;

void OpenFindBox() {
  g_findBoxOpen = true;
  g_findInput = g_findText; // 预填上次关键词，回车即"查找下一个"
  g_findBoxHoverClose = false;
  g_findBoxHoverBtn = false;
  // （替换按钮已移除）
  g_findBoxDragging = false;
  g_findInputDragging = false;
  g_findBoxField = 0;                     // 焦点在查找框
  g_findCursor = (int)g_findInput.size(); // 光标在末尾
  g_findSelAnchor = -1;
  // 初始位置：程序窗口正中
  g_findBoxX = (WINDOW_WIDTH - FIND_BOX_W) / 2;
  g_findBoxY = (WINDOW_HEIGHT - FIND_BOX_H) / 2;
  // 中文输入法模式下，字母会进入拼音合成窗口而到不了程序（前两版
  // "输入不了"的根因）。弹窗打开期间把 IME 切到英文半角模式，保证
  // 关键词可以直接键入；关闭时恢复原状态。
  g_findImeSaved = false;
  HIMC himc = ImmGetContext(GetHWnd());
  if (himc) {
    if (ImmGetOpenStatus(himc)) {
      DWORD conv = 0, sent = 0;
      if (ImmGetConversionStatus(himc, &conv, &sent)) {
        g_findSavedConv = conv;
        g_findSavedSent = sent;
        g_findImeSaved = true;
        ImmSetConversionStatus(
            himc, conv & ~(IME_CMODE_NATIVE | IME_CMODE_FULLSHAPE), sent);
      }
    }
    ImmReleaseContext(GetHWnd(), himc);
  }
  g_findBoxDirty = true;
}

void CloseFindBox() {
  g_findBoxOpen = false;
  g_findBoxHoverClose = false;
  g_findBoxHoverBtn = false;
  // （替换按钮已移除）
  g_findBoxDragging = false;
  g_findInputDragging = false;
  g_findSelAnchor = -1;
  // 注意：g_findResultOpen 保持不变 —— 结果弹窗与查找框独立存在
  // 恢复弹窗打开前的输入法状态
  if (g_findImeSaved) {
    HIMC himc = ImmGetContext(GetHWnd());
    if (himc) {
      ImmSetConversionStatus(himc, g_findSavedConv, g_findSavedSent);
      ImmReleaseContext(GetHWnd(), himc);
    }
    g_findImeSaved = false;
  }
  g_findBoxDirty = true;
}

// ---- 输入框光标/选区辅助（字节偏移一律按 GBK 字符边界对齐；
//      查找框与替换框共用，操作对象为键盘焦点字段 FieldText()）----

// pos 对齐到 <= pos 的字符边界（光标左移、删除前定位用）
static int FindBoxSnap(const std::string &s, int pos) {
  int n = (int)s.size();
  if (pos <= 0)
    return 0;
  if (pos >= n)
    return n;
  int i = 0;
  while (i < pos) {
    unsigned char c = (unsigned char)s[i];
    int step = (c >= 0x81 && i + 1 < n) ? 2 : 1;
    if (i + step > pos)
      return i; // pos 落在双字节字符中间
    i += step;
  }
  return i;
}

// pos 对齐到 >= pos 的字符边界（光标右移，跨过双字节字符）
static int FindBoxSnapNext(const std::string &s, int pos) {
  int n = (int)s.size();
  if (pos <= 0)
    return 0;
  if (pos >= n)
    return n;
  int i = 0;
  while (i < n) {
    unsigned char c = (unsigned char)s[i];
    int step = (c >= 0x81 && i + 1 < n) ? 2 : 1;
    i += step;
    if (i >= pos)
      return i;
  }
  return n;
}

// 左侧滚出输入框可视区的字节数（尾部滚动显示，光标始终可见）
static int FindBoxHiddenBytes(const std::string &s, int iw) {
  int pad = iw - 10;
  int hidden = 0;
  int n = (int)s.size();
  while (hidden < n && textwidth(s.substr(hidden).c_str()) > pad) {
    unsigned char c = (unsigned char)s[hidden];
    int step = (c >= 0x81 && hidden + 1 < n) ? 2 : 1;
    hidden += step;
  }
  return hidden;
}

// 输入框内像素偏移 -> 字节偏移（取最近字符边界，鼠标点击/拖选用）
static int FindBoxXToByte(const std::string &s, int dx, int hidden) {
  int n = (int)s.size();
  if (dx <= 0 || n <= hidden)
    return hidden;
  int best = hidden, bestDist = dx;
  int x = 0, i = hidden;
  while (i < n) {
    unsigned char c = (unsigned char)s[i];
    int step = (c >= 0x81 && i + 1 < n) ? 2 : 1;
    x += textwidth(s.substr(i, step).c_str());
    int d = x - dx;
    if (d < 0)
      d = -d;
    if (d < bestDist) {
      bestDist = d;
      best = i + step;
    }
    i += step;
  }
  return best;
}

// 删除焦点字段的当前选区（若有），光标落在选区起点
static void FindBoxDeleteSel() {
  if (g_findSelAnchor < 0)
    return;
  std::string &s = FieldText();
  int a = g_findSelAnchor, b = g_findCursor;
  if (a > b) {
    int t = a;
    a = b;
    b = t;
  }
  s.erase((size_t)a, (size_t)(b - a));
  g_findCursor = a;
  g_findSelAnchor = -1;
}

// 在焦点字段光标处插入字符（有选区先删除选区；WM_CHAR 的 DBCS 双字节
// 分两次到达，逐字节插入后自然拼成完整字符）
void FindBoxInsertChar(char ch) {
  if ((unsigned char)ch < 0x20)
    return;
  std::string &s = FieldText();
  if (s.size() >= 255)
    return;
  // 中央去重：三条输入通道（按键轮询/钩子翻译/WM_CHAR）可能对同一次
  // 击键各产生一份字符，同字符 25ms 内只收第一份。人手重复同字符
  // 最快约 50ms、长按重复间隔 50ms，均不受影响。
  static char s_lastCh = 0;
  static DWORD s_lastTick = 0;
  DWORD now = GetTickCount();
  if (ch == s_lastCh && now - s_lastTick < 25)
    return;
  s_lastCh = ch;
  s_lastTick = now;

  FindBoxDeleteSel();
  s.insert((size_t)g_findCursor, 1, ch);
  g_findCursor += 1; // DBCS 场景光标停在两字节中间，属瞬时态
  g_findSelAnchor = -1;
  g_findBoxDirty = true;
}

void FindBoxBackspace() {
  if (g_findSelAnchor >= 0) {
    FindBoxDeleteSel();
    g_findBoxDirty = true;
    return;
  }
  std::string &s = FieldText();
  int cur = FindBoxSnap(s, g_findCursor);
  if (cur <= 0)
    return;
  int p = FindBoxSnap(s, cur - 1); // 自动回退整个 GBK 双字节字符
  s.erase((size_t)p, (size_t)(cur - p));
  g_findCursor = p;
  g_findSelAnchor = -1;
  g_findBoxDirty = true;
}

// Delete 键：删除焦点字段光标处一个 GBK 字符
void FindBoxDeleteKey() {
  if (g_findSelAnchor >= 0) {
    FindBoxDeleteSel();
    g_findBoxDirty = true;
    return;
  }
  std::string &s = FieldText();
  int n = (int)s.size();
  int cur = FindBoxSnap(s, g_findCursor);
  if (cur >= n)
    return;
  unsigned char c = (unsigned char)s[cur];
  int step = (c >= 0x81 && cur + 1 < n) ? 2 : 1;
  s.erase((size_t)cur, (size_t)step);
  g_findSelAnchor = -1;
  g_findBoxDirty = true;
}

// 切换键盘焦点字段（查找框 <-> 替换框）：光标移到新字段末尾，选区收起
static void FindBoxFocusField(int f) {
  // 字段 1（替换框）位于查找结果面板中，面板未打开时不可切换
  if (f == 1 && !g_findResultOpen)
    return;
  if (g_findBoxField == f)
    return;
  g_findBoxField = f;
  g_findRepFocused = (f == 1); // FieldText() 据此路由到对应输入框
  g_findCursor = (int)FieldText().size();
  g_findSelAnchor = -1;
  g_findBoxDirty = true;
}

// "开始查找"：收集全文所有匹配并显示结果列表面板。
// 若无匹配则提示；否则打开结果面板供用户点选跳转。
void FindBoxConfirm() {
  if (g_findInput.empty())
    return;
  g_findText = g_findInput; // 供编辑区高亮使用
  FindAllMatches();
  if (g_findResults.empty()) {
    g_findResultOpen = false;
    g_findActive = false;
    MessageBoxA(GetHWnd(), "未找到匹配内容", "查找",
                MB_OK | MB_ICONINFORMATION);
    return;
  }
  g_findResultOpen = true;
  // 初始位置：查找框右下方偏移 20px（若超出窗口则夹紧）
  FIND_RESULT_H = FIND_RESULT_HEADER_H + FIND_RESULT_REPL_H + FIND_RESULT_BTN_H +
                  (FIND_RESULT_MAX_VISIBLE < (int)g_findResults.size()
                       ? FIND_RESULT_MAX_VISIBLE
                       : (int)g_findResults.size()) *
                      FIND_RESULT_ITEM_H;
  // 居中显示
  g_findResultX = (WINDOW_WIDTH - FIND_RESULT_W) / 2;
  g_findResultY = MENU_BAR_HEIGHT + 10 +
                  ((WINDOW_HEIGHT - STATUS_BAR_HEIGHT - MENU_BAR_HEIGHT - 20 -
                    FIND_RESULT_H) >> 1);
  if (g_findResultX < 0) g_findResultX = 0;
  if (g_findResultY < MENU_BAR_HEIGHT + 10) g_findResultY = MENU_BAR_HEIGHT + 10;
  g_findResultHoverClose = false;
  g_findResultDragging = false;
  // 每行复选框默认全选
  g_findResultChecked.assign(g_findResults.size(), true);
  // 替换输入框初始状态（保留上次内容方便连续替换）；
  // 光标/选区与查找框共用 g_findCursor / g_findSelAnchor，聚焦时重置
  g_findRepInput = g_findRepInput; // 保留上次输入（复用）
  g_findRepFocused = false;
  g_findBoxField = 0; // 键盘焦点回到查找框
  g_findCursor = (int)g_findInput.size();
  g_findSelAnchor = -1;

  // 同时高亮第一个匹配（设为当前匹配位置）
  g_findActive = true;
  g_findRow = g_findResults[0].row;
  g_findCol = g_findResults[0].col;
  g_findBoxDirty = true;
}

// 点击查找结果列表中某一项：跳转到该匹配位置并选中文本
void JumpToFindResult(int idx) {
  if (idx < 0 || idx >= (int)g_findResults.size())
    return;
  const FindResult &fr = g_findResults[idx];
  // 设为当前匹配高亮
  g_findActive = true;
  g_findRow = fr.row;
  g_findCol = fr.col;

  // 展开折叠块并跳转光标
  EnsureFolds();
  if (g_docToView[fr.row] < 0)
    ExpandFoldsContaining(fr.row);
  g_cursor.row = fr.row;
  g_cursor.col = fr.col;

  // 选中匹配文本（蓝底高亮）
  g_sel.active = true;
  g_sel.startRow = fr.row;
  g_sel.startCol = fr.col;
  g_sel.endRow = fr.row;
  g_sel.endCol = fr.col + (int)g_findText.size();
  g_highlightLine = -1;

  EnsureCursorVisible();
  g_findBoxDirty = true;
}

// 菜单/右键/Ctrl+F 入口：打开查找与替换弹窗
void Find() { OpenFindBox(); }

// ============================================================
// 替换：单处替换 + 全部替换
// ============================================================

// 校验 (g_findRow, g_findCol) 处是否仍是完整关键词（编辑后位置可能漂移）
static bool FindMatchStillValid(const std::string &kw) {
  if (!g_findActive || g_findRow < 0 || g_findCol < 0)
    return false;
  if (g_findRow >= g_buffer.getRowCount())
    return false;
  std::string line = g_buffer.getLine(g_findRow);
  if (g_findCol + (int)kw.size() > (int)line.size())
    return false;
  return line.compare(g_findCol, kw.size(), kw) == 0;
}

// 在 (r, c) 处把关键词原位替换为 rep：先逐字符删除旧词，再逐字符插入新词。
// 行内操作不跨行，折叠结构不受影响（与编辑路径行内删除同语义）。
static void ApplyReplaceAt(int r, int c, const std::string &kw,
                           const std::string &rep) {
  for (int i = 0; i < (int)kw.size(); i++)
    g_buffer.deleteChar(r, c);
  for (int i = 0; i < (int)rep.size(); i++)
    g_buffer.insertChar(r, c + i, rep[i]);
  MarkFoldsDirty(false); // 行数不变，保留折叠
  g_modified = true;
}

// "替换"按钮：替换当前高亮的这一个匹配，然后自动查找下一个。
// 若当前没有有效匹配（还没查找过 / 查找后编辑过导致位置漂移），
// 从头重新查找 —— 不能沿用 FindBoxConfirm 的"从上次位置之后继续"
// 逻辑：那会跳过早于旧位置的匹配，或在匹配已移到旧位置之前时
// 误报"未找到"（表现为按钮偶尔失效）。
void ReplaceOne() {
  std::string kw = g_findInput;
  if (kw.empty())
    return;

  if (!FindMatchStillValid(kw)) {
    g_findText = kw; // FindFrom 内部用 g_findText 匹配
    if (!FindFrom(0, 0)) {
      g_findActive = false;
      g_findRow = -1;
      g_findCol = -1;
      MessageBoxA(GetHWnd(), "未找到匹配内容", "替换",
                  MB_OK | MB_ICONINFORMATION);
      return;
    }
  }

  int r = g_findRow, c = g_findCol;

  // 匹配行可能位于折叠块内（查找后用户又折叠了）：先展开再替换，
  // 否则替换在缓冲区完成但画面上看不到，也像"按钮失效"。
  EnsureFolds();
  if (g_docToView[r] < 0)
    ExpandFoldsContaining(r);

  ApplyReplaceAt(r, c, kw, g_findRepInput);

  // 光标移到替换文本之后
  g_cursor.row = r;
  g_cursor.col = c + (int)g_findRepInput.size();
  g_sel.active = false;
  EnsureCursorVisible(); // 替换后光标位置变化，让其可见

  // 从替换文本末尾之后继续找下一个（即使替换文本包含关键词也不会
  // 被重复匹配 —— 天然规避死循环）
  g_findText = kw;
  g_findActive = false;
  if (!FindFrom(r, c + (int)g_findRepInput.size()))
    MessageBoxA(GetHWnd(), "后面没有更多匹配了", "替换",
                MB_OK | MB_ICONINFORMATION);
}

// "全部替换"按钮：从 (0,0) 起逐行扫描，替换所有不重叠匹配。
// 防死循环：每行的匹配位置集合基于原始行内容一次性收集完毕，从右往左
// 原位替换 —— 替换产生的文本永远不会被重新扫描，因此替换文本即使
// 包含关键词（如 abc -> abcabc）也不会被二次替换。
void ReplaceAll() {
  std::string kw = g_findInput;
  if (kw.empty())
    return;
  std::string rep = g_findRepInput;
  int count = 0;

  for (int r = 0; r < g_buffer.getRowCount(); r++) {
    std::string line = g_buffer.getLine(r);
    // 收集本行所有不重叠匹配（pos += kwLen 跳过整个关键词）
    std::vector<int> hits;
    size_t pos = 0;
    while ((pos = line.find(kw, pos)) != std::string::npos) {
      hits.push_back((int)pos);
      pos += kw.size();
    }
    if (hits.empty())
      continue;
    // 从右往左替换：左侧匹配位置不受右侧编辑影响
    for (int i = (int)hits.size() - 1; i >= 0; i--)
      ApplyReplaceAt(r, hits[i], kw, rep);
    count += (int)hits.size();
  }

  // 全文已无旧关键词，清除旧匹配高亮状态
  g_findActive = false;
  g_findRow = -1;
  g_findCol = -1;

  char msg[64];
  sprintf(msg, "共替换 %d 处", count);
  MessageBoxA(GetHWnd(), msg, "全部替换", MB_OK | MB_ICONINFORMATION);
  // 替换后重新搜索并刷新结果面板
  FindAllMatches();
  g_findResultChecked.assign(g_findResults.size(), true);
  if (g_findResults.empty())
    g_findResultOpen = false;
}

// 替换选中项 / 全部替换 共用的 (row,col) 收集结构
struct ReplaceHit { int r; int c; };

// "替换选中项"：仅替换 g_findResultChecked[idx]==true 的那些匹配。
// 策略：按 (row, col) 列表从右往左替换（行从大到小，同行内列从大到小），
// 避免替换位置漂移。替换完后重新 FindAllMatches 刷新面板。
void ReplaceSelectedInResults() {
  std::string kw = g_findInput;
  if (kw.empty() || g_findResults.empty())
    return;
  // 先收集选中的 (row, col)
  std::vector<ReplaceHit> hits;
  for (size_t i = 0; i < g_findResults.size(); i++) {
    if (i < g_findResultChecked.size() && g_findResultChecked[i]) {
      hits.push_back({g_findResults[i].row, g_findResults[i].col});
    }
  }
  if (hits.empty()) {
    MessageBoxA(GetHWnd(), "没有选中任何项", "替换选中",
                MB_OK | MB_ICONINFORMATION);
    return;
  }
  // 从右往左：按 row 降序、col 降序排序
  for (size_t i = 0; i < hits.size(); i++) {
    for (size_t j = i + 1; j < hits.size(); j++) {
      bool swap = hits[j].r > hits[i].r ||
                  (hits[j].r == hits[i].r && hits[j].c > hits[i].c);
      if (swap) { ReplaceHit t = hits[i]; hits[i] = hits[j]; hits[j] = t; }
    }
  }
  for (size_t i = 0; i < hits.size(); i++) {
    EnsureFolds();
    if (g_docToView[hits[i].r] < 0)
      ExpandFoldsContaining(hits[i].r);
    ApplyReplaceAt(hits[i].r, hits[i].c, kw, g_findRepInput);
  }

  char msg[64];
  sprintf(msg, "共替换 %d 处", (int)hits.size());
  MessageBoxA(GetHWnd(), msg, "替换选中", MB_OK | MB_ICONINFORMATION);
  // 刷新面板
  FindAllMatches();
  // 替换后所有匹配应该重新全选
  g_findResultChecked.assign(g_findResults.size(), true);
  if (g_findResults.empty())
    g_findResultOpen = false;
}

// 查找弹窗命中测试
bool HitFindBoxClose(int mx, int my) { // 右上角 ×
  if (!g_findBoxOpen)
    return false;
  int bx = g_findBoxX + FIND_BOX_W - 22, by = g_findBoxY + 4;
  return mx >= bx && mx < bx + 18 && my >= by && my < by + 18;
}
bool HitFindBoxBtn(int mx, int my) { // 开始查找按钮
  if (!g_findBoxOpen)
    return false;
  int bw = 84, bh = 20;
  int bx = g_findBoxX + FIND_BOX_W - 10 - bw, by = g_findBoxY + FIND_BOX_H - 30;
  return mx >= bx && mx < bx + bw && my >= by && my < by + bh;
}
bool PtInFindBox(int mx, int my) { // 弹窗整体范围
  return g_findBoxOpen && mx >= g_findBoxX && mx < g_findBoxX + FIND_BOX_W &&
         my >= g_findBoxY && my < g_findBoxY + FIND_BOX_H;
}

bool HitFindBoxInput(int mx, int my) { // 查找输入框（点击定位/拖选文本）
  int ix = g_findBoxX + 10, iy = g_findBoxY + 28;
  return g_findBoxOpen && mx >= ix && mx < ix + (FIND_BOX_W - 20) && my >= iy &&
         my < iy + 20;
}

// 把弹窗位置夹紧在窗口范围内（拖动后 / 窗口缩放后仍完整可见）
void ClampFindBoxPos() {
  if (g_findBoxX < 0)
    g_findBoxX = 0;
  if (g_findBoxX > WINDOW_WIDTH - FIND_BOX_W)
    g_findBoxX = WINDOW_WIDTH - FIND_BOX_W;
  if (g_findBoxY < MENU_BAR_HEIGHT)
    g_findBoxY = MENU_BAR_HEIGHT;
  if (g_findBoxY > WINDOW_HEIGHT - STATUS_BAR_HEIGHT - FIND_BOX_H)
    g_findBoxY = WINDOW_HEIGHT - STATUS_BAR_HEIGHT - FIND_BOX_H;
}

// ============================================================
// 绘制：查找与替换弹窗（标题行 + × + 查找框 + 替换框 + 三按钮）
// ============================================================

// 单个输入框绘制：底/框 + 占位提示 + 逐字符选区（蓝底白字）+ 闪烁光标。
// 查找框与替换框共用；只有键盘焦点字段显示光标与高亮边框。
static void DrawFindField(int ix, int iy, int iw, int ih,
                          const std::string &text, const char *hint,
                          bool focused) {
  setfillcolor(g_theme.editBg);
  bar(ix, iy, ix + iw, iy + ih);
  setlinecolor(focused ? g_theme.highlight : g_theme.dropBorder);
  rectangle(ix, iy, ix + iw, iy + ih);
  setbkcolor(g_theme.editBg);

  int n = (int)text.size();
  int hidden = FindBoxHiddenBytes(text, iw); // 左侧滚出的字节数
  int s = g_findSelAnchor, e = g_findCursor; // 选区 [s, e)（仅焦点字段）
  if (s > e) {
    int t = s;
    s = e;
    e = t;
  }

  if (n == 0) {
    settextcolor(g_theme.diagMuted);
    OutText(ix + 5, iy + 3, hint);
  } else {
    // 逐字符绘制：选区内蓝底白字（与编辑区选区样式一致）
    int cx = ix + 5;
    for (int i = hidden; i < n;) {
      unsigned char chb = (unsigned char)text[i];
      int step = (chb >= 0x81 && i + 1 < n) ? 2 : 1;
      std::string one = text.substr(i, step);
      int w = textwidth(one.c_str());
      bool inSel = (focused && s >= 0 && i >= s && i + step <= e);
      if (inSel) {
        setfillcolor(g_theme.selBg);
        bar(cx, iy + 2, cx + w, iy + ih - 2);
        settextcolor(g_theme.selText);
      } else {
        settextcolor(g_theme.editText);
      }
      OutText(cx, iy + 3, one);
      cx += w;
      i += step;
    }
  }

  // 闪烁光标（仅焦点字段，与编辑区同 500ms 相位）
  if (focused && (GetTickCount() / 500) % 2 == 0) {
    int curX = 0;
    if (g_findCursor > hidden)
      curX = textwidth(text.substr(hidden, g_findCursor - hidden).c_str());
    if (curX > iw - 7)
      curX = iw - 7;
    int cpx = ix + 5 + curX;
    setfillcolor(g_theme.editText);
    bar(cpx, iy + 3, cpx + 1, iy + ih - 3);
  }
}

void DrawFindBox() {
  if (!g_findBoxOpen)
    return;

  // 位置由打开时的居中值 + 用户拖动决定；夹紧保证窗口缩放后仍可见
  ClampFindBoxPos();
  int x = g_findBoxX, y = g_findBoxY;
  int x2 = x + FIND_BOX_W, y2 = y + FIND_BOX_H;

  // 面板背景 + 边框
  setfillcolor(g_theme.dropBg);
  setlinecolor(g_theme.dropBorder);
  bar(x, y, x2, y2);
  rectangle(x, y, x2, y2);

  settextstyle(14, 0, _T("宋体"));

  // 标题
  setbkcolor(g_theme.dropBg);
  settextcolor(g_theme.dropText);
  OutText(x + 10, y + 5, "查找与替换");

  // 右上角 × 关闭按钮（悬停时底色变红）
  {
    int bx = x2 - 22, by = y + 4, bs = 18;
    setfillcolor(g_findBoxHoverClose ? g_theme.errLineBg : g_theme.dropBg);
    bar(bx, by, bx + bs, by + bs);
    setbkcolor(g_findBoxHoverClose ? g_theme.errLineBg : g_theme.dropBg);
    settextcolor(g_findBoxHoverClose ? g_theme.errLineNumText
                                     : g_theme.dropText);
    OutText(bx + 5, by + 1, "×");
  }

  // 查找输入框（y+28）
  DrawFindField(x + 10, y + 28, FIND_BOX_W - 20, 20, g_findInput,
                "输入关键词（区分大小写）", true);

  // "开始查找"按钮（右对齐）
  {
    int bw = 100, bh = 20, by = y + FIND_BOX_H - 30;
    int bx = x + FIND_BOX_W - 10 - bw;
    COLORREF bgc = g_findBoxHoverBtn ? g_theme.highlight : g_theme.menuBarHover;
    setfillcolor(bgc);
    bar(bx, by, bx + bw, by + bh);
    setbkcolor(bgc);
    settextcolor(g_theme.dropText);
    OutText(bx + 10, by + 3, "开始查找");
  }
}

// ============================================================
// 绘制：查找结果独立弹窗（标题栏 + × 关闭 + 列表 + 底部状态栏）
// ============================================================

static void GetFindResultRect(int &rx, int &ry, int &rw, int &rh) {
  rx = g_findResultX;
  ry = g_findResultY;
  rw = FIND_RESULT_W;
  if (FIND_RESULT_H == 0) {
    int n = (int)g_findResults.size();
    int visN = n < FIND_RESULT_MAX_VISIBLE ? n : FIND_RESULT_MAX_VISIBLE;
    rh = FIND_RESULT_HEADER_H + FIND_RESULT_REPL_H + FIND_RESULT_BTN_H +
         visN * FIND_RESULT_ITEM_H;
  } else {
    rh = FIND_RESULT_H;
  }
}

// 关闭按钮命中（右上角）
bool HitFindResultClose(int mx, int my) {
  if (!g_findResultOpen) return false;
  int rx, ry, rw, rh; GetFindResultRect(rx, ry, rw, rh);
  int cx = rx + rw - 22, cy = ry + 5, cs = 18;
  return mx >= cx && mx < cx + cs && my >= cy && my < cy + cs;
}

// 标题栏命中（用于拖动）
bool HitFindResultTitle(int mx, int my) {
  if (!g_findResultOpen) return false;
  int rx, ry, rw, rh; GetFindResultRect(rx, ry, rw, rh);
  // 标题栏 = 除关闭按钮外的顶部 FIND_RESULT_HEADER_H
  int cx = rx + rw - 22, cy = ry + 5, cs = 18;
  bool isClose = mx >= cx && mx < cx + cs && my >= cy && my < cy + cs;
  return !isClose && mx >= rx && mx < rx + rw &&
         my >= ry && my < ry + FIND_RESULT_HEADER_H;
}

// 位置夹紧（拖动后/窗口缩放后保持可见）
static void ClampFindResultPos() {
  if (g_findResultX < 0) g_findResultX = 0;
  if (g_findResultX > WINDOW_WIDTH - FIND_RESULT_W)
    g_findResultX = WINDOW_WIDTH - FIND_RESULT_W;
  if (g_findResultY < MENU_BAR_HEIGHT)
    g_findResultY = MENU_BAR_HEIGHT;
  int rh = FIND_RESULT_H > 0 ? FIND_RESULT_H : 200;
  if (g_findResultY > WINDOW_HEIGHT - STATUS_BAR_HEIGHT - rh)
    g_findResultY = WINDOW_HEIGHT - STATUS_BAR_HEIGHT - rh;
}

// —— 辅助：计算替换输入框左侧滚动出的字节数 + 光标 x ——
static int FindRepHidden() {
  const std::string &s = g_findRepInput;
  int iw = FIND_RESULT_W - 100; // 输入框宽度
  int hidden = 0;
  while (hidden < (int)s.size()) {
    int w = textwidth(s.substr(hidden, 1).c_str());
    if (w == 0) break;
    if (textwidth(s.substr(hidden).c_str()) <= iw) break;
    hidden += 1;
  }
  return hidden;
}

void DrawFindResults() {
  if (!g_findResultOpen)
    return;

  int rx, ry, rw, rh;
  GetFindResultRect(rx, ry, rw, rh);
  ClampFindResultPos();
  rx = g_findResultX; ry = g_findResultY;

  int rx2 = rx + rw, ry2 = ry + rh;

  // 面板背景 + 边框
  setfillcolor(g_theme.dropBg);
  setlinecolor(g_theme.dropBorder);
  bar(rx, ry, rx2, ry2);
  rectangle(rx, ry, rx2, ry2);

  settextstyle(14, 0, _T("宋体"));

  // —— 标题栏 ——
  setfillcolor(g_theme.menuBarHover);
  bar(rx + 1, ry + 1, rx2 - 1, ry + FIND_RESULT_HEADER_H - 1);
  setbkcolor(g_theme.menuBarHover);
  settextcolor(g_theme.dropText);
  char title[64];
  sprintf(title, "查找结果（%d）", (int)g_findResults.size());
  OutText(rx + 10, ry + 6, title);

  // × 关闭按钮
  {
    int bx = rx + rw - 22, by = ry + 5, bs = 18;
    COLORREF closeBg = g_findResultHoverClose ? g_theme.errLineBg : g_theme.menuBarHover;
    setfillcolor(closeBg);
    bar(bx, by, bx + bs, by + bs);
    setbkcolor(closeBg);
    settextcolor(g_findResultHoverClose ? g_theme.errLineNumText : g_theme.dropText);
    OutText(bx + 5, by + 1, "×");
  }
  setlinecolor(g_theme.dropBorder);
  line(rx, ry + FIND_RESULT_HEADER_H, rx2, ry + FIND_RESULT_HEADER_H);

  // —— "替换为"输入框行 ——
  {
    int rowY = ry + FIND_RESULT_HEADER_H;
    // label
    setbkcolor(g_theme.dropBg);
    settextcolor(g_theme.diagMuted);
    OutText(rx + 8, rowY + 8, "替换为");
    // 输入框矩形
    int ix = rx + 55, iy = rowY + 6, iw = FIND_RESULT_W - 100, ih = 20;
    COLORREF rectBg = g_findRepFocused ? g_theme.editBg : g_theme.editBg;
    setfillcolor(rectBg);
    bar(ix, iy, ix + iw, iy + ih);
    setlinecolor(g_findRepFocused ? g_theme.highlight : g_theme.dropBorder);
    rectangle(ix, iy, ix + iw, iy + ih);
    // 文本
    setbkcolor(rectBg);
    settextcolor(g_theme.editText);
    int hidden = FindRepHidden();
    OutText(ix + 4, iy + 3, g_findRepInput.substr(hidden).c_str());
    // 光标（与查找框共用 g_findCursor：键盘编辑函数只维护这一份）
    if (g_findRepFocused) {
      int cur = g_findCursor;
      if (cur < hidden) cur = hidden;
      if (cur > (int)g_findRepInput.size()) cur = (int)g_findRepInput.size();
      int curX = ix + 4 + textwidth(g_findRepInput.substr(hidden, cur - hidden).c_str());
      if (curX < ix + 4) curX = ix + 4;
      setlinecolor(g_theme.dropText);
      line(curX, iy + 2, curX, iy + ih - 2);
    }
  }
  setlinecolor(g_theme.dropBorder);
  line(rx, ry + FIND_RESULT_HEADER_H + FIND_RESULT_REPL_H - 1,
       rx2, ry + FIND_RESULT_HEADER_H + FIND_RESULT_REPL_H - 1);

  // —— 列表区域 ——
  int listTop = ry + FIND_RESULT_HEADER_H + FIND_RESULT_REPL_H;
  int listBottom = ry2 - FIND_RESULT_BTN_H;
  int listH = listBottom - listTop;
  int visN = listH / FIND_RESULT_ITEM_H;
  int total = (int)g_findResults.size();
  int scroll = g_findResultScroll;
  int maxScroll = total - visN;
  if (maxScroll < 0) maxScroll = 0;
  if (scroll < 0) scroll = 0;
  if (scroll > maxScroll) scroll = maxScroll;
  g_findResultScroll = scroll;

  setfillcolor(g_theme.dropBg);
  setbkcolor(g_theme.dropBg);
  settextstyle(14, 0, _T("宋体"));

  if (g_findResults.empty()) {
    settextcolor(g_theme.diagMuted);
    OutText(rx + 10, listTop + 8, "（无匹配）");
  }

  for (int i = 0; i < visN; i++) {
    int idx = scroll + i;
    if (idx >= total) break;
    int iy = listTop + i * FIND_RESULT_ITEM_H;

    bool hover = (idx == g_findResultHover);
    if (hover) {
      setfillcolor(g_theme.menuBarHover);
      bar(rx + 1, iy, rx2 - 1, iy + FIND_RESULT_ITEM_H);
    }

    // 复选框（最左）
    int cbX = rx + 6, cbY = iy + 2, cbS = 13;
    setfillcolor(g_theme.editBg);
    setlinecolor(g_theme.dropBorder);
    bar(cbX, cbY, cbX + cbS, cbY + cbS);
    rectangle(cbX, cbY, cbX + cbS, cbY + cbS);
    if (idx < (int)g_findResultChecked.size() && g_findResultChecked[idx]) {
      setlinecolor(RGB(30, 90, 200)); // 深蓝勾线
      setlinestyle(PS_SOLID, 2);       // 加粗更清晰
      line(cbX + 2, cbY + 6, cbX + 6, cbY + cbS - 3);
      line(cbX + 6, cbY + cbS - 3, cbX + cbS - 2, cbY + 2);
      setlinestyle(PS_SOLID, 1);
    }

    // 行:列
    char loc[32];
    sprintf(loc, "%d:%d", g_findResults[idx].row + 1,
            g_findResults[idx].col + 1);
    settextcolor(g_theme.diagMuted);
    setbkcolor(hover ? g_theme.menuBarHover : g_theme.dropBg);
    OutText(cbX + cbS + 6, iy + 1, loc);

    // 匹配文本
    settextcolor(g_theme.editText);
    setbkcolor(hover ? g_theme.menuBarHover : g_theme.dropBg);
    int textX = cbX + cbS + 6 + 60; // loc 约 60 像素
    int maxTextW = rx2 - textX - 16;
    const std::string &txt = g_findResults[idx].text;
    std::string display = txt;
    if (textwidth(display.c_str()) > maxTextW) {
      int cutLen = (int)display.size();
      while (cutLen > 0 && textwidth(display.substr(0, cutLen).c_str()) > maxTextW)
        cutLen--;
      display = display.substr(0, cutLen);
    }
    OutText(textX, iy + 1, display);
  }

  // 滚动条
  if (total > visN) {
    int sbX = rx2 - 8;
    int sbTop = listTop;
    int sbH = listH;
    setfillcolor(g_theme.lineNumBg);
    bar(sbX, sbTop, sbX + 6, sbTop + sbH);
    int thumbH = (visN * sbH) / total;
    if (thumbH < 14) thumbH = 14;
    int thumbY = sbTop + (scroll * (sbH - thumbH)) / maxScroll;
    setfillcolor(g_theme.scrollThumb);
    bar(sbX, thumbY, sbX + 6, thumbY + thumbH);
  }

  // —— 底部按钮区 ——
  int btnY = ry2 - FIND_RESULT_BTN_H + 5;
  setlinecolor(g_theme.dropBorder);
  line(rx, listBottom, rx2, listBottom);

  // "全选"小按钮（左对齐）
  {
    int bx = rx + 6;
    int bw = 50, bh = 20;
    COLORREF bgc = g_theme.menuBarHover;
    setfillcolor(bgc);
    bar(bx, btnY, bx + bw, btnY + bh);
    setbkcolor(bgc);
    settextcolor(g_theme.dropText);
    OutText(bx + 12, btnY + 3, "全选");
    bx += bw + 4;
    bw = 72;
    setfillcolor(g_theme.menuBarHover);
    bar(bx, btnY, bx + bw, btnY + bh);
    setbkcolor(g_theme.menuBarHover);
    settextcolor(g_theme.dropText);
    OutText(bx + 10, btnY + 3, "取消全选");
  }

  // "替换选中项" + "全部替换"（右对齐）
  {
    int bw2 = 84, bw1 = 84, bh = 20;
    int bx2 = rx2 - 10 - bw2;
    int bx1 = bx2 - 6 - bw1;
    COLORREF bgc1 = g_theme.menuBarHover;
    COLORREF bgc2 = RGB(30, 90, 200); // "全部替换" 深蓝色
    setfillcolor(bgc1);
    bar(bx1, btnY, bx1 + bw1, btnY + bh);
    setbkcolor(bgc1);
    settextcolor(g_theme.dropText);
    OutText(bx1 + 10, btnY + 3, "替换选中");
    setfillcolor(bgc2);
    bar(bx2, btnY, bx2 + bw2, btnY + bh);
    setbkcolor(bgc2);
    settextcolor(WHITE);
    OutText(bx2 + 10, btnY + 3, "全部替换");
  }
}

// 命中测试：返回点击的列表项索引（-1=未命中列表）
int HitTestFindResults(int mx, int my) {
  if (!g_findResultOpen || g_findResults.empty())
    return -1;
  int rx, ry, rw, rh;
  GetFindResultRect(rx, ry, rw, rh);
  int listTop = ry + FIND_RESULT_HEADER_H + FIND_RESULT_REPL_H;
  int listBottom = ry + rh - FIND_RESULT_BTN_H;
  if (mx < rx || mx >= rx + rw || my < listTop || my >= listBottom)
    return -1;
  int idx = g_findResultScroll + (my - listTop) / FIND_RESULT_ITEM_H;
  if (idx < 0 || idx >= (int)g_findResults.size())
    return -1;
  return idx;
}

// 鼠标是否在结果弹窗范围内（用于滚轮事件拦截）
bool PtInFindResults(int mx, int my) {
  if (!g_findResultOpen)
    return false;
  int rx, ry, rw, rh;
  GetFindResultRect(rx, ry, rw, rh);
  return mx >= rx && mx < rx + rw && my >= ry && my < ry + rh;
}

// —— 结果弹窗内各控件的命中测试 ——

// "替换为"输入框
bool HitFindResultRepInput(int mx, int my) {
  if (!g_findResultOpen) return false;
  int rx, ry, rw, rh; GetFindResultRect(rx, ry, rw, rh);
  int rowY = ry + FIND_RESULT_HEADER_H;
  int ix = rx + 55, iy = rowY + 6, iw = FIND_RESULT_W - 100, ih = 20;
  return mx >= ix && mx < ix + iw && my >= iy && my < iy + ih;
}

// 某一行的复选框
bool HitFindResultCheckBox(int mx, int my, int &outIdx) {
  outIdx = -1;
  if (!g_findResultOpen || g_findResults.empty()) return false;
  int rx, ry, rw, rh; GetFindResultRect(rx, ry, rw, rh);
  int listTop = ry + FIND_RESULT_HEADER_H + FIND_RESULT_REPL_H;
  int listBottom = ry + rh - FIND_RESULT_BTN_H;
  if (my < listTop || my >= listBottom) return false;
  int idx = g_findResultScroll + (my - listTop) / FIND_RESULT_ITEM_H;
  if (idx < 0 || idx >= (int)g_findResults.size()) return false;
  int iy = listTop + (idx - g_findResultScroll) * FIND_RESULT_ITEM_H;
  int cbX = rx + 6, cbY = iy + 2, cbS = 13;
  if (mx >= cbX && mx < cbX + cbS && my >= cbY && my < cbY + cbS) {
    outIdx = idx;
    return true;
  }
  return false;
}

// 底部按钮：返回按钮类型
enum ResultBtn { RB_NONE = 0, RB_SEL_ALL, RB_UNSEL_ALL, RB_REPLACE_SEL, RB_REPLACE_ALL };
static ResultBtn HitFindResultBottomBtn(int mx, int my) {
  if (!g_findResultOpen) return RB_NONE;
  int rx, ry, rw, rh; GetFindResultRect(rx, ry, rw, rh);
  int btnY = ry + rh - FIND_RESULT_BTN_H + 5;
  int bh = 20;
  // 左对齐：全选 / 取消全选
  {
    int bx = rx + 6;
    int bw = 50;
    if (mx >= bx && mx < bx + bw && my >= btnY && my < btnY + bh) return RB_SEL_ALL;
    bx += bw + 4; bw = 56;
    if (mx >= bx && mx < bx + bw && my >= btnY && my < btnY + bh) return RB_UNSEL_ALL;
  }
  // 右对齐：替换选中 / 全部替换
  {
    int bw2 = 84, bw1 = 84;
    int bx2 = rx + rw - 10 - bw2;
    int bx1 = bx2 - 6 - bw1;
    if (mx >= bx1 && mx < bx1 + bw1 && my >= btnY && my < btnY + bh) return RB_REPLACE_SEL;
    if (mx >= bx2 && mx < bx2 + bw2 && my >= btnY && my < btnY + bh) return RB_REPLACE_ALL;
  }
  return RB_NONE;
}

// ============================================================
// 函数定义扫描
// ============================================================
struct FuncInfo {
  std::string name;
  int line; // 0-based 文档行号
};

// 判断一个标识符是否是 C 关键字（用于排除 if/for/while 等）
static bool IsCKeyword2(const char *s, int len) {
  static const char *kw[] = {
      "if",        "else",     "for",    "while",    "do",     "switch",
      "case",      "default",  "break",  "continue", "return", "sizeof",
      "typedef",   "struct",   "union",  "enum",     "static", "extern",
      "auto",      "register", "const",  "volatile", "inline", "restrict",
      "goto",      "void",     "char",   "short",    "int",    "long",
      "float",     "double",   "signed", "unsigned", "_Bool",  "_Complex",
      "_Imaginary"};
  for (size_t k = 0; k < sizeof(kw) / sizeof(kw[0]); k++) {
    if ((int)strlen(kw[k]) == len && strncmp(s, kw[k], len) == 0)
      return true;
  }
  return false;
}

// 从字符串 s 的 pos 位置开始，提取以标识符字符开头的单词，返回结束位置
static int ReadIdent(const char *s, int pos, int len, std::string &out) {
  out.clear();
  while (pos < len && (isalnum((unsigned char)s[pos]) || s[pos] == '_')) {
    out += s[pos];
    pos++;
  }
  return pos;
}

// 查找一行中与左括号 ( 匹配的右括号 ) 位置，跳过字符串/字符/嵌套括号
// 返回右括号下标，找不到返回 -1
static int FindMatchingParen(const std::string &s, int openPos) {
  int depth = 0;
  size_t n = s.size();
  for (size_t i = openPos; i < n; i++) {
    char c = s[i];
    if (c == '"' || c == '\'') {
      char q = c;
      i++;
      while (i < n && s[i] != q) {
        if (s[i] == '\\' && i + 1 < n)
          i++;
        i++;
      }
      continue;
    }
    if (c == '(')
      depth++;
    else if (c == ')') {
      depth--;
      if (depth == 0)
        return (int)i;
    }
  }
  return -1;
}

// 扫描当前缓冲区，收集所有疑似函数定义（name + 行号）
// 启发式：行内有 "标识符("，标识符非关键字；匹配的 ")" 之后要么同行有 "{"，
// 要么后续若干非空行以 "{" 开头（且不先遇到 ";"）。
std::vector<FuncInfo> FindFunctions() {
  std::vector<FuncInfo> result;
  int rows = g_buffer.getRowCount();
  bool inBlock = false; // 是否在 /* */ 块注释中

  for (int r = 0; r < rows; r++) {
    std::string line = g_buffer.getLine(r);

    // 块注释状态推进（粗略：只看本行是否出现 */）
    if (inBlock) {
      size_t e = line.find("*/");
      if (e == std::string::npos)
        continue;
      line = line.substr(e + 2);
      inBlock = false;
    }

    // 去除 // 行注释（不处理字符串内的 //，足够课程作业用）
    size_t cc = line.find("//");
    if (cc != std::string::npos)
      line = line.substr(0, cc);

    // 检查是否开始新的块注释
    size_t bc = line.find("/*");
    if (bc != std::string::npos) {
      inBlock = true;
      line = line.substr(0, bc);
    }

    // 跳过空行、预处理指令
    std::string trimmed = line;
    size_t firstNon = trimmed.find_first_not_of(" \t");
    if (firstNon == std::string::npos)
      continue;
    if (trimmed[firstNon] == '#')
      continue;

    // 在行内查找 "标识符(" 的模式
    int len = (int)line.size();
    for (int i = 0; i < len; i++) {
      char c = line[i];
      // 标识符起始
      if (isalpha((unsigned char)c) || c == '_') {
        int start = i;
        std::string ident;
        i = ReadIdent(line.c_str(), i, len, ident);
        // 紧跟 (  ？
        if (i < len && line[i] == '(') {
          if (!IsCKeyword2(ident.c_str(), (int)ident.size()) &&
              !ident.empty()) {
            int closeParen = FindMatchingParen(line, i);
            if (closeParen > 0) {
              // 检查右括号之后的内容
              std::string after = line.substr(closeParen + 1);
              size_t af = after.find_first_not_of(" \t");
              bool isDef = false;
              if (af != std::string::npos && after[af] == '{') {
                isDef = true;
              } else if (af == std::string::npos || after[af] != ';') {
                // 同行无 ; 也无 {：看下一个非空行是否以 { 开头
                for (int nr = r + 1; nr < rows && nr < r + 4; nr++) {
                  std::string nl = g_buffer.getLine(nr);
                  size_t nf = nl.find_first_not_of(" \t");
                  if (nf == std::string::npos)
                    continue;
                  if (nl[nf] == '{') {
                    isDef = true;
                    break;
                  }
                  if (nl[nf] == ';') {
                    isDef = false;
                    break;
                  }
                  // 其他内容（如继续的参数列表），继续找
                }
              }
              if (isDef) {
                FuncInfo fi;
                fi.name = ident;
                fi.line = r;
                result.push_back(fi);
              }
            }
          }
        }
      }
    }
  }
  return result;
}

// ============================================================
// 函数列表弹窗状态（仿右键菜单的自绘列表）
// ============================================================
bool g_funcListOpen = false;
int g_funcListX = 0;
int g_funcListY = 0;
int g_funcListHover = -1;
std::vector<FuncInfo> g_funcList; // 当前弹窗列出的函数
const int FUNC_ITEM_H = 22;
const int FUNC_LIST_W = 260;

// 打开函数列表弹窗
void GoToFunction() {
  g_funcList = FindFunctions();
  if (g_funcList.empty()) {
    MessageBoxA(GetHWnd(), "未在当前文件中识别到函数定义。", "转到函数",
                MB_OK | MB_ICONINFORMATION);
    return;
  }
  // 弹窗位置：编辑区左上角附近
  g_funcListX = TEXT_ORIGIN_X;
  g_funcListY = TEXT_ORIGIN_Y;
  g_funcListHover = -1;
  g_funcListOpen = true;
}

// 命中函数列表项；命中返回 true 并填 idx，未命中返回 false
bool HitTestFuncList(int mx, int my, int &idx) {
  if (!g_funcListOpen)
    return false;
  if (mx < g_funcListX || mx >= g_funcListX + FUNC_LIST_W)
    return false;
  int h = (int)g_funcList.size() * FUNC_ITEM_H;
  if (my < g_funcListY || my >= g_funcListY + h)
    return false;
  idx = (my - g_funcListY) / FUNC_ITEM_H;
  return idx >= 0 && idx < (int)g_funcList.size();
}

// ============================================================
// 变量监视窗口：数据结构 + 全局变量
// ============================================================
struct VarSnapshot {
  int bpRow;          // 断点行（1-based）
  std::string varName;
  std::string type;   // 变量类型（int/char/float/double 等）
  std::string value;  // 变量值（字符串形式）
  bool assigned;      // 是否已赋值（false → 显示"暂未赋值"）
};

std::vector<VarSnapshot> g_varSnapshots; // 解析后的变量快照
std::string g_programOutput;             // 调试运行时程序的 stdout 输出
bool g_watchWindowOpen = false;          // 变量监视窗口是否打开
int g_watchX = 0, g_watchY = 0;         // 窗口位置
int g_watchScroll = 0;                   // 滚动偏移
int g_watchHover = -1;                   // 列表项悬停
bool g_watchHoverClose = false;          // 关闭按钮悬停
bool g_watchDragging = false;            // 拖动中
int g_watchDragOffX = 0, g_watchDragOffY = 0;
char g_watchInputBuf[256] = "";          // 变量名输入框
int g_watchInputLen = 0;
bool g_watchInputActive = false;         // 输入框是否有焦点
int g_watchInputCursorPos = 0;           // 光标位置
std::string g_watchFilter;               // 当前过滤的变量名
bool g_watchHoverRefresh = false;        // 刷新按钮悬停
bool g_watchHoverQuery = false;          // 查看按钮悬停

const int WATCH_W = 420;
const int WATCH_TITLE_H = 28;
const int WATCH_INPUT_H = 36;
const int WATCH_BOTTOM_H = 32;
const int WATCH_ITEM_H = 22;
const int WATCH_MAX_OUTPUT_LINES = 5; // 程序输出最多显示行数

// 计算程序输出占用的高度（含分隔线）
static int GetOutputSectionHeight() {
  if (g_programOutput.empty()) return 0;
  int lines = 1; // 标题行
  for (size_t i = 0; i < g_programOutput.size(); i++) {
    if (g_programOutput[i] == '\n') lines++;
  }
  if (lines > WATCH_MAX_OUTPUT_LINES + 1) lines = WATCH_MAX_OUTPUT_LINES + 1;
  return lines * WATCH_ITEM_H + 4; // +4 为分隔间距
}
const int WATCH_MAX_VISIBLE = 10;

// 前向声明
std::string ReadFileAll(const char *path);
void ParseDebugOutput();
std::vector<int> GetFilteredSnapshots();

// ============================================================
// 变量监视窗口：绘制 + 命中测试
// ============================================================
static void GetWatchRect(int &rx, int &ry, int &rw, int &rh) {
  rx = g_watchX;
  ry = g_watchY;
  rw = WATCH_W;
  int n = GetFilteredSnapshots().size();
  int visRows = (n < WATCH_MAX_VISIBLE) ? n : WATCH_MAX_VISIBLE;
  int outH = GetOutputSectionHeight();
  rh = WATCH_TITLE_H + WATCH_INPUT_H + outH + visRows * WATCH_ITEM_H + WATCH_BOTTOM_H;
  if (rh < WATCH_TITLE_H + WATCH_INPUT_H + WATCH_BOTTOM_H)
    rh = WATCH_TITLE_H + WATCH_INPUT_H + WATCH_BOTTOM_H;
}

static void ClampWatchPos() {
  int rh;
  GetWatchRect(g_watchX, g_watchY, *(int*)0, rh);
  if (g_watchX < 0) g_watchX = 0;
  if (g_watchX > WINDOW_WIDTH - WATCH_W)
    g_watchX = WINDOW_WIDTH - WATCH_W;
  if (g_watchY < MENU_BAR_HEIGHT)
    g_watchY = MENU_BAR_HEIGHT;
  int dummy;
  GetWatchRect(g_watchX, g_watchY, dummy, rh);
  if (g_watchY > WINDOW_HEIGHT - STATUS_BAR_HEIGHT - rh)
    g_watchY = WINDOW_HEIGHT - STATUS_BAR_HEIGHT - rh;
}

bool PtInWatchWindow(int mx, int my) {
  if (!g_watchWindowOpen) return false;
  int rx, ry, rw, rh;
  GetWatchRect(rx, ry, rw, rh);
  return mx >= rx && mx < rx + rw && my >= ry && my < ry + rh;
}

bool HitWatchClose(int mx, int my) {
  if (!g_watchWindowOpen) return false;
  int rx, ry, rw, rh;
  GetWatchRect(rx, ry, rw, rh);
  int cx = rx + rw - 24;
  return mx >= cx && mx < cx + 20 && my >= ry + 4 && my < ry + 24;
}

bool HitWatchTitle(int mx, int my) {
  if (!g_watchWindowOpen) return false;
  int rx, ry, rw, rh;
  GetWatchRect(rx, ry, rw, rh);
  return mx >= rx && mx < rx + rw - 24 && my >= ry && my < ry + WATCH_TITLE_H;
}

bool HitWatchInput(int mx, int my) {
  if (!g_watchWindowOpen) return false;
  int rx, ry, rw, rh;
  GetWatchRect(rx, ry, rw, rh);
  int iy = ry + WATCH_TITLE_H + 6;
  int ih = WATCH_INPUT_H - 12;
  int ix = rx + 60;
  int iw = rw - 60 - 80;
  return mx >= ix && mx < ix + iw && my >= iy && my < iy + ih;
}

bool HitWatchQueryBtn(int mx, int my) {
  if (!g_watchWindowOpen) return false;
  int rx, ry, rw, rh;
  GetWatchRect(rx, ry, rw, rh);
  int by = ry + WATCH_TITLE_H + 6;
  int bh = WATCH_INPUT_H - 12;
  int bx = rx + rw - 72;
  return mx >= bx && mx < bx + 64 && my >= by && my < by + bh;
}

bool HitWatchRefreshBtn(int mx, int my) {
  if (!g_watchWindowOpen) return false;
  int rx, ry, rw, rh;
  GetWatchRect(rx, ry, rw, rh);
  int by = ry + rh - WATCH_BOTTOM_H + 4;
  int bh = WATCH_BOTTOM_H - 8;
  int bx = rx + rw - 72;
  return mx >= bx && mx < bx + 64 && my >= by && my < by + bh;
}

int HitWatchList(int mx, int my) {
  if (!g_watchWindowOpen) return -1;
  int rx, ry, rw, rh;
  GetWatchRect(rx, ry, rw, rh);
  int listTop = ry + WATCH_TITLE_H + WATCH_INPUT_H + GetOutputSectionHeight();
  int listBot = ry + rh - WATCH_BOTTOM_H;
  if (my < listTop || my >= listBot)
    return -1;
  std::vector<int> filtered = GetFilteredSnapshots();
  int idx = g_watchScroll + (my - listTop) / WATCH_ITEM_H;
  if (idx < 0 || idx >= (int)filtered.size())
    return -1;
  return idx;
}

void DrawWatchWindow() {
  if (!g_watchWindowOpen) return;

  ClampWatchPos();
  int rx = g_watchX, ry = g_watchY, rw, rh;
  GetWatchRect(rx, ry, rw, rh);

  // 阴影
  setfillcolor(RGB(0, 0, 0));
  setfillstyle(BS_SOLID);
  bar(rx + 3, ry + 3, rx + rw + 3, ry + rh + 3);

  // 主体背景
  setfillcolor(g_theme.editBg);
  bar(rx, ry, rx + rw, ry + rh);

  // 标题栏
  setfillcolor(g_theme.menuBarHover);
  bar(rx, ry, rx + rw, ry + WATCH_TITLE_H);
  setbkcolor(g_theme.menuBarHover);
  settextcolor(g_theme.dropText);
  OutText(rx + 10, ry + 6, "变量监视");
  // 关闭按钮
  COLORREF closeBg = g_watchHoverClose ? g_theme.errLineBg : g_theme.menuBarHover;
  setfillcolor(closeBg);
  bar(rx + rw - 24, ry + 4, rx + rw - 4, ry + 24);
  setbkcolor(closeBg);
  settextcolor(g_watchHoverClose ? RGB(255, 255, 255) : g_theme.dropText);
  OutText(rx + rw - 18, ry + 8, "x");

  // 输入行
  int iy = ry + WATCH_TITLE_H + 6;
  int ih = WATCH_INPUT_H - 12;
  int ix = rx + 60;
  int iw = rw - 60 - 80;

  settextcolor(g_theme.dropText);
  setbkcolor(g_theme.editBg);
  OutText(rx + 8, iy + 4, "变量名:");

  // 输入框背景
  setfillcolor(g_watchInputActive ? g_theme.editBg : g_theme.lineNumBg);
  bar(ix, iy, ix + iw, iy + ih);
  setlinecolor(g_watchInputActive ? g_theme.highlight : g_theme.lineNumSep);
  rectangle(ix - 1, iy - 1, ix + iw + 1, iy + ih + 1);

  // 输入框文字
  setbkcolor(g_theme.editBg);
  settextcolor(g_theme.editText);
  if (g_watchInputLen > 0) {
    OutText(ix + 4, iy + 3, g_watchInputBuf);
  }
  // 光标
  if (g_watchInputActive) {
    int cx = ix + 4 + g_watchInputCursorPos * CHAR_WIDTH;
    setlinecolor(g_theme.editText);
    line(cx, iy + 2, cx, iy + ih - 2);
  }

  // "查看" 按钮
  int qbx = rx + rw - 72;
  setfillcolor(g_watchHoverQuery ? RGB(30, 90, 200) : g_theme.menuBarHover);
  bar(qbx, iy, qbx + 64, iy + ih);
  setbkcolor(g_watchHoverQuery ? RGB(30, 90, 200) : g_theme.menuBarHover);
  settextcolor(g_watchHoverQuery ? WHITE : g_theme.dropText);
  OutText(qbx + 16, iy + 3, "查看");

  // 程序输出区
  int outTop = ry + WATCH_TITLE_H + WATCH_INPUT_H;
  int outH = GetOutputSectionHeight();
  if (outH > 0) {
    // 标题
    setbkcolor(g_theme.editBg);
    settextcolor(RGB(30, 130, 30));
    OutText(rx + 8, outTop + 2, "--- 程序输出 ---");
    // 输出内容（最多 WATCH_MAX_OUTPUT_LINES 行）
    settextcolor(g_theme.editText);
    int lineY = outTop + WATCH_ITEM_H;
    size_t pos = 0;
    int shown = 0;
    while (pos < g_programOutput.size() && shown < WATCH_MAX_OUTPUT_LINES) {
      size_t nl = g_programOutput.find('\n', pos);
      std::string one;
      if (nl == std::string::npos) {
        one = g_programOutput.substr(pos);
        pos = g_programOutput.size();
      } else {
        one = g_programOutput.substr(pos, nl - pos);
        pos = nl + 1;
      }
      // 去掉末尾 \r
      if (!one.empty() && one[one.size() - 1] == '\r') one.erase(one.size() - 1);
      if (!one.empty()) {
        OutText(rx + 12, lineY + 2, one.c_str());
        lineY += WATCH_ITEM_H;
        shown++;
      }
    }
  }

  // 列表区
  int listTop = ry + WATCH_TITLE_H + WATCH_INPUT_H + outH;
  int listBot = ry + rh - WATCH_BOTTOM_H;

  std::vector<int> filtered = GetFilteredSnapshots();
  int totalRows = (int)filtered.size();
  int maxVis = (listBot - listTop) / WATCH_ITEM_H;
  if (g_watchScroll < 0) g_watchScroll = 0;
  if (g_watchScroll > totalRows - maxVis)
    g_watchScroll = (totalRows > maxVis) ? totalRows - maxVis : 0;

  // 空状态
  if (filtered.empty()) {
    settextcolor(g_theme.dropText);
    setbkcolor(g_theme.editBg);
    OutText(rx + 10, listTop + 4,
            g_varSnapshots.empty()
                ? "暂无变量数据，点击「刷新」读取..."
                : "未找到匹配的变量");
  }

  // 列表项
  for (int i = 0; i < maxVis && g_watchScroll + i < totalRows; i++) {
    int idx = filtered[g_watchScroll + i];
    const VarSnapshot &vs = g_varSnapshots[idx];
    int rowY = listTop + i * WATCH_ITEM_H;

    // 悬停高亮
    if (idx == g_watchHover) {
      setfillcolor(g_theme.menuBarHover);
      bar(rx, rowY, rx + rw, rowY + WATCH_ITEM_H);
    }

    setbkcolor(g_theme.editBg);
    settextcolor(g_theme.editText);
    // 格式：BP=行号  变量名(类型) = 值（未赋值显示"暂未赋值"）
    char line[256];
    if (vs.assigned) {
      sprintf(line, "BP=%d  %s(%s) = %s", vs.bpRow, vs.varName.c_str(),
              vs.type.c_str(), vs.value.c_str());
    } else {
      settextcolor(g_theme.dropText);
      sprintf(line, "BP=%d  %s(%s) = 暂未赋值", vs.bpRow, vs.varName.c_str(),
              vs.type.c_str());
    }
    OutText(rx + 8, rowY + 3, line);
  }

  // 滚动条
  if (totalRows > maxVis) {
    int sbX = rx + rw - 8;
    int sbH = listBot - listTop;
    setfillcolor(g_theme.lineNumBg);
    bar(sbX, listTop, sbX + 6, listBot);
    int thumbH = sbH * maxVis / totalRows;
    int thumbY = listTop + (sbH - thumbH) * g_watchScroll / (totalRows - maxVis);
    setfillcolor(g_theme.scrollThumb);
    bar(sbX, thumbY, sbX + 6, thumbY + thumbH);
  }

  // 底部
  int by = ry + rh - WATCH_BOTTOM_H;
  setfillcolor(g_theme.menuBarHover);
  bar(rx, by, rx + rw, by + WATCH_BOTTOM_H);

  // 状态文字
  char status[128];
  sprintf(status, "共 %d 条记录", totalRows);
  setbkcolor(g_theme.menuBarHover);
  settextcolor(g_theme.dropText);
  OutText(rx + 8, by + 6, status);

  // "刷新" 按钮
  int rbx = rx + rw - 72;
  int rbw = 64;
  int rbh = WATCH_BOTTOM_H - 8;
  int rby = by + 4;
  setfillcolor(g_watchHoverRefresh ? RGB(30, 90, 200) : g_theme.lineNumBg);
  bar(rbx, rby, rbx + rbw, rby + rbh);
  setbkcolor(g_watchHoverRefresh ? RGB(30, 90, 200) : g_theme.menuBarHover);
  settextcolor(g_watchHoverRefresh ? WHITE : g_theme.dropText);
  OutText(rbx + 14, rby + 3, "刷新");
}

// 绘制函数列表弹窗
void DrawFuncList() {
  if (!g_funcListOpen || g_funcList.empty())
    return;
  int h = (int)g_funcList.size() * FUNC_ITEM_H;

  // 边框与背景
  setfillcolor(g_theme.dropBg);
  solidrectangle(g_funcListX, g_funcListY, g_funcListX + FUNC_LIST_W,
                 g_funcListY + h);
  setlinecolor(g_theme.dropBorder);
  rectangle(g_funcListX, g_funcListY, g_funcListX + FUNC_LIST_W,
            g_funcListY + h);

  setbkmode(TRANSPARENT);
  settextstyle(14, 0, _T("宋体"));
  for (int i = 0; i < (int)g_funcList.size(); i++) {
    int y = g_funcListY + i * FUNC_ITEM_H;
    if (i == g_funcListHover) {
      setfillcolor(g_theme.highlight);
      solidrectangle(g_funcListX + 1, y + 1, g_funcListX + FUNC_LIST_W - 1,
                     y + FUNC_ITEM_H - 1);
    }
    settextcolor(g_theme.dropText);
    char lineLabel[64];
    sprintf(lineLabel, "%4d  %s", g_funcList[i].line + 1,
            g_funcList[i].name.c_str());
    OutText(g_funcListX + 8, y + (FUNC_ITEM_H - textheight("Ag")) / 2,
            lineLabel);
  }
}

// ============================================================
// 代码自动补全（输入关键词前几个字母时弹出建议列表）
// ============================================================

// 补全候选词：C 关键字 + 常用标准库函数
static const char *g_completionWords[] = {
    // C 关键字
    "auto", "break", "case", "char", "const", "continue", "default", "do",
    "double", "else", "enum", "extern", "float", "for", "goto", "if", "inline",
    "int", "long", "register", "restrict", "return", "short", "signed",
    "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned",
    "void", "volatile", "while", "_Bool", "_Complex", "_Imaginary",
    // 常用标准库函数
    "printf", "scanf", "fprintf", "fscanf", "sprintf", "sscanf", "fopen",
    "fclose", "fread", "fwrite", "fgets", "fputs", "fputc", "fgetc", "malloc",
    "calloc", "realloc", "free", "memcpy", "memset", "memmove", "memcmp",
    "strlen", "strcpy", "strncpy", "strcat", "strncat", "strcmp", "strncmp",
    "strstr", "strchr", "strtok", "atoi", "atof", "atol", "abs", "rand",
    "srand", "time", "clock", "sqrt", "pow", "sin", "cos", "tan", "log",
    "log10", "exp", "floor", "ceil", "fabs", "fmod", "exit", "system",
    "getchar", "putchar", "gets", "puts", NULL};

// 补全弹窗状态
bool g_autocompleteOpen = false;
int g_autocompleteHover = 0;
std::vector<std::string> g_autocompleteItems;
std::string g_autocompletePrefix; // 当前正在输入的前缀
int g_autocompletePrefixLen;      // 前缀在缓冲区中的字节长度
int g_autocompleteX = 0;
int g_autocompleteY = 0;
const int AUTOCOMPLETE_W = 180;
const int AUTOCOMPLETE_H = 20; // 每项高度

// 判断字符是否是标识符组成部分（字母、数字、下划线）
static bool IsIdentChar(char c) {
  return isalnum((unsigned char)c) || c == '_';
}

// 提取光标所在的当前标识符前缀（从光标向左扫描到非标识符字符）
// 返回前缀字符串，并通过 outLen 返回其在缓冲区中的字节长度
static std::string GetCurrentWordPrefix(int &outLen) {
  std::string line = g_buffer.getLine(g_cursor.row);
  int col = g_cursor.col;
  int start = col;
  while (start > 0 && IsIdentChar(line[start - 1]))
    start--;
  outLen = col - start;
  if (outLen <= 0)
    return "";
  return line.substr(start, outLen);
}

// 更新补全状态：根据当前输入的前缀匹配候选词
void UpdateAutocomplete() {
  int prefixLen = 0;
  std::string prefix = GetCurrentWordPrefix(prefixLen);
  g_autocompletePrefix = prefix;
  g_autocompletePrefixLen = prefixLen;

  // 前缀太短不触发（至少 2 个字符）
  if (prefixLen < 2) {
    g_autocompleteOpen = false;
    g_autocompleteItems.clear();
    return;
  }

  g_autocompleteItems.clear();
  for (int i = 0; g_completionWords[i] != NULL; i++) {
    std::string w = g_completionWords[i];
    if ((int)w.size() > prefixLen && w.compare(0, prefixLen, prefix) == 0) {
      g_autocompleteItems.push_back(w);
    }
  }

  if (g_autocompleteItems.empty()) {
    g_autocompleteOpen = false;
    return;
  }

  // 定位弹窗到光标下方
  EnsureFolds();
  int viewRow = g_docToView[g_cursor.row];
  if (viewRow < 0)
    viewRow = 0;
  g_autocompleteX = TEXT_ORIGIN_X + g_cursor.col * CHAR_WIDTH;
  g_autocompleteY = TEXT_ORIGIN_Y + (viewRow + 1) * LINE_HEIGHT;
  // 防止弹窗超出编辑区右边界
  if (g_autocompleteX + AUTOCOMPLETE_W > EDIT_AREA_WIDTH)
    g_autocompleteX = EDIT_AREA_WIDTH - AUTOCOMPLETE_W;
  if (g_autocompleteX < TEXT_ORIGIN_X)
    g_autocompleteX = TEXT_ORIGIN_X;

  g_autocompleteHover = 0;
  g_autocompleteOpen = true;
}

// 接受当前选中的补全项：删除前缀，插入完整词
void AcceptAutocomplete() {
  if (!g_autocompleteOpen || g_autocompleteItems.empty())
    return;
  const std::string &word = g_autocompleteItems[g_autocompleteHover];
  // 删除前缀
  for (int i = 0; i < g_autocompletePrefixLen; i++) {
    if (g_cursor.col > 0) {
      g_buffer.deleteChar(g_cursor.row, g_cursor.col - 1);
      g_cursor.col--;
    }
  }
  // 插入完整词
  for (size_t i = 0; i < word.size(); i++) {
    g_buffer.insertChar(g_cursor.row, g_cursor.col, word[i]);
    g_cursor.col++;
  }
  g_autocompleteOpen = false;
  g_modified = true;
}

// 关闭补全弹窗
void CloseAutocomplete() {
  g_autocompleteOpen = false;
  g_autocompleteItems.clear();
}

// 绘制补全弹窗
void DrawAutocomplete() {
  if (!g_autocompleteOpen || g_autocompleteItems.empty())
    return;
  int n = (int)g_autocompleteItems.size();
  int h = n * AUTOCOMPLETE_H;

  setfillcolor(g_theme.dropBg);
  solidrectangle(g_autocompleteX, g_autocompleteY,
                 g_autocompleteX + AUTOCOMPLETE_W, g_autocompleteY + h);
  setlinecolor(g_theme.dropBorder);
  rectangle(g_autocompleteX, g_autocompleteY, g_autocompleteX + AUTOCOMPLETE_W,
            g_autocompleteY + h);

  setbkmode(TRANSPARENT);
  settextstyle(14, 0, _T("宋体"));
  for (int i = 0; i < n; i++) {
    int y = g_autocompleteY + i * AUTOCOMPLETE_H;
    if (i == g_autocompleteHover) {
      setfillcolor(g_theme.highlight);
      solidrectangle(g_autocompleteX + 1, y + 1,
                     g_autocompleteX + AUTOCOMPLETE_W - 1,
                     y + AUTOCOMPLETE_H - 1);
      settextcolor(g_theme.menuBarBg); // 选中项文字反色
    } else {
      settextcolor(g_theme.dropText);
    }
    // 前缀用关键词色，补全部分用普通色
    const std::string &w = g_autocompleteItems[i];
    int pfxLen = (int)g_autocompletePrefix.size();
    if (pfxLen > (int)w.size())
      pfxLen = (int)w.size();
    settextcolor(g_theme.keyword);
    OutText(g_autocompleteX + 8, y + (AUTOCOMPLETE_H - textheight("Ag")) / 2,
            w.substr(0, pfxLen));
    settextcolor(g_theme.dropText);
    OutText(g_autocompleteX + 8 + pfxLen * CHAR_WIDTH,
            y + (AUTOCOMPLETE_H - textheight("Ag")) / 2, w.substr(pfxLen));
  }
}

// ============================================================
// 编辑操作：复制（选区 -> 剪贴板，不改文本）
// ============================================================
void EditCopy() {
  if (!g_sel.active)
    return; // 无选区不操作
  std::string text = GetSelText();
  if (text.empty())
    return;
  CopyToClipboard(text);
}

// ============================================================
// 编辑操作：剪切（选区 -> 剪贴板，并删除选区内容）
// 返回 true 表示实际发生了剪切（无选区则返回 false）
// ============================================================
bool EditCut() {
  if (!g_sel.active)
    return false;
  std::string text = GetSelText();
  if (!text.empty())
    CopyToClipboard(text);
  UndoBeginAction(false); // 剪切是独立的一步
  DeleteSelContent();
  g_sel.active = false; // 剪切后清选区
  g_modified = true;
  MarkFoldsDirty(true); // 可能删除多行 → 展开
  return true;
}

// ============================================================
// 编辑操作：粘贴（剪贴板 -> 光标位置）
// 先删除当前选区（如有），再插入剪贴板内容
// 支持 \n 换行（多行插入）
// 返回 true 表示实际插入了内容（剪贴板为空则返回 false）
// ============================================================
bool EditPaste() {
  std::string text = PasteFromClipboard();
  if (text.empty())
    return false;

  UndoBeginAction(false); // 粘贴是独立的一步
  bool structural = (text.find('\n') != std::string::npos);
  // 如有选区，先删除（跨多行选区同样改变行数）
  if (g_sel.active) {
    NormalizeSel();
    if (g_sel.startRow != g_sel.endRow)
      structural = true;
    DeleteSelContent();
    g_sel.active = false;
  }

  // 逐字符插入（\n 触发换行）
  int r = g_cursor.row;
  int c = g_cursor.col;
  for (size_t i = 0; i < text.size(); i++) {
    char ch = text[i];
    if (ch == '\r')
      continue; // 跳过 \r（Windows 换行 \r\n）
    if (ch == '\n') {
      g_buffer.insertLineBreak(r, c);
      r++;
      c = 0;
    } else {
      g_buffer.insertChar(r, c, ch);
      c++;
    }
  }
  g_cursor.row = r;
  g_cursor.col = c;
  g_modified = true;
  // 含换行的粘贴（或删了跨多行选区）改变行数 → 展开；纯单行文本保留折叠
  MarkFoldsDirty(structural);
  return true;
}

// 前向声明（定义在下方）：无窗口执行命令行，避免控制台闪烁
int RunDirect(const char *exePath, const char *args, const char *errFile,
              const char *outFile = NULL);

// ============================================================
// 检查 gcc 是否可用
// 优先检查候选路径（Dev-Cpp 自带 MinGW），其次检查 PATH
// 找到后写入 g_gccPath 供后续编译使用
// 返回 true 表示可用
// ============================================================
bool CheckGccAvailable() {
  // 1) 已经检测过，复用结果
  if (!g_gccPath.empty()) {
    std::ifstream f(g_gccPath.c_str());
    return f.is_open();
  }

  // 2) 候选路径列表（按优先级排序）
  const char *candidates[] = {
      "C:\\Program Files (x86)\\Dev-Cpp\\MinGW64\\bin\\gcc.exe",
      "C:\\Dev-Cpp\\MinGW64\\bin\\gcc.exe",
      "D:\\Dev-Cpp\\MinGW64\\bin\\gcc.exe", "C:\\MinGW\\bin\\gcc.exe",
      "C:\\mingw64\\bin\\gcc.exe"};
  const int candCount = sizeof(candidates) / sizeof(candidates[0]);

  for (int i = 0; i < candCount; i++) {
    std::ifstream f(candidates[i]);
    if (f.is_open()) {
      g_gccPath = candidates[i];
      return true;
    }
  }

  // 3) 最后回退到 PATH，看 gcc 是否在系统路径
  //    RunDirect 直接 CreateProcess，可以传 "gcc" 让系统从 PATH 查找
  int ret = RunDirect("gcc", "--version", NULL);
  if (ret == 0) {
    g_gccPath = "gcc"; // 用 PATH 中的 gcc
    return true;
  }
  return false;
}

// ============================================================
// 无窗口执行命令行（避免控制台闪烁）
// 直接用 CreateProcessA 启动目标程序，不再通过 cmd.exe /C 中转 ——
// cmd.exe /C 对多组引号（gcc 路径 + src/exe 路径都含空格时）的解析
// 有缺陷，会把命令截断到第一个空格（报 "'C:\Program' 不是内部或外部命令"）。
// 改直接 CreateProcess 后，命令行只经过一次解析，引号正确传递。
//
// 编译需要把 stderr 重定向到 error.txt：CreateProcessA 不支持 cmd 的
// "2> file" 语法，改用 STARTUPINFO 的 hStdError 把 stderr 绑定到已打开
// 的文件句柄 —— 这正是比 system()/cmd /C 更底层、更可靠的做法。
// ============================================================

// 直接 CreateProcess 启动 exePath，命令行 args（不含 exePath 本身）。
// 若 errFile 非空，把 stderr 重定向到该文件。返回进程退出码。
int RunDirect(const char *exePath, const char *args, const char *errFile,
              const char *outFile) {
  // 完整命令行：exePath + 空格 + args（args 已包含 src/exe 参数）
  // CreateProcessA 会自动把第一对引号内的内容当 exePath，
  // 所以 exePath 若含空格需用 QuoteArg 加引号（在调用方完成）
  std::string full = std::string(exePath) + " " + args;
  char *buf = _strdup(full.c_str());
  if (!buf)
    return -1;

  STARTUPINFOA si = {};
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi = {};

  HANDLE hErr = NULL, hOut = NULL;
  if (errFile || outFile) {
    si.dwFlags = STARTF_USESTDHANDLES;
    if (errFile) {
      hErr = CreateFileA(errFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL, NULL);
      if (hErr == INVALID_HANDLE_VALUE) {
        free(buf);
        return -1;
      }
      SetHandleInformation(hErr, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
      si.hStdError = hErr;
    } else {
      si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    }
    if (outFile) {
      hOut = CreateFileA(outFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL, NULL);
      if (hOut == INVALID_HANDLE_VALUE) {
        if (hErr) CloseHandle(hErr);
        free(buf);
        return -1;
      }
      SetHandleInformation(hOut, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
      si.hStdOutput = hOut;
    } else {
      si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    }
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
  }

  BOOL ok = CreateProcessA(NULL, buf, NULL, NULL,
                           TRUE, // bInheritHandles=TRUE，让子进程继承句柄
                           CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
  free(buf);

  if (hErr) CloseHandle(hErr);
  if (hOut) CloseHandle(hOut);
  if (!ok) {
    if (errFile) DeleteFileA(errFile);
    if (outFile) DeleteFileA(outFile);
    return -1;
  }

  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD code = 1;
  GetExitCodeProcess(pi.hProcess, &code);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return (int)code;
}

// ============================================================
// 读取文件全部内容到 string
// ============================================================
std::string ReadFileAll(const char *path) {
  std::ifstream fin(path, std::ios::binary);
  if (!fin.is_open())
    return "";
  std::string s;
  char buf[4096];
  while (fin.read(buf, sizeof(buf)) || fin.gcount() > 0) {
    s.append(buf, fin.gcount());
  }
  return s;
}

// ============================================================
// 解析 gcc 错误格式：文件名:行号:列号: 类型: 描述
// 鲁棒处理：格式不匹配时整行作为 desc，line=col=0
// ============================================================
DiagEntry ParseGccLine(const std::string &line) {
  DiagEntry e;
  e.line = 0;
  e.col = 0;
  e.type = "";
  e.desc = line; // 默认：整行作为描述

  // gcc 报错格式： "文件名:行号[:列号]: 类型: 描述"
  // Windows 绝对路径形如 "C:\dir\test.c:3:5: error: ..."，
  // 盘符冒号 "C:" 后面是 '\' 而非数字，因此用"冒号后紧跟数字"
  // 来定位行号，可自然跳过盘符，也兼容含空格的路径。
  size_t n = line.size();
  size_t lineColon = std::string::npos;
  for (size_t i = 0; i < n; ++i) {
    if (line[i] == ':' && i + 1 < n && line[i + 1] >= '0' &&
        line[i + 1] <= '9') {
      lineColon = i;
      break;
    }
  }
  if (lineColon == std::string::npos)
    return e;

  // 解析行号
  size_t p = lineColon + 1;
  int ln = 0;
  while (p < n && line[p] >= '0' && line[p] <= '9') {
    ln = ln * 10 + (line[p] - '0');
    ++p;
  }
  e.line = ln;
  e.col = 0;

  // 若紧跟 ":数字"，再解析列号
  if (p < n && line[p] == ':' && p + 1 < n && line[p + 1] >= '0' &&
      line[p + 1] <= '9') {
    ++p;
    int colno = 0;
    while (p < n && line[p] >= '0' && line[p] <= '9') {
      colno = colno * 10 + (line[p] - '0');
      ++p;
    }
    e.col = colno;
  }

  // 余下部分：" 类型: 描述" 或 " 类型 描述"
  if (p >= n)
    return e;
  std::string rest = line.substr(p);
  // 去掉前导空白
  size_t sp = rest.find_first_not_of(" \t");
  if (sp != std::string::npos)
    rest = rest.substr(sp);

  // 提取类型（直到冒号或空白）
  size_t tend = rest.find(':');
  if (tend != std::string::npos) {
    e.type = rest.substr(0, tend);
    // 跳过冒号后的空白
    size_t dstart = rest.find_first_not_of(" \t", tend + 1);
    e.desc = (dstart != std::string::npos) ? rest.substr(dstart) : "";
  } else {
    // 没有冒号分隔类型，整段作为描述
    e.desc = rest;
  }
  return e;
}

// ============================================================
// 从 error.txt 解析所有错误到全局诊断列表
// ============================================================
void ParseErrorFile(const char *path) {
  DiagClear();
  g_diagSource = 0; // gcc 构建错误：整行标红
  std::ifstream fin(path, std::ios::binary);
  if (!fin.is_open())
    return;
  std::string line;
  char ch;
  while (fin.get(ch)) {
    if (ch == '\n') {
      if (!line.empty()) {
        DiagAdd(ParseGccLine(line));
        line.clear();
      }
    } else if (ch != '\r') {
      line += ch;
    }
  }
  // 处理最后一行（无换行结尾）
  if (!line.empty())
    DiagAdd(ParseGccLine(line));
}

// ============================================================
// 静态诊断模块（Static Diagnostics）
// ----------------------------------------------------------------
// 后端优先级：
//   1) clang -fsyntax-only -Wall -Wextra -fdiagnostics-format=json
//      解析 stderr 中的 JSON：kind(severity)/file/line/column/message
//   2) clang 不可用时回退 gcc -fsyntax-only -Wall -Wextra 的文本输出，
//      用行格式 "文件:行:列: 类型: 描述" + 下方 ^~~~ 指示行解析。
// 触发时机：保存文件、语法检测、编译成功后。
// 配置文件：源文件同目录 .mini_c_diag.cfg，可配置 -I 包含路径、
//           -D 宏定义、std= C 标准版本。
// ============================================================

// clang 检测状态：0 未检测 / 1 可用 / 2 不可用
int g_clangState = 0;
std::string g_clangPath = "";
std::string g_diagBackend = ""; // 上次实际使用的后端名（面板提示用）

// UTF-8 文本转 GBK(CP_ACP)：clang 在 Windows 上始终输出 UTF-8，
// 而本程序按 GBK 渲染，不转换会让含中文的路径/消息变乱码。
std::string Utf8ToAcp(const std::string &u8) {
  if (u8.empty())
    return std::string();
  int wlen = MultiByteToWideChar(CP_UTF8, 0, u8.c_str(), (int)u8.size(),
                                 NULL, 0);
  if (wlen <= 0)
    return u8;
  std::vector<wchar_t> wbuf(wlen);
  MultiByteToWideChar(CP_UTF8, 0, u8.c_str(), (int)u8.size(), &wbuf[0],
                      wlen);
  int nlen = WideCharToMultiByte(CP_ACP, 0, &wbuf[0], wlen, NULL, 0, NULL,
                                 NULL);
  if (nlen <= 0)
    return u8;
  std::string out(nlen, '\0');
  WideCharToMultiByte(CP_ACP, 0, &wbuf[0], wlen, &out[0], nlen, NULL, NULL);
  return out;
}

// ---------- 极简 JSON 提取（只服务 clang 诊断格式，非通用解析器） ----------

// 跳过空白
static void JsonSkipWs(const std::string &s, size_t &p, size_t e) {
  while (p < e && (s[p] == ' ' || s[p] == '\t' || s[p] == '\n' ||
                   s[p] == '\r'))
    p++;
}

// 从引号位置读取 JSON 字符串并做反转义；返回 false 表示 null/数字等非字符串
static bool JsonReadString(const std::string &s, size_t p, size_t e,
                           std::string &out, size_t &next) {
  out.clear();
  if (p >= e || s[p] != '"')
    return false;
  p++;
  while (p < e) {
    char c = s[p];
    if (c == '"') {
      next = p + 1;
      return true;
    }
    if (c == '\\' && p + 1 < e) {
      char h = s[p + 1];
      switch (h) {
      case '"': out += '"'; break;
      case '\\': out += '\\'; break;
      case '/': out += '/'; break;
      case 'b': out += '\b'; break;
      case 'f': out += '\f'; break;
      case 'n': out += '\n'; break;
      case 'r': out += '\r'; break;
      case 't': out += '\t'; break;
      case 'u': {
        // \uXXXX：取码点后按 CP_ACP 编码（诊断消息里极少见）
        unsigned cp = 0;
        for (int k = 0; k < 4 && p + 2 + k < e; k++) {
          char d = s[p + 2 + k];
          cp <<= 4;
          if (d >= '0' && d <= '9') cp |= d - '0';
          else if (d >= 'a' && d <= 'f') cp |= d - 'a' + 10;
          else if (d >= 'A' && d <= 'F') cp |= d - 'A' + 10;
        }
        wchar_t wc = (wchar_t)cp;
        char mb[8] = {0};
        int n = WideCharToMultiByte(CP_ACP, 0, &wc, 1, mb, sizeof(mb), NULL,
                                   NULL);
        if (n > 0)
          out.append(mb, n);
        p += 6; // 跳到 \uXXXX 末字节，下方统一 +2
        break;
      }
      default:
        out += h;
        break;
      }
      p += 2;
      continue;
    }
    out += c;
    p++;
  }
  return false; // 未闭合
}

// 读取整数值
static bool JsonReadInt(const std::string &s, size_t p, size_t e, int &out,
                        size_t &next) {
  JsonSkipWs(s, p, e);
  size_t st = p;
  if (p < e && (s[p] == '-' || s[p] == '+'))
    p++;
  size_t ds = p;
  while (p < e && s[p] >= '0' && s[p] <= '9')
    p++;
  if (p == ds)
    return false;
  out = atoi(s.substr(st, p - st).c_str());
  next = p;
  return true;
}

// 在 JSON 子串 [a,e) 内查找位于指定花括号深度的键（子串首字符若为 '{'，
// 其内部键深度即为 1），返回 ':' 后的值起始位置；找不到返回 npos。
// 方括号不改变花括号深度，因此 ranges/children 数组内对象深度自然递增。
static size_t JsonFindKey(const std::string &s, size_t a, size_t e,
                          const char *key, int wantDepth, size_t &valPos) {
  int depth = 0;
  size_t i = a;
  size_t klen = strlen(key);
  while (i < e) {
    char c = s[i];
    if (c == '"') {
      if (depth == wantDepth && i + 1 + klen < e &&
          memcmp(&s[i + 1], key, klen) == 0 && s[i + 1 + klen] == '"') {
        size_t j = i + 1 + klen + 1;
        JsonSkipWs(s, j, e);
        if (j < e && s[j] == ':') {
          j++;
          JsonSkipWs(s, j, e);
          valPos = j;
          return i;
        }
      }
      // 跳过整个字符串（含转义）
      size_t dummy;
      std::string tmp;
      size_t after = i;
      if (JsonReadString(s, i, e, tmp, after))
        i = after;
      else
        i++;
      continue;
    }
    if (c == '{')
      depth++;
    else if (c == '}')
      depth--;
    i++;
  }
  return std::string::npos;
}

// 从 p 所指 '{' 做花括号配对（字符串感知），返回 '}' 位置
static size_t JsonMatchBrace(const std::string &s, size_t p, size_t e) {
  int depth = 0;
  bool inStr = false;
  while (p < e) {
    char c = s[p];
    if (inStr) {
      if (c == '\\')
        p += 2;
      else {
        if (c == '"')
          inStr = false;
        p++;
      }
      continue;
    }
    if (c == '"') {
      inStr = true;
      p++;
      continue;
    }
    if (c == '{')
      depth++;
    else if (c == '}') {
      depth--;
      if (depth == 0)
        return p;
    }
    p++;
  }
  return std::string::npos;
}

// 枚举整个 JSON 文本中所有顶层（深度 0）花括号对象。
// clang 输出为 "[\n{...}\n,\n{...}\n]"，旧版本可能直接逐行输出对象，
// 两种形式都能覆盖；children 等嵌套对象不会被误提。
static void JsonEnumTopObjects(const std::string &s,
                               std::vector<std::pair<size_t, size_t> > &out) {
  int depth = 0;
  bool inStr = false;
  size_t n = s.size();
  for (size_t i = 0; i < n;) {
    char c = s[i];
    if (inStr) {
      if (c == '\\')
        i += 2;
      else {
        if (c == '"')
          inStr = false;
        i++;
      }
      continue;
    }
    if (c == '"') {
      inStr = true;
      i++;
      continue;
    }
    if (c == '{') {
      if (depth == 0) {
        size_t end = JsonMatchBrace(s, i, n);
        if (end == std::string::npos)
          return;
        out.push_back(std::make_pair(i, end + 1));
        i = end + 1;
        continue;
      }
      depth++;
    } else if (c == '}')
      depth--;
    i++;
  }
}

// 取对象内深度 1 的字符串/整型值
static bool JsonObjString(const std::string &s, size_t a, size_t b,
                          const char *key, std::string &out) {
  size_t vp;
  if (JsonFindKey(s, a, b, key, 1, vp) == std::string::npos)
    return false;
  size_t next;
  return JsonReadString(s, vp, b, out, next);
}
static bool JsonObjInt(const std::string &s, size_t a, size_t b,
                       const char *key, int &out) {
  size_t vp;
  if (JsonFindKey(s, a, b, key, 1, vp) == std::string::npos)
    return false;
  size_t next;
  return JsonReadInt(s, vp, b, out, next);
}

// 枚举某键（深度1）数组值中的第一层对象。用于 ranges / children。
static void JsonEnumKeyObjects(const std::string &s, size_t a, size_t b,
                               const char *key,
                               std::vector<std::pair<size_t, size_t> > &out) {
  size_t vp;
  if (JsonFindKey(s, a, b, key, 1, vp) == std::string::npos || vp >= b ||
      s[vp] != '[')
    return;
  size_t i = vp + 1;
  while (i < b && s[i] != ']') {
    if (s[i] == '{') {
      size_t end = JsonMatchBrace(s, i, b);
      if (end == std::string::npos)
        return;
      out.push_back(std::make_pair(i, end + 1));
      i = end + 1;
    } else
      i++;
  }
}

// severity 归一化：clang 的 "fatal error" 也算 error
static std::string NormalizeSeverity(const std::string &sev) {
  if (sev == "warning")
    return "warning";
  if (sev == "note" || sev == "remark")
    return "note";
  if (sev == "error" || sev == "fatal error" || sev == "fatal")
    return "error";
  return sev.empty() ? std::string("error") : sev;
}

// 路径归一化比较：小写 + 斜杠统一；完全相等或仅文件名相等即认为是当前文件
static std::string NormPath(const std::string &p) {
  std::string q = p;
  for (size_t i = 0; i < q.size(); i++) {
    if (q[i] == '/')
      q[i] = '\\';
    else if (q[i] >= 'A' && q[i] <= 'Z')
      q[i] = q[i] - 'A' + 'a';
  }
  return q;
}
static bool IsCurrentFileDiag(const std::string &file,
                              const std::string &srcPath) {
  if (file.empty())
    return true; // 无文件信息（命令行级诊断）予以保留
  std::string a = NormPath(file), b = NormPath(srcPath);
  if (a == b)
    return true;
  // clang 有时把相对路径展开成绝对路径，兜底比较文件名
  return ExtractFileName(a) == ExtractFileName(b);
}

// 依据源码行内容估算波浪线结束列（1-based 排他列）：
// 标识符整段、GBK 双字节 2 列、其余单字符 1 列。
static int EstimateColEnd(const std::string &lineText, int col1) {
  int i = col1 - 1;
  int n = (int)lineText.size();
  if (i < 0)
    i = 0;
  if (i >= n)
    return col1;
  unsigned char c = (unsigned char)lineText[i];
  if (isalnum(c) || c == '_') {
    int j = i;
    while (j < n &&
           (isalnum((unsigned char)lineText[j]) || lineText[j] == '_'))
      j++;
    return col1 + (j - i);
  }
  if (IsGbkLeadByte(c))
    return col1 + 2;
  return col1 + 1;
}

// ---------- clang 检测 ----------
// 优先：与已检测到的 gcc 同目录的 clang.exe（MinGW 工具链自带）；
// 其次：常见 LLVM 安装目录；最后：PATH 中的 clang。
bool CheckClangAvailable() {
  if (g_clangState == 1) {
    if (g_clangPath == "clang")
      return true;
    std::ifstream f(g_clangPath.c_str());
    if (f.is_open())
      return true;
    g_clangState = 0; // 文件被删除则重新检测
  }
  if (g_clangState == 2)
    return false;

  // 1) gcc 同目录
  if (g_gccPath.size() > 7) {
    std::string dir = FolderOfFile(g_gccPath);
    if (!dir.empty()) {
      std::string cand = dir + "clang.exe";
      std::ifstream f(cand.c_str());
      if (f.is_open()) {
        g_clangPath = cand;
        g_clangState = 1;
        return true;
      }
    }
  }
  // 2) LLVM 安装目录
  const char *cands[] = {"C:\\Program Files\\LLVM\\bin\\clang.exe",
                         "C:\\Program Files (x86)\\LLVM\\bin\\clang.exe",
                         "D:\\LLVM\\bin\\clang.exe",
                         "C:\\LLVM\\bin\\clang.exe"};
  for (int i = 0; i < (int)(sizeof(cands) / sizeof(cands[0])); i++) {
    std::ifstream f(cands[i]);
    if (f.is_open()) {
      g_clangPath = cands[i];
      g_clangState = 1;
      return true;
    }
  }
  // 3) PATH
  if (RunDirect("clang", "--version", NULL) == 0) {
    g_clangPath = "clang";
    g_clangState = 1;
    return true;
  }
  g_clangState = 2;
  return false;
}

// ---------- 诊断配置文件 .mini_c_diag.cfg ----------
// 行格式（# 开头为注释，大小写不敏感）：
//   -I./include      或 -I ./include   头文件包含路径
//   -DDEBUG=1        或 -D DEBUG=1     宏定义
//   std=c11                             指定 C 标准（默认 gnu99）
// include 相对路径相对配置文件所在目录解析。
struct DiagConfig {
  std::vector<std::string> includes;
  std::vector<std::string> defines;
  std::string stdver; // 如 c11 / gnu99；空 = 默认
  std::string cfgDir;
};

static std::string TrimStr(const std::string &s) {
  size_t a = s.find_first_not_of(" \t\r\n");
  if (a == std::string::npos)
    return std::string();
  size_t b = s.find_last_not_of(" \t\r\n");
  return s.substr(a, b - a + 1);
}

// 绝对路径判断：盘符开头或斜杠开头
static bool DiagPathIsAbs(const std::string &p) {
  if (p.size() >= 2 && p[1] == ':')
    return true;
  return !p.empty() && (p[0] == '\\' || p[0] == '/');
}

// 当前文档对应的配置文件路径（源文件同目录）
std::string DiagConfigPath() {
  if (!g_filePath.empty())
    return FolderOfFile(g_filePath) + ".mini_c_diag.cfg";
  return std::string(".mini_c_diag.cfg");
}

DiagConfig LoadDiagConfig() {
  DiagConfig cfg;
  if (!g_filePath.empty())
    cfg.cfgDir = FolderOfFile(g_filePath);

  std::string path = DiagConfigPath();
  std::ifstream fin(path.c_str(), std::ios::binary);
  if (!fin.is_open())
    return cfg;
  std::string content((std::istreambuf_iterator<char>(fin)),
                      std::istreambuf_iterator<char>());
  content = Utf8ToAcp(content); // 兼容用户用 UTF-8 编辑配置

  size_t start = 0;
  while (start <= content.size()) {
    size_t nl = content.find('\n', start);
    std::string raw =
        content.substr(start, nl == std::string::npos ? std::string::npos
                                                      : nl - start);
    start = (nl == std::string::npos) ? content.size() + 1 : nl + 1;
    std::string line = TrimStr(raw);
    if (line.empty() || line[0] == '#')
      continue;

    // 相对路径转绝对（以盘符/反斜杠/正斜杠开头视为绝对路径）
    if ((line.size() > 2 && (line[0] == '-') &&
         (line[1] == 'I' || line[1] == 'i'))) {
      std::string v = TrimStr(line.substr(2));
      if (!v.empty()) {
        if (!DiagPathIsAbs(v) && !cfg.cfgDir.empty())
          v = cfg.cfgDir + v;
        cfg.includes.push_back(v);
      }
    } else if ((line.size() > 2 && (line[0] == '-') &&
                (line[1] == 'D' || line[1] == 'd'))) {
      std::string v = TrimStr(line.substr(2));
      if (!v.empty())
        cfg.defines.push_back(v);
    } else if (line.size() > 4 &&
               (line[0] == 's' || line[0] == 'S') &&
               line.substr(0, 4) == "std=") {
      cfg.stdver = TrimStr(line.substr(4));
    } else if (line.size() > 8 &&
               line.compare(0, 8, "include=") == 0) {
      std::string v = TrimStr(line.substr(8));
      if (!v.empty()) {
        if (!DiagPathIsAbs(v) && !cfg.cfgDir.empty())
          v = cfg.cfgDir + v;
        cfg.includes.push_back(v);
      }
    } else if (line.size() > 7 && line.compare(0, 7, "define=") == 0) {
      std::string v = TrimStr(line.substr(7));
      if (!v.empty())
        cfg.defines.push_back(v);
    }
  }
  return cfg;
}

// 拼出 -I/-D/-std 参数片段
std::string DiagConfigFlags(const DiagConfig &cfg) {
  std::string args;
  args += " -std=";
  args += cfg.stdver.empty() ? "gnu99" : cfg.stdver;
  for (size_t i = 0; i < cfg.includes.size(); i++) {
    args += " -I";
    args += QuoteArg(cfg.includes[i]);
  }
  for (size_t i = 0; i < cfg.defines.size(); i++) {
    args += " -D";
    args += QuoteArg(cfg.defines[i]);
  }
  return args;
}

// 菜单"诊断配置..."：不存在则写模板，再用记事本打开让用户编辑
void OpenDiagConfig() {
  std::string path = DiagConfigPath();
  std::ifstream probe(path.c_str(), std::ios::binary);
  if (!probe.is_open()) {
    std::ofstream fout(path.c_str(), std::ios::binary);
    if (fout.is_open()) {
      const char *tpl =
          "# Mini-C Studio 静态诊断配置\r\n"
          "# 以 # 开头为注释；修改后保存，下次\"语法检测/保存/编译\"时生效\r\n"
          "\r\n"
          "# 头文件包含路径（相对路径相对本文件所在目录，每行一个）\r\n"
          "# -I./include\r\n"
          "\r\n"
          "# 宏定义（等价于命令行 -D）\r\n"
          "# -DDEBUG\r\n"
          "# -DBUFFER_SIZE=256\r\n"
          "\r\n"
          "# C 语言标准（默认 gnu99，可改 c89/c99/c11/gnu11 等）\r\n"
          "# std=c11\r\n";
      fout.write(tpl, (std::streamsize)strlen(tpl));
      fout.close();
    }
  } else {
    probe.close();
  }
  std::string dir = FolderOfFile(path);
  ShellExecuteA(GetHWnd(), "open", "notepad.exe", QuoteArg(path).c_str(),
                dir.empty() ? NULL : dir.c_str(), SW_SHOWNORMAL);
}

// 取对象内某键对应的对象值 "key":{...}，返回该对象区间 [oa,ob)
static bool JsonKeyObject(const std::string &s, size_t a, size_t b,
                          const char *key, size_t &oa, size_t &ob) {
  size_t vp;
  if (JsonFindKey(s, a, b, key, 1, vp) == std::string::npos || vp >= b ||
      s[vp] != '{')
    return false;
  size_t e = JsonMatchBrace(s, vp, b);
  if (e == std::string::npos)
    return false;
  oa = vp;
  ob = e + 1;
  return true;
}

// 从一个位置对象 {file,line,col} 读取三个字段（缺省字段保持原值）
static void JsonReadPosition(const std::string &s, size_t a, size_t b,
                             std::string &file, int &line, int &col) {
  if (file.empty())
    JsonObjString(s, a, b, "file", file);
  JsonObjInt(s, a, b, "line", line);
  JsonObjInt(s, a, b, "col", col);
}

// 解析单个 clang JSON 诊断对象（主诊断与 children 附注共用）。
// 兼容两版格式：
//   旧版(clang ~10)：顶层 file/line/col + "ranges":[{start,end}]
//   新版(clang 12+)：无顶层位置，"range":{"start":{file,line,col},
//                                        "end":{...}}
static bool ExtractClangDiag(const std::string &s, size_t a, size_t b,
                             DiagEntry &out) {
  std::string sev, msg, file;
  int line = 0, col = 0, colEnd = 0;
  if (!JsonObjString(s, a, b, "severity", sev))
    return false;
  JsonObjString(s, a, b, "message", msg);
  JsonObjString(s, a, b, "file", file);
  JsonObjInt(s, a, b, "line", line);
  JsonObjInt(s, a, b, "col", col);

  // 新版：位置内嵌于 "range":"start"
  if (line <= 0) {
    size_t ra, rb;
    if (JsonKeyObject(s, a, b, "range", ra, rb)) {
      size_t sa, sb;
      if (JsonKeyObject(s, ra, rb, "start", sa, sb))
        JsonReadPosition(s, sa, sb, file, line, col);
    }
  }

  // 结束列（排他）：新版 range.end.col
  {
    size_t ra, rb;
    if (JsonKeyObject(s, a, b, "range", ra, rb)) {
      size_t ea, eb;
      if (JsonKeyObject(s, ra, rb, "end", ea, eb))
        JsonObjInt(s, ea, eb, "col", colEnd);
    }
  }
  // 旧版：ranges[0].end.col
  if (colEnd <= 0) {
    std::vector<std::pair<size_t, size_t> > ranges;
    JsonEnumKeyObjects(s, a, b, "ranges", ranges);
    if (!ranges.empty()) {
      size_t ea, eb;
      if (JsonKeyObject(s, ranges[0].first, ranges[0].second, "end", ea, eb))
        JsonObjInt(s, ea, eb, "col", colEnd);
    }
  }

  out.line = line;
  out.col = col;
  out.colEnd = (colEnd > col) ? colEnd : 0; // 无效范围交给 EstimateColEnd
  out.file = file;
  out.type = NormalizeSeverity(sev);
  out.desc = msg;
  return true;
}

// ---------- clang JSON 诊断解析 ----------
// 成功解析至少一个对象（包括"零诊断"时 clang 输出 "[]"）返回 true；
// 输出不是合法 JSON（如老版本 clang 不支持该参数）返回 false 以便回退。
bool ParseClangJsonDiagnostics(const std::string &jsonText,
                               const std::string &srcPath) {
  std::vector<std::pair<size_t, size_t> > objs;
  JsonEnumTopObjects(jsonText, objs);
  if (objs.empty()) {
    // "[]" 合法但无诊断；若文本里除空白/方括号外还有内容则为非法输出
    for (size_t i = 0; i < jsonText.size(); i++) {
      char c = jsonText[i];
      if (c != '[' && c != ']' && c != ' ' && c != '\t' && c != '\r' &&
          c != '\n')
        return false;
    }
    return true; // 合法的空数组
  }

  for (size_t oi = 0; oi < objs.size(); oi++) {
    size_t a = objs[oi].first, b = objs[oi].second;

    DiagEntry e;
    if (!ExtractClangDiag(jsonText, a, b, e))
      continue;
    // 只保留当前文件的诊断（系统头文件内的噪声由 clang 自身抑制，
    // 这里再做一道过滤；无 file 的命令行级诊断保留）
    if (e.line > 0 && !IsCurrentFileDiag(e.file, srcPath))
      continue;
    DiagAdd(e);

    // children（如"上一个定义在这里"等附注）：同文件才展示
    std::vector<std::pair<size_t, size_t> > children;
    JsonEnumKeyObjects(jsonText, a, b, "children", children);
    for (size_t ci = 0; ci < children.size(); ci++) {
      DiagEntry ce;
      if (!ExtractClangDiag(jsonText, children[ci].first, children[ci].second,
                            ce))
        continue;
      if (ce.line <= 0 || !IsCurrentFileDiag(ce.file, srcPath))
        continue;
      DiagAdd(ce);
    }
  }
  return true;
}

// ---------- GCC 文本格式回退解析 ----------
// 严格识别 "文件:行[:列]: (fatal )error|warning|note: 描述"，
// 跳过源码上下文行、^~~~ 指示行、"In file included from" 等非诊断行。
struct GccDiagLine {
  std::string file;
  int line;
  int col;
  std::string type; // error/warning/note
  std::string desc;
};

static bool ParseGccDiagLine(const std::string &text, GccDiagLine &out) {
  out.file.clear();
  out.line = 0;
  out.col = 0;
  out.type.clear();
  out.desc.clear();

  size_t n = text.size();
  size_t lineColon = std::string::npos;
  for (size_t i = 0; i < n; ++i) {
    if (text[i] == ':' && i + 1 < n && text[i + 1] >= '0' &&
        text[i + 1] <= '9') {
      lineColon = i;
      break;
    }
  }
  if (lineColon == std::string::npos)
    return false;

  out.file = text.substr(0, lineColon);

  size_t p = lineColon + 1;
  int ln = 0;
  while (p < n && text[p] >= '0' && text[p] <= '9') {
    ln = ln * 10 + (text[p] - '0');
    ++p;
  }
  out.line = ln;

  if (p < n && text[p] == ':' && p + 1 < n && text[p + 1] >= '0' &&
      text[p + 1] <= '9') {
    ++p;
    int cc = 0;
    while (p < n && text[p] >= '0' && text[p] <= '9') {
      cc = cc * 10 + (text[p] - '0');
      ++p;
    }
    out.col = cc;
  }

  if (p >= n || text[p] != ':')
    return false;
  std::string rest = TrimStr(text.substr(p + 1));

  // 类型允许 "fatal error"（含空格）
  static const char *kKnown[] = {"fatal error", "error", "warning", "note"};
  static const char *kMap[] = {"error", "error", "warning", "note"};
  for (int k = 0; k < 4; k++) {
    size_t kl = strlen(kKnown[k]);
    if (rest.size() >= kl && rest.compare(0, kl, kKnown[k]) == 0 &&
        (rest.size() == kl || rest[kl] == ':' || rest[kl] == ' ')) {
      out.type = kMap[k];
      size_t d = kl;
      if (d < rest.size() && rest[d] == ':')
        d++;
      out.desc = TrimStr(rest.substr(d));
      // 去掉 gcc 附带的告警选项后缀，如 "未使用变量 [-Wunused-variable]"
      if (!out.desc.empty() && out.desc[out.desc.size() - 1] == ']') {
        size_t p = out.desc.rfind(" [-W");
        if (p != std::string::npos)
          out.desc = TrimStr(out.desc.substr(0, p));
      }
      return true;
    }
  }
  return false; // 类型无法识别 → 源码上下文等非诊断行
}

// 在诊断行之后的若干行里找 "^~~~" 指示行，返回波浪覆盖的列数
static int GccCaretRunLength(const std::vector<std::string> &lines,
                             size_t idx) {
  for (size_t k = idx + 1; k < lines.size() && k < idx + 4; k++) {
    const std::string &l = lines[k];
    size_t car = l.find('^');
    if (car == std::string::npos)
      continue;
    // '^' 之前只允许空白/竖线/数字（形如 "  3 |   ^"）
    bool ok = true;
    for (size_t q = 0; q < car; q++) {
      char c = l[q];
      if (c != ' ' && c != '\t' && c != '|' && !(c >= '0' && c <= '9') &&
          c != '-' && c != '+') {
        ok = false;
        break;
      }
    }
    if (!ok)
      continue;
    size_t run = 0;
    while (car + run < l.size() &&
           (l[car + run] == '^' || l[car + run] == '~'))
      run++;
    return run > 0 ? (int)run : 1;
  }
  return 0;
}

void ParseGccTextDiagnostics(const std::vector<std::string> &lines,
                             const std::string &srcPath) {
  for (size_t i = 0; i < lines.size(); i++) {
    GccDiagLine d;
    if (!ParseGccDiagLine(lines[i], d))
      continue;
    if (d.line <= 0 || !IsCurrentFileDiag(d.file, srcPath))
      continue;
    DiagEntry e;
    e.line = d.line;
    e.col = d.col;
    e.file = d.file;
    e.type = d.type;
    e.desc = d.desc;
    e.colEnd = 0;
    if (d.col > 0) {
      int run = GccCaretRunLength(lines, i);
      // run>=2（^^~/^^^）直接采用；单字符 '^' 留给 EstimateColEnd
      // 扩展到整个标识符，波浪线更醒目（gcc 4.7 只输出单字符 caret）
      if (run >= 2)
        e.colEnd = d.col + run; // 排他列
    }
    DiagAdd(e);
  }
}

// 用 gcc 文本格式解析器处理构建输出 error.txt。
// 与旧 ParseErrorFile 不同：会正确识别 error/warning/note 类型、解析列号、
// 读取 ^~~~ 指示范围，并设置 g_diagSource=1 以触发波浪线渲染。
void ParseBuildErrorFile(const char *path, const std::string &srcPath) {
  DiagClear();
  g_diagSource = 1;
  g_diagSelIdx = -1;
  std::string raw = ReadFileAll(path);
  std::vector<std::string> lines;
  std::string cur;
  for (size_t i = 0; i < raw.size(); i++) {
    if (raw[i] == '\n') {
      if (!cur.empty() && cur[cur.size() - 1] == '\r')
        cur.erase(cur.size() - 1);
      lines.push_back(cur);
      cur.clear();
    } else
      cur += raw[i];
  }
  if (!cur.empty()) {
    if (cur[cur.size() - 1] == '\r')
      cur.erase(cur.size() - 1);
    lines.push_back(cur);
  }
  ParseGccTextDiagnostics(lines, srcPath);
  g_diagBackend = "gcc";
}

// 把当前缓冲区写入临时文件（未保存文档做静态诊断时使用）
bool WriteBufferToTemp() {
  std::ofstream fout(TEMP_C, std::ios::binary);
  if (!fout.is_open())
    return false;
  std::string text = g_buffer.getText();
  fout.write(text.c_str(), (std::streamsize)text.size());
  fout.close();
  return true;
}

// 当前文件名是否为 C 系源文件（保存时是否值得触发诊断）
bool IsCSourceFile(const std::string &path) {
  size_t dot = path.find_last_of('.');
  if (dot == std::string::npos)
    return false;
  std::string ext = path.substr(dot);
  for (size_t i = 0; i < ext.size(); i++)
    if (ext[i] >= 'A' && ext[i] <= 'Z')
      ext[i] = ext[i] - 'A' + 'a';
  return ext == ".c" || ext == ".h" || ext == ".cpp" || ext == ".cc" ||
         ext == ".cxx" || ext == ".hpp";
}

// ============================================================
// 执行静态诊断
// quiet=true（保存/编译后自动触发）：找不到后端不弹框；
// quiet=false（菜单"语法检测"）：找不到后端时弹框提示。
// 返回 true 表示存在 error 级诊断。
// ============================================================
bool RunStaticDiagnostics(bool quiet) {
  DiagClear();
  g_diagSource = 1;
  g_diagSelIdx = -1;

  // 选择被分析文件：已保存文档直接分析磁盘文件（相对 include 正确）；
  // 未保存的新文档/有改动时写入 temp.c 分析。
  std::string srcPath;
  bool usedTemp = false;
  if (!g_filePath.empty() && !g_modified) {
    srcPath = g_filePath;
  } else {
    if (!WriteBufferToTemp()) {
      DiagEntry e;
      e.line = 0;
      e.col = 0;
      e.colEnd = 0;
      e.type = "error";
      e.desc = "无法写入临时文件 temp.c";
      DiagAdd(e);
      g_buildOk = false;
      return true;
    }
    srcPath = TEMP_C;
    usedTemp = true;
  }

  DiagConfig cfg = LoadDiagConfig();
  std::string cfgFlags = DiagConfigFlags(cfg);

  bool ran = false;

  // ---- 后端 1：clang（JSON） ----
  if (CheckClangAvailable()) {
    std::string args =
        "-fsyntax-only -Wall -Wextra -fno-color-diagnostics "
        "-fdiagnostics-format=json " +
        cfgFlags + " " + QuoteArg(srcPath);

    // LLVM 独立安装的 clang 需要指定 mingw 目标与工具链，否则会报
    // "stdio.h not found"（与 gcc 同目录时 clang 可自行探测，无需添加）
    if (g_clangPath != "clang" && g_gccPath.size() > 7) {
      std::string gccDir = FolderOfFile(g_gccPath);
      std::string clangDir = FolderOfFile(g_clangPath);
      if (!gccDir.empty() && NormPath(gccDir) != NormPath(clangDir)) {
        std::string toolchain = gccDir.substr(0, gccDir.size() - 1);
        size_t bs = toolchain.find_last_of("\\/");
        if (bs != std::string::npos)
          toolchain = toolchain.substr(0, bs); // MinGW64 根（含 bin 目录）
        args = "--target=x86_64-w64-mingw32 --gcc-toolchain=" +
               QuoteArg(toolchain) + " " + args;
      }
    }

    int ret = RunDirect(QuoteArg(g_clangPath).c_str(), args.c_str(), ERR_TXT);
    (void)ret;
    std::string raw = ReadFileAll(ERR_TXT);
    std::string text = Utf8ToAcp(raw);
    bool jsonOk = ParseClangJsonDiagnostics(text, srcPath);
    if (jsonOk) {
      ran = true;
      g_diagBackend = "clang";
    }
    // jsonOk=false：clang 存在但输出非法（如版本过旧不支持 JSON 参数），
    // 落到下方 GCC 回退。
  }

  // ---- 后端 2：GCC（文本） ----
  if (!ran) {
    if (!CheckGccAvailable()) {
      if (!quiet) {
        MessageBoxA(GetHWnd(),
                    "未找到 clang 或 gcc，无法进行静态诊断。\n"
                    "请安装 LLVM(clang) 或 MinGW(gcc) 并将其 bin 目录加入 "
                    "PATH。",
                    "静态诊断", MB_OK | MB_ICONERROR);
      }
      DiagEntry e;
      e.line = 0;
      e.col = 0;
      e.colEnd = 0;
      e.type = "error";
      e.desc = "未找到 clang/gcc 后端";
      DiagAdd(e);
      g_buildOk = false;
      if (usedTemp)
        remove(TEMP_C);
      remove(ERR_TXT);
      return true;
    }
    std::string args =
        "-fsyntax-only -Wall -Wextra -fmessage-length=0 " + cfgFlags + " " +
        QuoteArg(srcPath);
    int ret = RunDirect(QuoteArg(g_gccPath).c_str(), args.c_str(), ERR_TXT);
    (void)ret;
    std::string raw = ReadFileAll(ERR_TXT);
    std::vector<std::string> lines;
    {
      std::string cur;
      for (size_t i = 0; i < raw.size(); i++) {
        if (raw[i] == '\n') {
          if (!cur.empty() && cur[cur.size() - 1] == '\r')
            cur.erase(cur.size() - 1);
          lines.push_back(cur);
          cur.clear();
        } else
          cur += raw[i];
      }
      if (!cur.empty()) {
        if (cur[cur.size() - 1] == '\r')
          cur.erase(cur.size() - 1);
        lines.push_back(cur);
      }
    }
    ParseGccTextDiagnostics(lines, srcPath);
    ran = true;
    g_diagBackend = "gcc";
  }

  if (usedTemp)
    remove(TEMP_C);
  remove(ERR_TXT);

  // 统计 error/warning；colEnd 未知时按源码 token 估算
  int errs = 0, warns = 0;
  for (int i = 0; i < g_diagCount; i++) {
    DiagEntry &e = g_diagList[i];
    if (e.type == "error")
      errs++;
    else if (e.type == "warning")
      warns++;
    if (e.line > 0 && (e.colEnd <= 0 || e.colEnd <= e.col)) {
      int r = e.line - 1;
      if (r < g_buffer.getRowCount())
        e.colEnd = e.col > 0
                       ? EstimateColEnd(g_buffer.getLine(r), e.col)
                       : 0;
    }
  }

  if (errs == 0 && warns == 0) {
    DiagEntry ok;
    ok.line = 0;
    ok.col = 0;
    ok.colEnd = 0;
    ok.type = "success";
    ok.desc = std::string("静态诊断通过（") + g_diagBackend + "）";
    DiagAdd(ok);
    g_buildOk = true;
  } else {
    g_buildOk = (errs == 0);
  }
  return errs > 0;
}

// ============================================================
// 静态诊断波浪线：在某一可见行的 error/warning 列范围下方绘制
// error 红色 / warning 黄色；note 不画线。
// ============================================================
void DrawLineDiagnosticSquiggles(int r, int y) {
  if (g_diagSource != 1 || g_diagCount == 0)
    return;
  int lineNo = r + 1;
  int rowLen = g_buffer.getColCount(r);
  int clipRight = EDIT_AREA_WIDTH - SCROLLBAR_W; // 不侵入垂直滚动条
  int baseY = y + LINE_HEIGHT - 6;

  for (int i = 0; i < g_diagCount; i++) {
    const DiagEntry &e = g_diagList[i];
    if (e.line != lineNo)
      continue;
    if (e.type != "error" && e.type != "warning")
      continue;
    if (e.col <= 0)
      continue;

    int c1 = e.col - 1; // 起始字节列（0-based）
    int c2;             // 结束字节列（排他）
    if (e.colEnd > e.col)
      c2 = e.colEnd - 1;
    else
      c2 = c1 + 1;
    if (c1 < 0)
      c1 = 0;
    if (c2 > rowLen)
      c2 = rowLen;
    if (c2 <= c1)
      continue;

    int x1 = TEXT_ORIGIN_X + (c1 - g_leftCol) * CHAR_WIDTH;
    int x2 = TEXT_ORIGIN_X + (c2 - g_leftCol) * CHAR_WIDTH;
    // 水平裁剪：整条在窗外则跳过
    if (x2 <= TEXT_ORIGIN_X || x1 >= clipRight)
      continue;
    if (x1 < TEXT_ORIGIN_X)
      x1 = TEXT_ORIGIN_X;
    if (x2 > clipRight)
      x2 = clipRight;

    setlinecolor(e.type == "error" ? g_theme.diagError : g_theme.diagWarning);
    setlinestyle(PS_SOLID, 1);
    // 2px 一段的正弦波浪：y 在 baseY 与 baseY-3 间交替
    int prevX = x1, prevY = baseY;
    bool up = true;
    for (int xx = x1 + 2; xx <= x2; xx += 2) {
      int yy = up ? baseY - 3 : baseY;
      line(prevX, prevY, xx, yy);
      prevX = xx;
      prevY = yy;
      up = !up;
    }
    if (prevX < x2)
      line(prevX, prevY, x2, up ? baseY - 3 : baseY);
  }
}

// 双击/单击诊断项的统一跳转：preciseCol 时定位到列，否则只定位行首
void JumpToDiagnostic(int idx, bool preciseCol) {
  if (idx < 0 || idx >= g_diagCount)
    return;
  const DiagEntry &e = g_diagList[idx];
  if (e.line <= 0)
    return; // 命令行级诊断无位置可跳
  int target = e.line - 1;
  int rows = g_buffer.getRowCount();
  if (target >= rows)
    target = rows - 1;
  if (target < 0)
    target = 0;
  EnsureFolds();
  if (g_docToView[target] < 0)
    ExpandFoldsContaining(target);
  g_cursor.row = target;
  if (preciseCol && e.col > 0) {
    int rowLen = g_buffer.getColCount(target);
    g_cursor.col = e.col - 1;
    if (g_cursor.col > rowLen)
      g_cursor.col = rowLen;
    if (g_cursor.col < 0)
      g_cursor.col = 0;
  } else {
    g_cursor.col = 0;
  }
  g_sel.active = false;
  g_highlightLine = target;
  EnsureCursorVisible(); // 纵向+横向滚动到目标行列
}

// ============================================================
// 绘制：诊断面板（右侧 300×646）
// ============================================================
void DrawDiagPanel() {
  // 面板背景
  setfillcolor(g_theme.diagBg);
  bar(DIAG_PANEL_X, DIAG_PANEL_Y, DIAG_PANEL_X + DIAG_PANEL_WIDTH,
      DIAG_PANEL_Y + DIAG_PANEL_HEIGHT);

  // 面板左边竖线分隔
  setlinecolor(g_theme.diagBorder);
  line(DIAG_PANEL_X, DIAG_PANEL_Y, DIAG_PANEL_X,
       DIAG_PANEL_Y + DIAG_PANEL_HEIGHT);

  // 标题栏
  setfillcolor(g_theme.diagTitleBg);
  bar(DIAG_PANEL_X, DIAG_PANEL_Y, DIAG_PANEL_X + DIAG_PANEL_WIDTH,
      DIAG_PANEL_Y + DIAG_TITLE_H);
  setlinecolor(g_theme.diagBorder);
  rectangle(DIAG_PANEL_X, DIAG_PANEL_Y, DIAG_PANEL_X + DIAG_PANEL_WIDTH,
            DIAG_PANEL_Y + DIAG_TITLE_H);

  setbkcolor(g_theme.diagTitleBg);
  settextcolor(g_theme.dropText);
  settextstyle(14, 0, _T("宋体"));
  OutText(DIAG_PANEL_X + 8, DIAG_PANEL_Y + 5, "问题面板");
  // 标题栏右侧提示双击跳转
  settextcolor(g_theme.diagMuted);
  settextstyle(12, 0, _T("宋体"));
  OutText(DIAG_PANEL_X + DIAG_PANEL_WIDTH - 70, DIAG_PANEL_Y + 7,
          "双击跳转行列");

  // 内容区
  int contentY = DIAG_PANEL_Y + DIAG_TITLE_H;

  if (g_diagCount == 0) {
    setbkcolor(g_theme.diagBg);
    settextcolor(g_theme.diagMuted);
    settextstyle(13, 0, _T("宋体"));
    OutText(DIAG_PANEL_X + 8, contentY + 8, "(无诊断信息)");
    return;
  }

  // 逐条绘制问题项：彩色类型签 + 行:列 + 描述
  settextstyle(13, 0, _T("宋体"));
  int maxShow = (DIAG_PANEL_HEIGHT - DIAG_TITLE_H - 20) / DIAG_ITEM_HEIGHT;
  int showCount = (g_diagCount < maxShow) ? g_diagCount : maxShow;

  for (int i = 0; i < showCount; i++) {
    int y = contentY + i * DIAG_ITEM_HEIGHT;
    const DiagEntry &de = g_diagList[i];

    // 选中项高亮
    if (i == g_diagSelIdx) {
      setfillcolor(g_theme.highlight);
      bar(DIAG_PANEL_X, y, DIAG_PANEL_X + DIAG_PANEL_WIDTH,
          y + DIAG_ITEM_HEIGHT);
    }

    int tx = DIAG_PANEL_X + 6;
    // 1) 类型签（彩色）
    std::string tag;
    if (de.type == "error") {
      tag = "错";
      settextcolor(g_theme.diagError);
    } else if (de.type == "warning") {
      tag = "警";
      settextcolor(g_theme.diagWarning);
    } else if (de.type == "note") {
      tag = "注";
      settextcolor(g_theme.diagInfo);
    } else if (de.type == "success") {
      tag = "√";
      settextcolor(g_theme.diagSuccess);
    } else {
      tag = "?";
      settextcolor(g_theme.diagInfo);
    }
    setbkcolor((i == g_diagSelIdx) ? g_theme.highlight : g_theme.diagBg);
    OutText(tx, y + 4, tag);
    tx += textwidth(tag.c_str()) + 6;

    // 2) 行:列（次要色）
    if (de.line > 0) {
      std::string loc = IntToStr(de.line);
      if (de.col > 0)
        loc += ":" + IntToStr(de.col);
      loc += " ";
      settextcolor(g_theme.diagMuted);
      OutText(tx, y + 4, loc);
      tx += textwidth(loc.c_str());
    }

    // 3) 描述（普通文字色），按剩余像素 GBK 安全截断
    settextcolor(g_theme.dropText);
    int remainW = (DIAG_PANEL_X + DIAG_PANEL_WIDTH - 4) - tx;
    std::string desc = de.desc;
    // 去掉描述内的换行/制表，避免在面板上错位
    for (size_t q = 0; q < desc.size(); q++)
      if (desc[q] == '\n' || desc[q] == '\r' || desc[q] == '\t')
        desc[q] = ' ';
    std::string ell = "...";
    int ellW = textwidth(ell.c_str());
    if (textwidth(desc.c_str()) > remainW) {
      // 逐字节缩短，同时保证不切在 GBK 双字节中间
      while (!desc.empty() &&
             textwidth(desc.c_str()) + ellW > remainW) {
        size_t L = desc.size();
        if (L >= 2 && IsGbkLeadByte((unsigned char)desc[L - 2]))
          desc.resize(L - 2);
        else
          desc.resize(L - 1);
      }
      desc += ell;
    }
    OutText(tx, y + 4, desc);
  }

  // 底部汇总：错误数 / 警告数 / 总数 / 后端
  int nErr = 0, nWarn = 0;
  for (int i = 0; i < g_diagCount; i++) {
    if (g_diagList[i].type == "error")
      nErr++;
    else if (g_diagList[i].type == "warning")
      nWarn++;
  }
  settextcolor(g_theme.diagMuted);
  settextstyle(12, 0, _T("宋体"));
  std::string summary =
      "错" + IntToStr(nErr) + " 警" + IntToStr(nWarn) +
      " 共" + IntToStr(g_diagCount);
  if (!g_diagBackend.empty())
    summary += " [" + g_diagBackend + "]";
  OutText(DIAG_PANEL_X + 6, DIAG_PANEL_Y + DIAG_PANEL_HEIGHT - 17, summary);
}

// ============================================================
// 点击检测：诊断面板错误项
// 返回索引（-1 表示未点中）
// ============================================================
int HitTestDiagPanel(int mx, int my) {
  if (g_diagCount == 0)
    return -1;
  int contentY = DIAG_PANEL_Y + DIAG_TITLE_H;
  if (my < contentY)
    return -1;
  int maxShow = (DIAG_PANEL_HEIGHT - DIAG_TITLE_H - 20) / DIAG_ITEM_HEIGHT;
  int showCount = (g_diagCount < maxShow) ? g_diagCount : maxShow;
  int endY = contentY + showCount * DIAG_ITEM_HEIGHT;
  if (my >= endY)
    return -1;
  if (mx < DIAG_PANEL_X || mx > DIAG_PANEL_X + DIAG_PANEL_WIDTH)
    return -1;
  return (my - contentY) / DIAG_ITEM_HEIGHT;
}

// ============================================================
// 编译当前缓冲区
// 流程：
//   1. 新文件先弹"保存为"；并把当前内容落盘到源文件（编译即保存）
//   2. 检查 gcc
//   3. gcc "源文件.c" -o "同目录同名.exe" 2> error.txt
//   4. 读 error.txt 判断成功/失败
// 返回 true 表示编译成功
// ============================================================
bool CompileCurrent() {
  // 0) 确保源文件已保存到用户选择的路径（DEV-C++ 行为）；
  //    新文件会先弹"保存为"，取消则中止编译
  if (!EnsureSavedForBuild())
    return false;

  // 1) 检查编译器
  if (!CheckGccAvailable()) {
    MessageBoxA(GetHWnd(),
                "编译器未找到，请安装 MinGW 并将其 bin 目录加入 PATH。",
                "编译错误", MB_OK | MB_ICONERROR);
    return false;
  }

  // 2) 源文件已在 EnsureSavedForBuild 中落盘；推导同目录 exe 路径
  std::string srcPath = g_filePath;
  std::string exePath = ExePathForSource(srcPath);

  // 3) 调用 gcc 编译真实源文件（路径加引号，兼容空格/中文）。
  // stderr 重定向通过 RunDirect 的 STARTUPINFO hStdError 完成，
  // 命令行里不再出现 "2> error.txt" —— 那是 cmd.exe 的语法。
  std::string gccFull = QuoteArg(g_gccPath);
  // -fexec-charset=GBK：用户源文件为 UTF-8 时，把字符串常量统一转成
  // GBK 再输出，避免 printf 中文在 GBK 控制台/监视窗口里乱码
  std::string args = std::string("-std=gnu99 -fexec-charset=GBK ") +
                     QuoteArg(srcPath) + " -o " + QuoteArg(exePath);
  int ret = RunDirect(gccFull.c_str(), args.c_str(), ERR_TXT);

  // 4) 用 gcc 返回值判断成败（更可靠），辅以 error.txt 内容
  // 注意：即使 gcc 无错误，2> 重定向也会创建空的 error.txt
  std::string err = ReadFileAll(ERR_TXT);

  if (ret == 0 && err.empty()) {
    // 编译成功：再跑一次 -Wall -Wextra 静态诊断（clang 优先，gcc 回退），
    // 让警告以黄色波浪线显示；无任何诊断时面板提示"静态诊断通过"。
    remove(ERR_TXT); // 删除空的 error.txt（诊断过程会自行重建）
    RunStaticDiagnostics(true);
    return true;
  } else {
    // 编译失败：用静态诊断解析器解析 error.txt（带类型/列/波浪线）
    g_buildOk = false;
    ParseBuildErrorFile(ERR_TXT, srcPath);
    if (g_diagCount == 0) {
      // gcc 返回非 0 但 error.txt 为空：补一条失败提示
      DiagEntry fail;
      fail.line = 0;
      fail.col = 0;
      fail.colEnd = 0;
      fail.type = "error";
      fail.desc = "编译失败（无详细错误输出）";
      DiagAdd(fail);
    }
    remove(ERR_TXT);
    return false;
  }
}

// ============================================================
// 语法检测：静态诊断入口
// 优先 clang -fsyntax-only -fdiagnostics-format=json；
// clang 不可用时回退 gcc -fsyntax-only 文本输出解析。
// 结果以波浪线 + 问题面板展示（见 RunStaticDiagnostics）。
// 返回 true 表示无 error。
// ============================================================
bool SyntaxCheck() {
  bool hasError = RunStaticDiagnostics(false);
  return !hasError;
}

// ============================================================
// 运行编译产物（与源文件同目录、同主名的 .exe）
// 用 ShellExecuteA 异步启动一个 cmd 窗口运行 exe，运行后 pause 等待按键，
// 用户可以看到输出。工作目录设为源文件所在目录，程序内的相对路径
// （fopen 等）相对于其自身目录，与 DEV-C++ 一致。
// 用 ShellExecuteA 而非 system("start ...") 避免 system 自身创建的
// 中间 cmd.exe 控制台一闪而过。
// ============================================================
void RunProgram() {
  if (g_filePath.empty()) {
    MessageBoxA(GetHWnd(), "未找到可执行文件，请先编译。", "运行失败",
                MB_OK | MB_ICONWARNING);
    return;
  }

  std::string exePath = ExePathForSource(g_filePath);

  // 检查 exe 是否存在
  std::ifstream f(exePath.c_str(), std::ios::binary);
  if (!f.is_open()) {
    MessageBoxA(GetHWnd(), "未找到可执行文件，请先编译。", "运行失败",
                MB_OK | MB_ICONWARNING);
    return;
  }
  f.close();

  // 以源文件目录为工作目录，命令中使用带引号的 exe 文件名
  // & 保证即使程序崩溃，pause 也会执行
  std::string dir = FolderOfFile(g_filePath);
  std::string exeName = ExtractFileName(exePath);
  std::string params = "/C title Mini-C 程序输出 & \"" + exeName + "\" & pause";
  ShellExecuteA(GetHWnd(), "open", "cmd.exe", params.c_str(),
                dir.empty() ? NULL : dir.c_str(), SW_SHOWNORMAL);
}

// ============================================================
// 编译并运行：先编译，成功才运行；失败则不运行，错误已在右侧诊断面板
// ============================================================
void TestCompileAndRun() {
  if (CompileCurrent()) {
    RunProgram();
  }
}

// ============================================================
// 变量声明结构：记录变量名、类型、是否在断点前已赋值
// ============================================================
struct VarDecl {
  std::string name;
  std::string type;   // "int","char","float","double","long","short","unsigned"
  bool assigned;      // 断点行之前是否被赋值
};

// 判断一个字符串是否为 C 关键字（非变量名）——调试专用版本
static bool IsCKeywordDbg(const std::string &s) {
  static const char *keywords[] = {
      "int", "return", "if", "else", "for", "while", "do", "break",
      "continue", "switch", "case", "default", "void", "char", "float",
      "double", "long", "short", "unsigned", "signed", "const", "static",
      "struct", "union", "enum", "typedef", "sizeof", "main", "printf",
      "scanf", "getchar", "putchar", "include", "define", "NULL", "auto",
      "register", "volatile", "extern", "goto", "inline", "restrict",
      "_Bool", "_Complex", "_Imaginary"};
  int kwCount = sizeof(keywords) / sizeof(keywords[0]);
  for (int k = 0; k < kwCount; k++)
    if (s == keywords[k]) return true;
  return false;
}

// 判断字符是否可作为标识符的一部分——调试专用版本
static bool IsIdentCharDbg(char c) {
  return isalnum((unsigned char)c) || c == '_';
}

// 从 position 开始提取一个标识符
static std::string ReadIdent(const std::string &s, size_t &pos) {
  std::string id;
  while (pos < s.size() && IsIdentCharDbg(s[pos]))
    id += s[pos++];
  return id;
}

// 跳过空白
static void SkipWs(const std::string &s, size_t &pos) {
  while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t'))
    pos++;
}

// 判断变量 var 在 [declRow, bpRow) 区间内是否被赋值
// 赋值特征：var= / var+= / var++ / var-- / ++var / --var / &var 在 scanf 类函数中
static bool IsVarAssignedBefore(const std::string &var, int declRow, int bpRow) {
  for (int r = declRow; r < bpRow && r < g_buffer.getRowCount(); r++) {
    std::string line = g_buffer.getLine(r);
    size_t pos = 0;
    while (pos < line.size()) {
      size_t found = line.find(var, pos);
      if (found == std::string::npos) break;
      // 确认是完整标识符边界
      bool leftOk = (found == 0 || !IsIdentCharDbg(line[found - 1]));
      bool rightOk = (found + var.size() >= line.size() ||
                      !IsIdentCharDbg(line[found + var.size()]));
      if (leftOk && rightOk) {
        size_t after = found + var.size();
        // 检查赋值/自增自减运算符
        SkipWs(line, after);
        if (after < line.size()) {
          char c = line[after];
          // var = ... / var += ... 等
          if (c == '=') return true;
          if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%') {
            if (after + 1 < line.size() && line[after + 1] == '=')
              return true;
            // var++ / var--
            if ((c == '+' || c == '-') && after + 1 < line.size() &&
                line[after + 1] == c)
              return true;
          }
        }
        // 检查 ++var / --var
        if (found >= 2) {
          char c1 = line[found - 1], c2 = line[found - 2];
          if ((c1 == '+' && c2 == '+') || (c1 == '-' && c2 == '-'))
            return true;
        }
        // 检查 &var 出现在 scanf 类调用中（粗略：含 "scan" 或 "scanf"）
        if (found >= 1 && line[found - 1] == '&') {
          // 往前看是否有 scanf/sscanf/fscanf
          std::string before = line.substr(0, found);
          if (before.find("scanf") != std::string::npos ||
              before.find("scanf_s") != std::string::npos)
            return true;
        }
      }
      pos = found + var.size();
    }
  }
  return false;
}

// 根据变量类型返回 printf 格式符
static const char *FormatForType(const std::string &type) {
  if (type == "long") return "%ld";
  if (type == "unsigned" || type == "unsigned int" || type == "unsigned short")
    return "%u";
  if (type == "unsigned long") return "%lu";
  if (type == "char") return "%d";       // char 用 %d 输出 ASCII 值，避免不可见字符破坏格式
  if (type == "float" || type == "double") return "%f";
  return "%d"; // int / short / 默认
}

// ============================================================
// 变量提取：扫描断点行之前若干行，提取所有仍在作用域内的基本类型变量，
// 并静态分析每个变量在断点前是否已被赋值。
// 支持：int / char / float / double / long / short / unsigned 系列
// 作用域追踪：遇到 { 开新作用域，遇到 } 关闭，只保留当前仍在作用域内的变量
// ============================================================
std::vector<VarDecl> ExtractVars(int bpRow) {
  // 作用域栈：每个元素是该层声明的变量列表
  std::vector<std::vector<VarDecl> > scopeStack;
  scopeStack.push_back(std::vector<VarDecl>()); // 全局作用域（第 0 层）
  // 标记每层是否为 for 循环变量作用域（for(int i;...) 的 i 所在层）
  std::vector<bool> scopeIsFor;
  scopeIsFor.push_back(false);

  static const char *typeNames[] = {
      "unsigned long", "unsigned int", "unsigned short",
      "unsigned", "long", "short", "double", "float", "char", "int"};
  int tnCount = sizeof(typeNames) / sizeof(typeNames[0]);

  int startRow = (bpRow > 80) ? bpRow - 80 : 0;
  for (int r = startRow; r < bpRow && r < g_buffer.getRowCount(); r++) {
    std::string line = g_buffer.getLine(r);
    size_t pos = 0;

    while (pos < line.size()) {
      char ch = line[pos];

      // 跳过字符串字面量
      if (ch == '"') {
        pos++;
        while (pos < line.size() && line[pos] != '"') {
          if (line[pos] == '\\' && pos + 1 < line.size()) pos += 2;
          else pos++;
        }
        if (pos < line.size()) pos++;
        continue;
      }
      // 跳过字符常量
      if (ch == '\'') {
        pos++;
        while (pos < line.size() && line[pos] != '\'') {
          if (line[pos] == '\\' && pos + 1 < line.size()) pos += 2;
          else pos++;
        }
        if (pos < line.size()) pos++;
        continue;
      }
      // 跳过行注释
      if (ch == '/' && pos + 1 < line.size() && line[pos + 1] == '/') {
        break;
      }
      // 跳过块注释（单行内）
      if (ch == '/' && pos + 1 < line.size() && line[pos + 1] == '*') {
        pos += 2;
        while (pos + 1 < line.size() && !(line[pos] == '*' && line[pos + 1] == '/'))
          pos++;
        if (pos + 1 < line.size()) pos += 2;
        continue;
      }
      // 检测 for (：为 for 循环变量开一个独立作用域
      if (ch == 'f' && pos + 4 <= line.size() &&
          line.compare(pos, 4, "for ") == 0) {
        // 确认 for 后面有 (
        size_t after = pos + 3;
        while (after < line.size() && line[after] == ' ') after++;
        if (after < line.size() && line[after] == '(') {
          scopeStack.push_back(std::vector<VarDecl>());
          scopeIsFor.push_back(true);
          pos = after + 1; // 跳过 (
          continue;
        }
      }
      // 开新作用域
      if (ch == '{') {
        scopeStack.push_back(std::vector<VarDecl>());
        scopeIsFor.push_back(false);
        pos++;
        continue;
      }
      // 关闭作用域：弹出该层所有变量；
      // 若弹出的是普通块作用域且上一层是 for 作用域，则 for 变量也随之失效，一并弹出
      if (ch == '}') {
        if (scopeStack.size() > 1) {
          bool poppedFor = scopeIsFor.back();
          scopeStack.pop_back();
          scopeIsFor.pop_back();
          // 如果弹出的不是 for 作用域，且当前顶层是 for 作用域，说明 for 循环体结束，for 变量也失效
          if (!poppedFor && scopeIsFor.size() > 1 && scopeIsFor.back()) {
            scopeStack.pop_back();
            scopeIsFor.pop_back();
          }
        }
        pos++;
        continue;
      }
      // 跳过空白
      if (ch == ' ' || ch == '\t') {
        pos++;
        continue;
      }

      // 尝试匹配类型关键字
      std::string matchedType;
      size_t typeEnd = pos;
      for (int t = 0; t < tnCount; t++) {
        size_t tlen = strlen(typeNames[t]);
        if (pos + tlen <= line.size() &&
            line.compare(pos, tlen, typeNames[t]) == 0) {
          // 类型关键字右边必须是空白或 *
          if (pos + tlen < line.size() &&
              (line[pos + tlen] == ' ' || line[pos + tlen] == '\t' ||
               line[pos + tlen] == '*')) {
            matchedType = typeNames[t];
            typeEnd = pos + tlen;
            break;
          }
        }
      }

      if (matchedType.empty()) {
        // 不是类型关键字，跳过到下一个 token 边界
        pos++;
        continue;
      }

      // 解析类型后的变量列表
      size_t p = typeEnd;
      while (p < line.size() && line[p] == '*') p++; // 跳过指针星号

      while (p < line.size() && line[p] != ';' && line[p] != '{' &&
             line[p] != '}' && line[p] != ')') {
        SkipWs(line, p);
        if (p >= line.size() || line[p] == ';' || line[p] == '{' ||
            line[p] == '}' || line[p] == ')') break;

        std::string ident = ReadIdent(line, p);
        if (!ident.empty() && !IsCKeywordDbg(ident)) {
          size_t q = p;
          SkipWs(line, q);
          bool initedHere = (q < line.size() && line[q] == '=');
          bool assigned =
              initedHere || IsVarAssignedBefore(ident, r + 1, bpRow);
          VarDecl vd;
          vd.name = ident;
          vd.type = matchedType;
          vd.assigned = assigned;
          scopeStack.back().push_back(vd);
        }
        // 跳过到下一个逗号或语句结束符
        while (p < line.size() && line[p] != ',' && line[p] != ';' &&
               line[p] != '{' && line[p] != '}' && line[p] != ')')
          p++;
        if (p < line.size() && line[p] == ',') p++;
      }

      // 移动 pos 到语句结束处（分号或花括号或右括号）
      size_t endPos = line.find_first_of(";{}", pos);
      pos = (endPos == std::string::npos) ? line.size() : endPos;
      if (pos < line.size() && (line[pos] == '{' || line[pos] == '}')) {
        // 不移动 pos，让下一轮处理花括号
      } else if (pos < line.size() && line[pos] == ';') {
        pos++;
      }
    }
  }

  // 收集所有仍在作用域内的变量（从所有栈层收集）
  std::vector<VarDecl> decls;
  for (size_t i = 0; i < scopeStack.size(); i++) {
    for (size_t j = 0; j < scopeStack[i].size(); j++) {
      decls.push_back(scopeStack[i][j]);
    }
  }
  return decls;
}

// ============================================================
// 调试输出解析：从 debug_out.txt 解析 [DBG] 行
// 格式：[DBG] BP=行号 类型 变量名=值
//   已赋值：[DBG] BP=5 int x=10
//   未赋值：[DBG] BP=5 float y=__NA__
// ============================================================
void ParseDebugOutput() {
  g_varSnapshots.clear();
  std::string content = ReadFileAll("debug_out.txt");
  if (content.empty())
    return;

  size_t pos = 0;
  while (pos < content.size()) {
    size_t dbgPos = content.find("[DBG]", pos);
    if (dbgPos == std::string::npos)
      break;

    size_t lineEnd = content.find('\n', dbgPos);
    if (lineEnd == std::string::npos)
      lineEnd = content.size();
    std::string line = content.substr(dbgPos, lineEnd - dbgPos);

    VarSnapshot vs;
    vs.bpRow = 0;
    vs.assigned = true;

    // 找 BP=
    size_t bpPos = line.find("BP=");
    if (bpPos != std::string::npos) {
      size_t numStart = bpPos + 3;
      std::string numStr;
      while (numStart < line.size() && isdigit((unsigned char)line[numStart])) {
        numStr += line[numStart++];
      }
      vs.bpRow = atoi(numStr.c_str());
    }

    // 跳过 BP=数字 后的空格，读取类型 token
    size_t typeStart = (bpPos != std::string::npos) ? bpPos + 3 : 0;
    while (typeStart < line.size() &&
           (isdigit((unsigned char)line[typeStart]) || line[typeStart] == ' '))
      typeStart++;
    // typeStart 现在指向类型首字符
    size_t typeEnd = typeStart;
    while (typeEnd < line.size() && line[typeEnd] != ' ')
      typeEnd++;
    if (typeEnd > typeStart)
      vs.type = line.substr(typeStart, typeEnd - typeStart);

    // 找 varname=value（类型之后的第一个 =）
    size_t eqPos = line.find('=', typeEnd);
    if (eqPos != std::string::npos) {
      // varname 在 typeEnd 之后到 eqPos 之间（去掉前后空格）
      size_t vnStart = typeEnd;
      while (vnStart < eqPos && line[vnStart] == ' ') vnStart++;
      size_t vnEnd = eqPos;
      while (vnEnd > vnStart && line[vnEnd - 1] == ' ') vnEnd--;
      vs.varName = line.substr(vnStart, vnEnd - vnStart);

      vs.value = line.substr(eqPos + 1);
      // 去掉末尾的 \r
      if (!vs.value.empty() && vs.value[vs.value.size()-1] == '\r')
        vs.value.erase(vs.value.size()-1);

      // __NA__ 标记表示未赋值
      if (vs.value == "__NA__") {
        vs.assigned = false;
        vs.value = "";
      }
    }

    if (!vs.varName.empty())
      g_varSnapshots.push_back(vs);

    pos = lineEnd + 1;
  }
}

// 获取过滤后的快照列表
std::vector<int> GetFilteredSnapshots() {
  std::vector<int> indices;
  for (int i = 0; i < (int)g_varSnapshots.size(); i++) {
    if (g_watchFilter.empty() ||
        g_varSnapshots[i].varName.find(g_watchFilter) != std::string::npos) {
      indices.push_back(i);
    }
  }
  return indices;
}


void DebugRun() {
  // 无断点 → 直接编译并运行
  if (g_breakpoints.empty()) {
    MessageBoxA(GetHWnd(),
                "未设置任何断点。\n请在编辑区左侧断点栏点击设置断点。",
                "调试运行", MB_OK | MB_ICONINFORMATION);
    return;
  }

  // 确保源文件已保存
  if (!EnsureSavedForBuild())
    return;

  // 检查编译器
  if (!CheckGccAvailable()) {
    MessageBoxA(GetHWnd(),
                "编译器未找到，请安装 MinGW 并将其 bin 目录加入 PATH。",
                "调试运行", MB_OK | MB_ICONERROR);
    return;
  }

  // 构造调试版源码：在断点行之前插入暂停语句
  std::string debugSrc = "temp_debug.c";
  std::string debugExe = "temp_debug.exe";
  std::ofstream fout(debugSrc.c_str(), std::ios::binary);
  if (!fout.is_open()) {
    MessageBoxA(GetHWnd(), "无法写入调试临时文件", "调试运行",
                MB_OK | MB_ICONERROR);
    return;
  }

  int rows = g_buffer.getRowCount();
  for (int r = 0; r < rows; r++) {
    std::string line = g_buffer.getLine(r);
    // 在断点行前插入变量输出语句（不再用 getchar 暂停，程序自动运行到底）
    if (g_breakpoints.count(r)) {
      // 提取该断点作用域内的所有变量（含类型与赋值状态），插入 fprintf 输出
      std::vector<VarDecl> vars = ExtractVars(r);
      for (size_t i = 0; i < vars.size(); i++) {
        const VarDecl &vd = vars[i];
        char dbgStmt[512];
        if (vd.assigned) {
          // 已赋值：按类型输出运行时值
          // fprintf(stderr, "[DBG] BP=行号 类型 变量名=格式符\n", 变量名);
          const char *fmt = FormatForType(vd.type);
          sprintf(dbgStmt,
                  "fprintf(stderr, \"[DBG] BP=%d %s %s=%s\\n\", %s);\n"
                  "fflush(stderr);\n",
                  r + 1, vd.type.c_str(), vd.name.c_str(), fmt, vd.name.c_str());
        } else {
          // 未赋值：输出 __NA__ 标记，监视窗口显示"暂未赋值"
          sprintf(dbgStmt,
                  "fprintf(stderr, \"[DBG] BP=%d %s %s=__NA__\\n\");\n"
                  "fflush(stderr);\n",
                  r + 1, vd.type.c_str(), vd.name.c_str());
        }
        fout.write(dbgStmt, strlen(dbgStmt));
      }
    }
    // 写原始行
    fout.write(line.c_str(), line.size());
    fout.put('\n');
  }
  fout.close();

  // 编译调试版
  std::string gccFull = QuoteArg(g_gccPath);
  // -fexec-charset=GBK：插桩源与用户源同为 UTF-8，统一转 GBK 输出，
  // 保证用户 printf 中文在监视窗口/控制台不乱码
  std::string args = std::string("-std=gnu99 -fexec-charset=GBK ") +
                     QuoteArg(debugSrc) + " -o " + QuoteArg(debugExe);
  int ret = RunDirect(gccFull.c_str(), args.c_str(), ERR_TXT);
  std::string err = ReadFileAll(ERR_TXT);

  if (ret != 0 || !err.empty()) {
    // 编译失败：用静态诊断解析器解析 error.txt
    g_buildOk = false;
    ParseBuildErrorFile(ERR_TXT, debugSrc);
    // 截取错误信息前 500 字显示给用户，方便定位
    std::string shortErr = err;
    if (shortErr.size() > 500) shortErr = shortErr.substr(0, 500) + "...";
    std::string msg = "调试版编译失败：\n" + shortErr +
                      "\n\n可能原因：断点行不在有效语句位置，或断点作用域外的变量被引用。";
    MessageBoxA(GetHWnd(), msg.c_str(), "调试运行", MB_OK | MB_ICONERROR);
    remove(ERR_TXT);
    return;
  }

  // 运行调试版：无窗口运行，stderr→debug_out.txt，stdout→debug_stdout.txt，等待结束
  remove("debug_out.txt");
  remove("debug_stdout.txt");
  RunDirect(QuoteArg(debugExe).c_str(), "", "debug_out.txt", "debug_stdout.txt");

  // 保存程序输出到全局变量，供监视窗口显示
  g_programOutput = ReadFileAll("debug_stdout.txt");

  g_buildOk = true;

  // 解析调试输出，填充 g_varSnapshots
  ParseDebugOutput();

  // 打开变量监视窗口
  g_watchWindowOpen = true;
  g_watchX = (WINDOW_WIDTH - WATCH_W) / 2;
  g_watchY = MENU_BAR_HEIGHT + 30;
  g_watchScroll = 0;
  g_watchFilter = "";
  g_watchInputBuf[0] = '\0';
  g_watchInputLen = 0;
  g_watchInputActive = false;
  g_watchInputCursorPos = 0;
}

// ============================================================
// 处理键盘输入（基于 GetAsyncKeyState 轮询）
// 返回值：true 表示发生了编辑（需要重绘 + 状态变"已修改"）
//         false 表示仅光标移动或无操作
// 根本原因：EasyX 内部有自己的消息循环，会消化掉 WM_KEYDOWN/WM_CHAR，
//           PeekMessage 拿不到。改用 GetAsyncKeyState 直接查询按键状态，
//           配合"按下沿"检测（prev=false && now=true）确保只触发一次。
// ============================================================

// 全局：上一帧按键状态（true=按下）
static bool g_keyState[256] = {false};

// 检查某键是否"刚刚按下"（按下沿检测，避免连续重复触发）
bool KeyJustPressed(int vk) {
  bool now = (GetAsyncKeyState(vk) & 0x8000) != 0;
  bool pressed = (now && !g_keyState[vk]);
  g_keyState[vk] = now;
  return pressed;
}

// ============================================================
// 长按重复触发器（用于删除键连续删除）
// 首次按下立即触发一次；持续按住超过 initDelay 后，
// 每隔 repeatInterval 触发一次；释放后复位状态。
// 与 KeyJustPressed 互不干扰：内部维护独立状态数组。
// ============================================================
bool KeyRepeat(int vk, DWORD initDelay = 400, DWORD repeatInterval = 50) {
  static struct KeyRepState {
    bool active;       // 当前是否处于按下状态
    DWORD pressTime;   // 本次按下的起始时刻
    DWORD lastTrigger; // 上次触发时刻
  } state[256] = {{false, 0, 0}};

  bool now = (GetAsyncKeyState(vk) & 0x8000) != 0;
  DWORD nowTime = GetTickCount();

  // 按下沿：立即触发一次
  if (now && !state[vk].active) {
    state[vk].active = true;
    state[vk].pressTime = nowTime;
    state[vk].lastTrigger = nowTime;
    return true;
  }
  // 释放沿：复位
  if (!now && state[vk].active) {
    state[vk].active = false;
    return false;
  }
  // 持续按下：超过初始延迟后，按间隔重复触发
  if (now && state[vk].active) {
    if (nowTime - state[vk].pressTime >= initDelay &&
        nowTime - state[vk].lastTrigger >= repeatInterval) {
      state[vk].lastTrigger = nowTime;
      return true;
    }
  }
  return false;
}

// ============================================================
// 全局低级键盘钩子（WH_KEYBOARD_LL）
// ------------------------------------------------------------
// 实测（探针 A~F）：本版 EasyX 的图形窗口运行在内部 GUI 线程 ——
//   - 键盘输入只送到该内部线程，主线程 PeekMessage 取不到任何窗口消息；
//   - 主线程 GetAsyncKeyState 也读不到击键（探针 F 零命中）；
//   - 内部消息环转发的键盘消息：WM_IME_CHAR 被丢弃、WM_KEYDOWN 不带
//     vkcode，无法翻译。
// 唯一能在主线程稳定拿到全部击键（真实键盘与程序注入均可）的通道是
// WH_KEYBOARD_LL：钩子回调在"安装线程（即主线程）的消息循环"中分发，
// 所以主循环每帧必须 PeekMessage 驱动回调（见 main）。
// 回调只做轻量工作：维护修饰键状态（Shift/Ctrl/Alt/Caps）+ 击键入环形
// 缓冲；主循环逐帧取出用 ToAscii 翻译 —— 标点键的 VK 码是 VK_OEM_*
// （','=0xBC、'.'=0xBE、'/'=0xBF、';'=0xBA 等），翻译后才能得到
// < > : " | { } , . / ; 等符号（旧代码按 ASCII 轮询永远匹配不到）。
// ============================================================

struct LowLevelKeyEvent {
  BYTE vk;   // 虚拟键码
  BYTE scan; // 硬件扫描码（ToAscii 需要）
  bool down; // true=按下（含长按重复），false=抬起
};

static HHOOK g_llHook = NULL;
static BYTE g_llKb[256] = {0}; // 钩子维护的键盘状态表
static LowLevelKeyEvent g_llBuf[64];
static int g_llHead = 0, g_llTail = 0;

// WM_CHAR 环 与 钩子两条字符通道的去重时间戳（同一击键两条通道都会产出
// 字符时，后到的一条在去重窗口内被丢弃，避免 "aa" 双字）
static DWORD g_lastRingCharTick = 0; // 上次从消息环插入字符的时刻
static DWORD g_lastHookCharTick = 0; // 上次从钩子翻译插入字符的时刻
// 实测环消息滞后钩子事件约 47ms（内部线程桥接延迟），窗口取 80ms 覆盖
#define CHAR_DEDUP_WINDOW_MS 80

static bool LLEnqueue(BYTE vk, BYTE scan, bool down) {
  int next = (g_llTail + 1) % 64;
  if (next == g_llHead)
    return false; // 缓冲满（10ms 帧间隔下不会发生）
  g_llBuf[g_llTail].vk = vk;
  g_llBuf[g_llTail].scan = scan;
  g_llBuf[g_llTail].down = down;
  g_llTail = next;
  return true;
}

static bool LLDequeue(LowLevelKeyEvent &out) {
  if (g_llHead == g_llTail)
    return false;
  out = g_llBuf[g_llHead];
  g_llHead = (g_llHead + 1) % 64;
  return true;
}

LRESULT CALLBACK LowLevelKeyProc(int code, WPARAM wp, LPARAM lp) {
  // 钩子是全局的，只处理本窗口在前台时的击键
  if (code == HC_ACTION && GetForegroundWindow() == GetHWnd()) {
    KBDLLHOOKSTRUCT *kb = (KBDLLHOOKSTRUCT *)lp;
    bool down = (wp == WM_KEYDOWN || wp == WM_SYSKEYDOWN);
    BYTE vk = (BYTE)kb->vkCode;
    BYTE scan = (BYTE)kb->scanCode;

    // F11：切换全屏（按下沿触发，不进常规键队列）
    if (vk == VK_F11 && down) {
      ToggleFullScreen();
      // 吞下本击键，不传给系统/其他钩子
      return 1;
    }

    // 修饰键（左右变体归一）同步到状态表
    BYTE canon = vk;
    if (vk == VK_LSHIFT || vk == VK_RSHIFT)
      canon = VK_SHIFT;
    if (vk == VK_LCONTROL || vk == VK_RCONTROL)
      canon = VK_CONTROL;
    if (vk == VK_LMENU || vk == VK_RMENU)
      canon = VK_MENU;
    if (canon == VK_SHIFT || canon == VK_CONTROL || canon == VK_MENU) {
      BYTE bit = down ? 0x80 : 0x00;
      g_llKb[vk] = bit;
      g_llKb[canon] = bit;
    } else if (vk == VK_CAPITAL) {
      if (down)
        g_llKb[VK_CAPITAL] ^= 0x01; // CapsLock 按下沿翻转
    } else {
      LLEnqueue(vk, scan, down);
    }
  }
  return CallNextHookEx(g_llHook, code, wp, lp);
}

// ============================================================
// 可打印字符输入辅助
// ============================================================

// 插入一个可打印字符（有选区时先删选区）；返回 true 表示发生了编辑
// 控制字符（回车/退格/Tab/Ctrl 组合）由 HandleKeyboard 的 VK 分支处理
bool InsertTypedChar(char ch) {
  if ((unsigned char)ch < 0x20)
    return false;
  // 连续打字在 800ms 内合并为一个撤销步骤
  UndoBeginAction(true);
  bool crossLine = false;
  if (g_sel.active) {
    NormalizeSel();
    crossLine = (g_sel.startRow != g_sel.endRow);
    DeleteSelContent();
    g_sel.active = false;
  }
  g_buffer.insertChar(g_cursor.row, g_cursor.col, ch);
  g_cursor.col++;
  // 普通打字不破坏折叠；仅当替换了跨多行选区（行数变化）时展开
  MarkFoldsDirty(crossLine);
  return true;
}

// 查找弹窗输入用：边沿检测可打印键（美式布局），返回对应 ASCII；无新按下返回
// -1。 用 KeyJustPressed：每次物理按下只产生一个字符（KeyRepeat 在短按跨两帧时
// 会触发两次，是"输入一次出两个"的根因）。长按连发由钩子/WM_CHAR 通道
// 按系统重复率自然补充。
static int FindBoxVKeyChar(bool shift) {
  bool caps = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;

  for (int vk = 'A'; vk <= 'Z'; vk++) {
    if (KeyJustPressed(vk)) {
      bool up = (shift != caps); // Shift 与 CapsLock 异或决定大小写
      return up ? vk : (vk - 'A' + 'a');
    }
  }
  // 数字行 + Shift 符号（美式布局）：) ! @ # $ % ^ & * (
  static const char digShift[11] = ")!@#$%^&*(";
  for (int vk = '0'; vk <= '9'; vk++) {
    if (KeyJustPressed(vk))
      return shift ? digShift[vk - '0'] : (char)vk;
  }

  if (KeyJustPressed(VK_SPACE))
    return ' ';
  if (KeyJustPressed(VK_OEM_1))
    return shift ? ':' : ';';
  if (KeyJustPressed(VK_OEM_PLUS))
    return shift ? '+' : '=';
  if (KeyJustPressed(VK_OEM_COMMA))
    return shift ? '<' : ',';
  if (KeyJustPressed(VK_OEM_MINUS))
    return shift ? '_' : '-';
  if (KeyJustPressed(VK_OEM_PERIOD))
    return shift ? '>' : '.';
  if (KeyJustPressed(VK_OEM_2))
    return shift ? '?' : '/';
  if (KeyJustPressed(VK_OEM_3))
    return shift ? '~' : '`';
  if (KeyJustPressed(VK_OEM_4))
    return shift ? '{' : '[';
  if (KeyJustPressed(VK_OEM_5))
    return shift ? '|' : '\\';
  if (KeyJustPressed(VK_OEM_6))
    return shift ? '}' : ']';
  if (KeyJustPressed(VK_OEM_7))
    return shift ? '"' : '\'';
  return -1;
}

bool HandleKeyboard() {
  // 窗口必须在前台才处理键盘（避免抢输入）
  if (GetForegroundWindow() != GetHWnd())
    return false;

  // Esc：关闭已打开的菜单/函数列表/补全弹窗/查找弹窗
  if (KeyJustPressed(VK_ESCAPE)) {
    if (g_funcListOpen) {
      g_funcListOpen = false;
      g_funcListHover = -1;
    }
    if (g_autocompleteOpen)
      CloseAutocomplete();
    if (g_activeMenu >= 0) {
      g_activeMenu = -1;
      g_subMenuHover = -1;
    }
    if (g_ctxMenuOpen) {
      g_ctxMenuOpen = false;
      g_ctxMenuHoverIdx = -1;
    }
    // 替换输入框聚焦时：第一次 Esc 仅让替换框失焦（按键回到查找框/编辑器）
    if (g_findRepFocused) {
      g_findRepFocused = false;
      return false;
    }
    if (g_findBoxOpen)
      CloseFindBox();
    if (g_helpOpen)
      g_helpOpen = false;
    if (g_aboutOpen)
      g_aboutOpen = false;
    return false;
  }

  // 帮助 / 关于窗口打开时为模态：屏蔽编辑区与其余快捷键
  if (g_helpOpen || g_aboutOpen)
    return false;

  // === 变量监视窗口输入框激活时接管按键 ===
  if (g_watchWindowOpen && g_watchInputActive) {
    bool shiftDown = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

    if (KeyJustPressed(VK_LEFT)) {
      if (g_watchInputCursorPos > 0) g_watchInputCursorPos--;
      return false;
    }
    if (KeyJustPressed(VK_RIGHT)) {
      if (g_watchInputCursorPos < g_watchInputLen) g_watchInputCursorPos++;
      return false;
    }
    if (KeyJustPressed(VK_HOME)) {
      g_watchInputCursorPos = 0;
      return false;
    }
    if (KeyJustPressed(VK_END)) {
      g_watchInputCursorPos = g_watchInputLen;
      return false;
    }
    if (KeyJustPressed(VK_BACK)) {
      if (g_watchInputCursorPos > 0) {
        memmove(g_watchInputBuf + g_watchInputCursorPos - 1,
                g_watchInputBuf + g_watchInputCursorPos,
                g_watchInputLen - g_watchInputCursorPos);
        g_watchInputLen--;
        g_watchInputCursorPos--;
        g_watchInputBuf[g_watchInputLen] = '\0';
      }
      return false;
    }
    if (KeyJustPressed(VK_DELETE)) {
      if (g_watchInputCursorPos < g_watchInputLen) {
        memmove(g_watchInputBuf + g_watchInputCursorPos,
                g_watchInputBuf + g_watchInputCursorPos + 1,
                g_watchInputLen - g_watchInputCursorPos);
        g_watchInputLen--;
        g_watchInputBuf[g_watchInputLen] = '\0';
      }
      return false;
    }
    if (KeyJustPressed(VK_RETURN)) {
      // Enter = "查看"
      g_watchFilter = g_watchInputLen > 0 ? g_watchInputBuf : "";
      g_watchScroll = 0;
      return false;
    }
    // 可打印字符输入
    for (int c = 0x20; c <= 0x7E; c++) {
      if (KeyJustPressed(c)) {
        if (g_watchInputLen < 255) {
          memmove(g_watchInputBuf + g_watchInputCursorPos + 1,
                  g_watchInputBuf + g_watchInputCursorPos,
                  g_watchInputLen - g_watchInputCursorPos);
          g_watchInputBuf[g_watchInputCursorPos] = (char)c;
          g_watchInputLen++;
          g_watchInputCursorPos++;
          g_watchInputBuf[g_watchInputLen] = '\0';
        }
        return false;
      }
    }
    return false; // 吞掉其他键
  }

  // === 查找弹窗/替换输入框激活时接管按键：输入进弹窗，编辑器不响应 ===
  if (g_findBoxOpen || g_findRepFocused) {
    bool shiftDown = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    bool ctrlDown = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;

    // 光标移动（Shift+方向键扩展选区；无 Shift 时先收起选区）。
    // 操作对象为键盘焦点字段 FieldText()。
    if (KeyJustPressed(VK_LEFT)) {
      if (shiftDown) {
        if (g_findSelAnchor < 0)
          g_findSelAnchor = g_findCursor;
        g_findCursor = FindBoxSnap(FieldText(), g_findCursor - 1);
      } else {
        if (g_findSelAnchor >= 0) // 有选区：光标跳到选区左端
          g_findCursor =
              (g_findSelAnchor < g_findCursor) ? g_findSelAnchor : g_findCursor;
        else
          g_findCursor = FindBoxSnap(FieldText(), g_findCursor - 1);
        g_findSelAnchor = -1;
      }
      g_findBoxDirty = true;
      return false;
    }
    if (KeyJustPressed(VK_RIGHT)) {
      if (shiftDown) {
        if (g_findSelAnchor < 0)
          g_findSelAnchor = g_findCursor;
        g_findCursor = FindBoxSnapNext(FieldText(), g_findCursor + 1);
      } else {
        if (g_findSelAnchor >= 0) // 有选区：光标跳到选区右端
          g_findCursor =
              (g_findCursor > g_findSelAnchor) ? g_findCursor : g_findSelAnchor;
        else
          g_findCursor = FindBoxSnapNext(FieldText(), g_findCursor + 1);
        g_findSelAnchor = -1;
      }
      g_findBoxDirty = true;
      return false;
    }
    if (KeyJustPressed(VK_HOME)) {
      if (shiftDown && g_findSelAnchor < 0)
        g_findSelAnchor = g_findCursor;
      g_findCursor = 0;
      if (!shiftDown)
        g_findSelAnchor = -1;
      g_findBoxDirty = true;
      return false;
    }
    if (KeyJustPressed(VK_END)) {
      if (shiftDown && g_findSelAnchor < 0)
        g_findSelAnchor = g_findCursor;
      g_findCursor = (int)FieldText().size();
      if (!shiftDown)
        g_findSelAnchor = -1;
      g_findBoxDirty = true;
      return false;
    }

    if (KeyJustPressed(VK_BACK)) {
      FindBoxBackspace();
      return false;
    }
    if (KeyJustPressed(VK_DELETE)) {
      FindBoxDeleteKey();
      return false;
    }
    if (KeyJustPressed(VK_RETURN)) {
      FindBoxConfirm();
      return false;
    }

    // Tab：在查找框和替换框之间切换焦点
    if (KeyJustPressed(VK_TAB)) {
      FindBoxFocusField(1 - g_findBoxField);
      return false;
    }

    // Ctrl+A 全选（作用于当前焦点字段）
    if (ctrlDown && KeyJustPressed(0x41)) {
      g_findSelAnchor = 0;
      g_findCursor = (int)FieldText().size();
      g_findBoxDirty = true;
      return false;
    }

    // Ctrl 组合不输入字符：Ctrl+F 开/关弹窗，其余忽略
    if (ctrlDown) {
      if (KeyJustPressed(0x46))
        CloseFindBox(); // 'F'
      return false;
    }

    // 可打印字符：边沿检测，一次物理按下只收一个（长按连发由
    // 钩子/WM_CHAR 通道按系统重复率补充）
    int ch = FindBoxVKeyChar(shiftDown);
    if (ch > 0)
      FindBoxInsertChar((char)ch);
    return false;
  }

  // === 代码补全弹窗的键盘导航（优先于普通编辑键）===
  if (g_autocompleteOpen) {
    if (KeyJustPressed(VK_DOWN)) {
      if (g_autocompleteHover < (int)g_autocompleteItems.size() - 1)
        g_autocompleteHover++;
      return false;
    }
    if (KeyJustPressed(VK_UP)) {
      if (g_autocompleteHover > 0)
        g_autocompleteHover--;
      return false;
    }
    if (KeyJustPressed(VK_TAB) || KeyJustPressed(VK_RETURN)) {
      AcceptAutocomplete();
      return true;
    }
    // 左右方向键 / Home / End：光标水平移动，关闭补全
    if (KeyJustPressed(VK_LEFT) || KeyJustPressed(VK_RIGHT) ||
        KeyJustPressed(VK_HOME) || KeyJustPressed(VK_END)) {
      CloseAutocomplete();
      // 不 return，让方向键继续执行正常的光标移动
    }
  }

  bool edited = false;
  bool moved = false;
  int rows = g_buffer.getRowCount();
  int curCols = g_buffer.getColCount(g_cursor.row);

  // === Ctrl 快捷键（最先处理，命中后直接 return，避免被字符输入处理）===
  bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
  if (ctrl) {
    // Ctrl+N：新建
    if (KeyJustPressed(0x4E)) { // 'N'
      FileNew();
      return edited;
    }
    // Ctrl+O：打开
    if (KeyJustPressed(0x4F)) { // 'O'
      FileOpen();
      return edited;
    }
    // Ctrl+S：保存
    if (KeyJustPressed(0x53)) { // 'S'
      FileSave();
      return edited;
    }
    // Ctrl+Shift+S：另存为（Shift 也按下时）
    bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    if (shift && KeyJustPressed(0x53)) {
      FileSaveAs();
      return edited;
    }
    // Ctrl+Shift+[：全部折叠；Ctrl+Shift+]：全部展开
    if (shift && KeyJustPressed(VK_OEM_4)) { // '['
      FoldAll();
      return edited; // 折叠不修改文档，仅触发重绘
    }
    if (shift && KeyJustPressed(VK_OEM_6)) { // ']'
      ExpandAll();
      return edited;
    }
    // Ctrl+T：切换亮色/暗色主题
    if (KeyJustPressed(0x54)) { // 'T'
      ToggleTheme();
      return edited; // 不修改文档，仅触发重绘
    }
    // Ctrl+Tab：循环切换标签
    if (KeyJustPressed(VK_TAB)) {
      CycleTab(1);
      return edited; // 不修改文档，仅触发重绘
    }
    // Ctrl+F9：编译（VK_F9 = 0x78），只编译不运行
    if (KeyJustPressed(0x78)) {
      CompileCurrent();
      return edited;
    }
    // Ctrl+F5 或 F10：编译并运行（失败不运行，错误显示在右侧）
    if (KeyJustPressed(0x79)) { // VK_F10
      TestCompileAndRun();
      return edited;
    }
    // F5：调试运行（生成带断点暂停的临时文件并运行）
    if (KeyJustPressed(0x74)) { // VK_F5
      DebugRun();
      return edited;
    }
    // Ctrl+A：全选（选区覆盖所有文本）
    if (KeyJustPressed(0x41)) { // 'A'
      g_sel.startRow = 0;
      g_sel.startCol = 0;
      g_sel.endRow = g_buffer.getRowCount() - 1;
      g_sel.endCol = g_buffer.getColCount(g_sel.endRow);
      g_sel.active = true;
      // 光标移到末尾
      g_cursor.row = g_sel.endRow;
      g_cursor.col = g_sel.endCol;
      return edited;
    }
    // Ctrl+C：复制选区
    if (KeyJustPressed(0x43)) { // 'C'
      EditCopy();
      return edited;
    }
    // Ctrl+V：粘贴（剪贴板为空不算编辑，避免幻影 modified 标记）
    if (KeyJustPressed(0x56)) { // 'V'
      return EditPaste();
    }
    // Ctrl+X：剪切
    if (KeyJustPressed(0x58)) { // 'X'
      return EditCut();         // 无选区不算编辑
    }
    // Ctrl+Z：撤销；Ctrl+Shift+Z：重做
    if (KeyJustPressed(0x5A)) { // 'Z'
      return shift ? EditRedo() : EditUndo();
    }
    // Ctrl+Y：重做
    if (KeyJustPressed(0x59)) { // 'Y'
      return EditRedo();
    }
    // Ctrl+F：打开/关闭查找弹窗（弹窗内回车或"开始查找"执行查找）
    if (KeyJustPressed(0x46)) { // 'F'
      if (g_findBoxOpen)
        CloseFindBox();
      else
        Find();
      return edited;
    }
    // Ctrl+Shift+G：转到函数（弹出函数列表，须在 Ctrl+G 之前判断）
    if (KeyJustPressed(0x47) && (GetAsyncKeyState(VK_SHIFT) & 0x8000)) {
      GoToFunction();
      return edited;
    }
    // Ctrl+G：转到行
    if (KeyJustPressed(0x47)) { // 'G'
      GoToLine();
      return edited;
    }
    // Ctrl 按住时直接 return，避免下面字符循环误触发
    return edited;
  }

  // === 方向键（按下沿触发）===
  // Shift 按下时扩展选区，否则清选区
  bool shiftDown = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
  // 方向移动以"可见行"为单位（折叠隐藏行被整体跳过）
  EnsureFolds();
  int curView = g_docToView[g_cursor.row];
  if (curView < 0)
    curView = 0;

  if (KeyJustPressed(VK_UP)) {
    if (shiftDown && !g_sel.active) {
      // 开始选区：起点 = 当前光标
      g_sel.startRow = g_cursor.row;
      g_sel.startCol = g_cursor.col;
      g_sel.active = true;
    }
    if (curView > 0) {
      g_cursor.row = g_viewToDoc[curView - 1];
      int newCols = g_buffer.getColCount(g_cursor.row);
      if (g_cursor.col > newCols)
        g_cursor.col = newCols;
      moved = true;
    }
    if (shiftDown && g_sel.active) {
      g_sel.endRow = g_cursor.row;
      g_sel.endCol = g_cursor.col;
    } else if (!shiftDown && g_sel.active) {
      g_sel.active = false;
    }
  }
  if (KeyJustPressed(VK_DOWN)) {
    if (shiftDown && !g_sel.active) {
      g_sel.startRow = g_cursor.row;
      g_sel.startCol = g_cursor.col;
      g_sel.active = true;
    }
    if (curView + 1 < (int)g_viewToDoc.size()) {
      g_cursor.row = g_viewToDoc[curView + 1];
      int newCols = g_buffer.getColCount(g_cursor.row);
      if (g_cursor.col > newCols)
        g_cursor.col = newCols;
      moved = true;
    }
    if (shiftDown && g_sel.active) {
      g_sel.endRow = g_cursor.row;
      g_sel.endCol = g_cursor.col;
    } else if (!shiftDown && g_sel.active) {
      g_sel.active = false;
    }
  }
  if (KeyJustPressed(VK_LEFT)) {
    if (shiftDown && !g_sel.active) {
      g_sel.startRow = g_cursor.row;
      g_sel.startCol = g_cursor.col;
      g_sel.active = true;
    }
    if (g_cursor.col > 0) {
      g_cursor.col--;
      moved = true;
    } else if (curView > 0) {
      g_cursor.row = g_viewToDoc[curView - 1]; // 上一可见行（跳过折叠区）
      g_cursor.col = g_buffer.getColCount(g_cursor.row);
      moved = true;
    }
    if (shiftDown && g_sel.active) {
      g_sel.endRow = g_cursor.row;
      g_sel.endCol = g_cursor.col;
    } else if (!shiftDown && g_sel.active) {
      g_sel.active = false;
    }
  }
  if (KeyJustPressed(VK_RIGHT)) {
    if (shiftDown && !g_sel.active) {
      g_sel.startRow = g_cursor.row;
      g_sel.startCol = g_cursor.col;
      g_sel.active = true;
    }
    if (g_cursor.col < curCols) {
      g_cursor.col++;
      moved = true;
    } else if (curView + 1 < (int)g_viewToDoc.size()) {
      g_cursor.row = g_viewToDoc[curView + 1]; // 下一可见行（整块跳过）
      g_cursor.col = 0;
      moved = true;
    }
    if (shiftDown && g_sel.active) {
      g_sel.endRow = g_cursor.row;
      g_sel.endCol = g_cursor.col;
    } else if (!shiftDown && g_sel.active) {
      g_sel.active = false;
    }
  }

  // === PageUp / PageDown：以可见行页为单位滚动并移动光标 ===
  // 与方向键不同：PageUp/Down 主要作用是"滚动"，光标跟随到该页顶/底，
  // 这样反复按 PageDown 能持续向下翻页（光标始终在新页顶部）。
  if (KeyJustPressed(VK_PRIOR)) { // PageUp
    int visLines = GetVisibleLineCount();
    int viewCnt = (int)g_viewToDoc.size();
    if (viewCnt > 0) {
      // 目标视图行：当前光标视图行 - 一页；若已在该页顶部，再滚一页
      int target = curView - visLines;
      if (target == curView)
        target = curView - 1; // 极小屏 / 已到顶再翻
      if (target < 0)
        target = 0;
      g_cursor.row = g_viewToDoc[target];
      int newCols = g_buffer.getColCount(g_cursor.row);
      if (g_cursor.col > newCols)
        g_cursor.col = newCols;
      g_sel.active = false;
      moved = true;
    }
  }
  if (KeyJustPressed(VK_NEXT)) { // PageDown
    int visLines = GetVisibleLineCount();
    int viewCnt = (int)g_viewToDoc.size();
    if (viewCnt > 0) {
      int target = curView + visLines;
      if (target == curView)
        target = curView + 1;
      if (target >= viewCnt)
        target = viewCnt - 1;
      g_cursor.row = g_viewToDoc[target];
      int newCols = g_buffer.getColCount(g_cursor.row);
      if (g_cursor.col > newCols)
        g_cursor.col = newCols;
      g_sel.active = false;
      moved = true;
    }
  }

  // === 编辑键 ===
  if (KeyRepeat(VK_DELETE)) {
    // 长按重复触发时，文本可能已变化，需重新计算行数与当前行列数
    rows = g_buffer.getRowCount();
    curCols = g_buffer.getColCount(g_cursor.row);
    // 有选区时删除整个选区
    if (g_sel.active) {
      NormalizeSel();
      // 整行选区（行号栏点击产生）：删除整行而非仅选区文本
      bool wholeLine = (g_sel.startRow == g_sel.endRow) &&
                       (g_sel.startCol == 0) &&
                       (g_sel.endCol == g_buffer.getColCount(g_sel.endRow));
      if (wholeLine) {
        int row = g_sel.startRow;
        UndoBeginAction(true);
        g_buffer.deleteLine(row);
        int newRows = g_buffer.getRowCount();
        // 调整光标：删除点上方行不变，等于/下方行上移
        if (g_cursor.row > row)
          g_cursor.row--;
        if (g_cursor.row >= newRows)
          g_cursor.row = newRows - 1;
        if (g_cursor.row < 0)
          g_cursor.row = 0;
        g_cursor.col = 0;
      } else {
        UndoBeginAction(true);
        DeleteSelContent();
      }
      g_sel.active = false;
      MarkFoldsDirty(true); // 选区删除可能跨行 → 展开
      edited = true;
    } else if (g_cursor.col < curCols) {
      UndoBeginAction(true); // 长按 Delete 连续删除合并为一步
      g_buffer.deleteChar(g_cursor.row, g_cursor.col);
      MarkFoldsDirty(false); // 行内删字符：保留折叠
      edited = true;
    } else if (g_cursor.row < rows - 1) {
      // 下一文档行若被折叠隐藏，先展开，避免误并折叠区内的行
      if (g_docToView[g_cursor.row + 1] < 0)
        ExpandFoldsContaining(g_cursor.row + 1);
      UndoBeginAction(true);
      int nextLen = g_buffer.getColCount(g_cursor.row + 1);
      for (int i = 0; i < nextLen; i++) {
        g_buffer.insertChar(g_cursor.row, g_cursor.col + i,
                            g_buffer.getChar(g_cursor.row + 1, i));
      }
      g_buffer.deleteLine(g_cursor.row + 1);
      MarkFoldsDirty(true); // 合并下一行 → 展开
      edited = true;
    }
  }
  if (KeyRepeat(VK_BACK)) {
    // 长按重复触发时，文本可能已变化，需重新计算行数与当前行列数
    rows = g_buffer.getRowCount();
    curCols = g_buffer.getColCount(g_cursor.row);
    // 有选区时删除整个选区
    if (g_sel.active) {
      UndoBeginAction(true);
      DeleteSelContent();
      g_sel.active = false;
      MarkFoldsDirty(true); // 选区删除可能跨行 → 展开
      edited = true;
    } else if (g_cursor.col > 0) {
      UndoBeginAction(true); // 长按 Backspace 连续删除合并为一步
      g_buffer.deleteChar(g_cursor.row, g_cursor.col - 1);
      g_cursor.col--;
      MarkFoldsDirty(false); // 行内删字符：保留折叠
      edited = true;
    } else if (g_cursor.row > 0) {
      // 上一文档行若被折叠隐藏，先展开，避免误并折叠区内的行
      if (g_docToView[g_cursor.row - 1] < 0)
        ExpandFoldsContaining(g_cursor.row - 1);
      UndoBeginAction(true);
      int prevLen = g_buffer.getColCount(g_cursor.row - 1);
      int curLen = g_buffer.getColCount(g_cursor.row);
      for (int i = 0; i < curLen; i++) {
        g_buffer.insertChar(g_cursor.row - 1, prevLen + i,
                            g_buffer.getChar(g_cursor.row, i));
      }
      g_buffer.deleteLine(g_cursor.row);
      g_cursor.row--;
      g_cursor.col = prevLen;
      MarkFoldsDirty(true); // 合并上一行 → 展开
      edited = true;
    }
    if (edited)
      UpdateAutocomplete(); // 退格后重新评估补全
  }
  if (KeyJustPressed(VK_RETURN)) {
    CloseAutocomplete();    // 换行后关闭补全
    UndoBeginAction(false); // 每次回车独立一步
    // 有选区时先删除选区再换行
    if (g_sel.active) {
      DeleteSelContent();
      g_sel.active = false;
    }
    g_buffer.insertLineBreak(g_cursor.row, g_cursor.col);
    g_cursor.row++;
    g_cursor.col = 0;
    MarkFoldsDirty(true); // 新起一行 → 展开
    edited = true;
  }

  // === 可打印字符由主循环的 WH_KEYBOARD_LL 钩子翻译处理 ===
  // 原因（探针 F 实测）：图形窗口在 EasyX 内部 GUI 线程，主线程
  // GetAsyncKeyState/GetKeyboardState 都读不到该窗口的击键，本函数
  // 无法完成字符翻译；字符输入已移到 main() 中对钩子事件的
  // ToAscii 处理（见 LowLevelKeyProc）。本函数保留方向键、Backspace、
  // Delete、Enter、Tab、Ctrl 快捷键等导航/编辑逻辑（物理键盘下
  // GetAsyncKeyState 对这些键仍有效）。

  return edited;
}

// ============================================================
// gutter 折叠标记命中测试
// mx/my 为窗口坐标；命中某个可折叠块起点时返回该起点"文档行"，否则 -1
// ============================================================
int HitTestFoldGutter(int mx, int my) {
  if (mx < LINE_NUM_WIDTH || mx >= TEXT_ORIGIN_X)
    return -1;
  EnsureFolds();
  if (g_viewToDoc.empty())
    return -1;
  int bottom = WINDOW_HEIGHT - STATUS_BAR_HEIGHT;
  if (my < TEXT_ORIGIN_Y || my >= bottom)
    return -1;
  int vr = (my - TEXT_ORIGIN_Y) / LINE_HEIGHT;
  if (vr < 0)
    vr = 0;
  if (vr >= (int)g_viewToDoc.size())
    vr = (int)g_viewToDoc.size() - 1;
  int docRow = g_viewToDoc[vr];
  for (size_t k = 0; k < g_folds.size(); k++)
    if (g_folds[k].start == docRow)
      return docRow;
  return -1;
}

// ============================================================
// 鼠标坐标 -> 编辑区行列（不改状态，行列为"文档行"）
// strictX=true：要求 mx 在编辑区横向范围内（用于点击判定）
// strictX=false：mx 越界时按列 clamp（用于拖拽，允许鼠标移出右边界仍选到行尾）
// clampY=true：my 越出编辑区上/下边界时按行 clamp 到首行/末行（用于拖拽，
//              鼠标拖到菜单栏/状态栏方向仍能把选区扩展到首行/末行）
// 返回 false 表示 my 不在编辑区纵向范围（且未要求 clampY）
// ============================================================
bool PixelToCell(int mx, int my, int &outRow, int &outCol, bool strictX = true,
                 bool clampY = false) {
  if (strictX && (mx < LINE_NUM_WIDTH || mx >= EDIT_AREA_WIDTH))
    return false;
  EnsureFolds();
  if (g_viewToDoc.empty())
    return false;

  int bottom = WINDOW_HEIGHT - STATUS_BAR_HEIGHT;
  int viewCnt = (int)g_viewToDoc.size();
  int visLines = GetVisibleLineCount();
  int vr;
  if (my < TEXT_ORIGIN_Y) {
    if (!clampY)
      return false;
    vr = g_topLine; // 向上拖出 → 当前最顶可见行
  } else if (my >= bottom) {
    if (!clampY)
      return false;
    vr = g_topLine + visLines - 1; // 向下拖出 → 当前最底可见行
    if (vr >= viewCnt)
      vr = viewCnt - 1;
  } else {
    vr = g_topLine + (my - TEXT_ORIGIN_Y) / LINE_HEIGHT; // 加垂直滚动偏移
  }
  if (vr < 0)
    vr = 0;
  if (vr >= viewCnt)
    vr = viewCnt - 1;

  int row = g_viewToDoc[vr]; // 视图行 → 文档行

  int col = g_leftCol + (mx - TEXT_ORIGIN_X) / CHAR_WIDTH; // 加水平滚动偏移
  int curCols = g_buffer.getColCount(row);
  if (col < 0)
    col = 0;
  if (col > curCols)
    col = curCols;

  outRow = row;
  outCol = col;
  return true;
}

// ============================================================
// 处理鼠标点击编辑区：移动光标并开始拖拽选区
// mx/my 为窗口内坐标
// 返回 true 表示需要重绘（点击必然重绘以清/设选区）
// ============================================================
bool HandleMouseClickInEditArea(int mx, int my) {
  int row, col;
  if (!PixelToCell(mx, my, row, col, true))
    return false;

  g_cursor.row = row;
  g_cursor.col = col;
  EnsureCursorVisible(); // 点击可能落在当前可见区外（如折叠展开后）

  g_highlightLine = -1; // 点击编辑区时清高亮行
  SetFocus(GetHWnd());  // 获取键盘焦点

  // 开始鼠标拖拽选区：锚点 = 当前光标位置
  g_mouseSelecting = true;
  g_selAnchorRow = row;
  g_selAnchorCol = col;
  g_sel.active = false;  // 单击先清选区，拖拽时再激活
  SetCapture(GetHWnd()); // 捕获鼠标，确保拖出窗口仍能收到 LBUTTONUP

  return true;
}

// ============================================================
// 右键上下文菜单：点击检测
// 返回 true 表示命中菜单项区域；outIdx 为项索引（含分隔线）
// 调用方需自行判断该项 action 是否为 0（分隔线不可点）
// ============================================================
bool HitTestContextMenu(int mx, int my, int &outIdx) {
  if (!g_ctxMenuOpen)
    return false;
  if (mx < g_ctxMenuX || mx >= g_ctxMenuX + g_ctxMenuW)
    return false;
  int top = g_ctxMenuY;
  int h = g_ctxItemCount * g_ctxMenuItemH;
  if (my < top || my >= top + h)
    return false;
  outIdx = (my - top) / g_ctxMenuItemH;
  return true;
}

// ============================================================
// 右键上下文菜单：执行指定项操作
// ============================================================
void ExecuteContextItem(int action) {
  switch (action) {
  case 7:
    EditUndo();
    break; // 撤销
  case 8:
    EditRedo();
    break; // 重做
  case 1:
    EditCut();
    break; // 剪切
  case 2:
    EditCopy();
    break; // 复制
  case 3:
    EditPaste();
    break; // 粘贴
  case 4:  // 删除选区
    if (g_sel.active) {
      UndoBeginAction(false);
      DeleteSelContent();
      g_sel.active = false;
      g_modified = true;
    }
    break;
  case 5: // 全选
    g_sel.startRow = 0;
    g_sel.startCol = 0;
    g_sel.endRow = g_buffer.getRowCount() - 1;
    g_sel.endCol = g_buffer.getColCount(g_sel.endRow);
    g_sel.active = true;
    g_cursor.row = g_sel.endRow;
    g_cursor.col = g_sel.endCol;
    break;
  case 6:
    Find();
    break;
  default:
    break;
  }
}

// ============================================================
// 右键上下文菜单：绘制
// ============================================================
void DrawContextMenu() {
  if (!g_ctxMenuOpen)
    return;

  int top = g_ctxMenuY;
  int h = g_ctxItemCount * g_ctxMenuItemH;

  // 菜单背景 + 边框
  setfillcolor(g_theme.dropBg);
  setlinecolor(g_theme.dropBorder);
  bar(g_ctxMenuX, top, g_ctxMenuX + g_ctxMenuW, top + h);
  rectangle(g_ctxMenuX, top, g_ctxMenuX + g_ctxMenuW, top + h);

  setbkcolor(g_theme.dropBg);
  settextstyle(14, 0, _T("宋体"));

  for (int i = 0; i < g_ctxItemCount; i++) {
    int y = top + i * g_ctxMenuItemH;

    // 分隔线
    if (g_ctxItems[i].action == 0) {
      setlinecolor(g_theme.dropSeparator);
      line(g_ctxMenuX + 4, y + g_ctxMenuItemH / 2, g_ctxMenuX + g_ctxMenuW - 4,
           y + g_ctxMenuItemH / 2);
      continue;
    }

    // 悬停高亮（仅启用项）
    if (i == g_ctxMenuHoverIdx && g_ctxItems[i].enabled) {
      setfillcolor(g_theme.highlight);
      bar(g_ctxMenuX + 1, y, g_ctxMenuX + g_ctxMenuW - 1, y + g_ctxMenuItemH);
    }

    settextcolor(g_ctxItems[i].enabled ? g_theme.dropText
                                       : g_theme.dropDisabled);
    OutText(g_ctxMenuX + 12, y + 4, g_ctxItems[i].label);
  }
}

// ============================================================
// 初始化文本（启动时空白，无示例代码）
// ============================================================
void InitSampleText() {
  g_buffer.setText("");
  g_cursor.row = 0;
  g_cursor.col = 0;
}

// ============================================================
// 主函数
// ============================================================
int main() {
  // 初始化图形窗口
  initgraph(WINDOW_WIDTH, WINDOW_HEIGHT);
  SetWindowText(GetHWnd(), _T("Mini-C Studio"));

  // 主动获取前台焦点和键盘焦点（关键！否则键盘无反应）
  HWND hWnd = GetHWnd();
  SetForegroundWindow(hWnd);

  // 启用边框拉伸 + 最大化按钮：替换窗口样式为可调整尺寸
  {
    LONG style = GetWindowLong(hWnd, GWL_STYLE);
    style |= WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX;
    SetWindowLong(hWnd, GWL_STYLE, style);
    // SWP_FRAMECHANGED 让样式生效，SWP_DRAWFRAME 重绘边框
    SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED |
                     SWP_DRAWFRAME);
  }

  // 子类化窗口过程：拦截 WM_SIZE / WM_GETMINMAXINFO
  g_oldWndProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC,
                                           (LONG_PTR)SubclassedWndProc);
  // 初始布局（更新一次派生量）
  {
    RECT cr;
    GetClientRect(hWnd, &cr);
    UpdateLayout(cr.right, cr.bottom);
    g_pendingW = cr.right;
    g_pendingH = cr.bottom;
  }
  SetFocus(hWnd);
  SetActiveWindow(hWnd);

  // 布局菜单项坐标
  int x = 5;
  for (int i = 0; i < g_menuCount; i++) {
    g_menus[i].x = x;
    x += g_menus[i].width;
  }

  // 初始化示例文本
  InitSampleText();

  // 开启批量绘图
  BeginBatchDraw();
  RedrawAll();

  // 安装全局低级键盘钩子：回调在本主线程的消息循环中分发（见
  // LowLevelKeyProc）。 这是 EasyX
  // 内部窗口线程架构下主线程能稳定拿到真实击键的唯一通道。
  g_llHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyProc,
                              GetModuleHandle(NULL), 0);

  // 主消息循环
  bool running = true;
  DWORD lastDraw = GetTickCount();

  while (running) {
    bool needRedraw = false;

    // 点窗口右上角 X / Alt+F4 的关闭请求：在主线程弹保存确认。
    //   是 → 保存（新文件先弹"保存为"），成功后退出；
    //   否 → 不保存直接退出；
    //   取消 → 什么都不做，回到原编辑界面（窗口保持打开）。
    if (g_closeRequested) {
      g_closeRequested = false;
      if (ConfirmDiscardAllTabs()) {
        running = false;
      }
    }
    if (!running)
      break;

    // 拉伸/移动模态中：主线程完全静默（不 Resize、不重绘、不刷新，
    // 所有屏幕绘制由内部线程的色块 WM_PAINT 负责），杜绝跨线程竞争。
    if (!g_inSizeMove && g_layoutDirty) {
      g_layoutDirty = false;
      // 目标：拉伸路径用 EXIT 时捕获的 g_target；外部触发路径用 pending
      int tw = g_targetW > 0 ? g_targetW : g_pendingW;
      int th = g_targetH > 0 ? g_targetH : g_pendingH;
      if (tw > 0 && th > 0 && (tw != WINDOW_WIDTH || th != WINDOW_HEIGHT)) {
        UpdateLayout(tw, th);
        // 开启保护窗口：此后 250ms 内 Resize 自产的 WM_SIZE 全部忽略
        g_ignoreSizeUntil = GetTickCount() + 250;
        // 第一步：Resize 把 EasyX 帧缓冲撑到目标尺寸（必须，否则黑屏）。
        // 但 Resize 内部按 initgraph 时记录的边框换算外框，而我们在
        // initgraph 后才加入 WS_THICKFRAME（更厚），会导致实际客户区
        // 比目标小一个边框差，且会触发新 WM_SIZE 形成塌缩循环。
        Resize(NULL, tw, th);
        // 第二步：用原生 SetWindowPos 按当前真实样式反向精确校正，
        // 强制客户区 == 目标尺寸。SetWindowPos 不触碰 EasyX 缓冲
        // （缓冲保持上一步的目标尺寸），也不经过 EasyX Resize，
        // 因而不会再次塌缩。
        RECT cc;
        GetClientRect(GetHWnd(), &cc);
        if (cc.right != tw || cc.bottom != th) {
          RECT rc = {0, 0, tw, th};
          DWORD s = (DWORD)GetWindowLongPtr(GetHWnd(), GWL_STYLE);
          DWORD es = (DWORD)GetWindowLongPtr(GetHWnd(), GWL_EXSTYLE);
          AdjustWindowRectEx(&rc, s, FALSE, es);
          SetWindowPos(GetHWnd(), NULL, 0, 0, rc.right - rc.left,
                       rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER);
          // 此次 SetWindowPos 产生的 WM_SIZE 落在保护窗口内，
          // 会被忽略，不再回灌塌缩。
        }
      }
      g_targetW = g_targetH = 0; // 一次性目标，消费完清空
      needRedraw = true;
    }

    // 0) 主线程消息泵：WH_KEYBOARD_LL 钩子回调借此分发。
    //    主线程不拥有图形窗口，这里取不到任何窗口消息，
    //    主要用来驱动钩子（以及未来的线程消息）。
    {
      MSG thmsg;
      while (PeekMessage(&thmsg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&thmsg);
        DispatchMessage(&thmsg);
      }
    }

    // 1) 消息泵：本版 EasyX 的图形窗口运行在内部线程，跨线程消息只能
    //    通过 peekmessage 从内部消息环取（主线程自己 PeekMessage 取不到
    //    窗口消息，实测 PostMessage 到窗口的 WM_CHAR 主线程收不到）。
    //    必须用 0xFF 过滤器一次排空所有类别 —— 旧式 MouseHit() 等价于
    //    peekmessage(EX_MOUSE)，会把非鼠标消息（键盘/字符）整条丢弃，
    //    这曾是标点符号输入不进的第一层原因。
    //    另一个实测结论：真实击键经内部线程转发后，WM_CHAR/WM_IME_CHAR
    //    都到不了这个环（WM_IME_CHAR 被丢弃、WM_KEYDOWN 的 vkcode 为空），
    //    主线程 GetAsyncKeyState 也读不到内部窗口的击键（探针 F），所以
    //    可打印字符改由 WH_KEYBOARD_LL 钩子捕获（见主循环步骤 2）。
    ExMessage msg;
    while (peekmessage(&msg, 0xFF, true)) {
      // WM_CHAR 字符消息（内部线程 TranslateMessage 的产物，以及 IME
      // 提交的 DBCS 字节、外部程序投递的字符都走这里）。
      // 去重：英文模式下同一击键钩子也会翻译出一份字符，若钩子刚
      // 插入过（去重窗口内），丢弃环里这份，避免双字。
      if (msg.message == WM_CHAR) {
        // 帮助 / 关于窗口打开时屏蔽字符输入（模态）
        if (g_helpOpen || g_aboutOpen)
          continue;
        // 查找弹窗/替换输入框激活时：所有可打印字符进弹窗输入框（含 IME
        // 提交的中文 DBCS 字节 >= 0x80）。与轮询/钩子通道互为冗余，
        // 由 FindBoxInsertChar 内部"同字符 25ms"去重合并。
        if (g_findBoxOpen || g_findRepFocused) {
          // 钩子刚投递过同一击键则跳过（两条通道跨线程到达，
          // 同字符 25ms 中央去重兜不住时间差大的重复）
          if ((unsigned char)msg.ch >= 0x20 &&
              GetTickCount() - g_lastHookCharTick >= CHAR_DEDUP_WINDOW_MS) {
            FindBoxInsertChar((char)msg.ch);
            needRedraw = true;
          }
          continue;
        }
        if (GetTickCount() - g_lastHookCharTick >= CHAR_DEDUP_WINDOW_MS) {
          if (InsertTypedChar((char)msg.ch)) {
            g_lastRingCharTick = GetTickCount();
            needRedraw = true;
            g_modified = true;
            UpdateAutocomplete();
          }
        }
        continue;
      }
      // 只有鼠标消息进入下面的原有处理逻辑；
      // 其余消息（WM_KEYUP/窗口消息/不可靠的键盘转发）忽略
      if (msg.message < WM_MOUSEFIRST || msg.message > WM_MOUSELAST)
        continue;

      int mx = msg.x;
      int my = msg.y;
      const UINT umsg = msg.message;

      if (umsg == WM_LBUTTONDOWN) {

        // 查找结果弹窗内的点击优先处理：查找框与结果面板可同时存在，
        // 此时点击结果面板不能被误判为"查找框外部点击"而关闭查找框，
        // 否则"替换为"输入框第一次点击只会关框、第二次才聚焦，且
        // 聚焦后键盘路由（g_findBoxOpen）已断，按键全部漏进编辑器。
        if (g_findResultOpen && PtInFindResults(mx, my)) {
          if (HitFindResultClose(mx, my)) {
            g_findResultOpen = false;
            g_findRepFocused = false;
            needRedraw = true;
            continue;
          } else if (HitFindResultTitle(mx, my)) {
            g_findResultDragging = true;
            g_findResultDragOffX = mx - g_findResultX;
            g_findResultDragOffY = my - g_findResultY;
            SetCapture(GetHWnd());
            needRedraw = true;
            continue;
          } else if (HitFindResultRepInput(mx, my)) {
            // 点击"替换为"输入框：获得键盘焦点并按点击位置定位光标。
            // 绝不能关闭查找框：三条键盘输入通道都靠弹窗状态接管按键，
            // 一旦 g_findBoxOpen=false，按键会全部漏进编辑器。
            g_findRepFocused = true;
            int rrx, rry, rrw, rrh;
            GetFindResultRect(rrx, rry, rrw, rrh);
            int repIx = rrx + 55; // 输入框左边缘 x
            int repHidden = FindRepHidden();
            g_findCursor = FindBoxXToByte(g_findRepInput, mx - (repIx + 4),
                                          repHidden);
            g_findSelAnchor = -1;
            needRedraw = true;
            continue;
          } else {
            ResultBtn btn = HitFindResultBottomBtn(mx, my);
            int cbIdx = -1;
            bool hitCB = HitFindResultCheckBox(mx, my, cbIdx);
            int listIdx = HitTestFindResults(mx, my);

            if (btn != RB_NONE) {
              // 底部按钮
              g_findRepFocused = false;
              switch (btn) {
                case RB_SEL_ALL:
                  g_findResultChecked.assign(g_findResults.size(), true); break;
                case RB_UNSEL_ALL:
                  g_findResultChecked.assign(g_findResults.size(), false); break;
                case RB_REPLACE_SEL:
                  ReplaceSelectedInResults(); break;
                case RB_REPLACE_ALL:
                  ReplaceAll(); break;
              }
              needRedraw = true;
              continue;
            } else if (hitCB) {
              // 命中复选框：切换勾选
              if (cbIdx >= 0 && cbIdx < (int)g_findResultChecked.size())
                g_findResultChecked[cbIdx] = !g_findResultChecked[cbIdx];
              g_findRepFocused = false;
              needRedraw = true;
              continue;
            } else if (listIdx >= 0) {
              // 点击列表行（非复选框）：跳转定位
              g_findRepFocused = false;
              JumpToFindResult(listIdx);
              needRedraw = true;
              continue;
            } else if (PtInFindResults(mx, my)) {
              // 点在弹窗内部但什么都没命中（空白区域）：替换框失焦
              g_findRepFocused = false;
              needRedraw = true;
              continue;
            }
          }
        }

        // 查找弹窗打开时，左键优先处理弹窗（×/按钮/输入框拖选/拖动）
        if (g_findBoxOpen) {
          if (HitFindBoxClose(mx, my)) {
            CloseFindBox();
          } else if (HitFindBoxBtn(mx, my)) {
            FindBoxConfirm();
          } else if (HitFindBoxInput(mx, my)) {
            // 点击查找框：键盘焦点回到查找输入框，替换框失焦
            g_findRepFocused = false;
            // 点击查找框：定位光标，准备拖选文本
            int hidden = FindBoxHiddenBytes(g_findInput, FIND_BOX_W - 20);
            int pos =
                FindBoxXToByte(g_findInput, mx - (g_findBoxX + 15), hidden);
            g_findCursor = pos;
            g_findSelAnchor = pos;
            g_findInputDragging = true;
            SetCapture(GetHWnd());
          } else if (PtInFindBox(mx, my)) {
            // 按住弹窗其他区域：开始拖动整个弹窗
            g_findBoxDragging = true;
            g_findDragOffX = mx - g_findBoxX;
            g_findDragOffY = my - g_findBoxY;
          } else {
            // 点在两个弹窗外部：关闭查找框，替换框一并失焦
            CloseFindBox();
            g_findRepFocused = false;
          }
          needRedraw = true;
          continue; // 消耗本次点击，不透传给编辑器
        }

        // 两个弹窗都不在点击范围内：若替换框仍有焦点则失焦，按键回编辑器
        if (g_findRepFocused) {
          g_findRepFocused = false;
          needRedraw = true;
        }

        // 变量监视窗口：关闭/拖拽/输入框/查看/刷新/列表点击
        if (g_watchWindowOpen) {
          if (HitWatchClose(mx, my)) {
            g_watchWindowOpen = false;
            g_watchInputActive = false;
            needRedraw = true;
            continue;
          } else if (HitWatchTitle(mx, my)) {
            g_watchDragging = true;
            g_watchDragOffX = mx - g_watchX;
            g_watchDragOffY = my - g_watchY;
            SetCapture(GetHWnd());
            needRedraw = true;
            continue;
          } else if (HitWatchInput(mx, my)) {
            g_watchInputActive = true;
            // 点击定位光标
            int ix = g_watchX + 60;
            int c = (mx - ix - 4) / CHAR_WIDTH;
            if (c < 0) c = 0;
            if (c > g_watchInputLen) c = g_watchInputLen;
            g_watchInputCursorPos = c;
            needRedraw = true;
            continue;
          } else if (HitWatchQueryBtn(mx, my)) {
            // "查看"：设置过滤条件
            g_watchFilter = g_watchInputLen > 0 ? g_watchInputBuf : "";
            g_watchScroll = 0;
            needRedraw = true;
            continue;
          } else if (HitWatchRefreshBtn(mx, my)) {
            // "刷新"：重新读取调试输出
            ParseDebugOutput();
            g_watchScroll = 0;
            needRedraw = true;
            continue;
          } else if (PtInWatchWindow(mx, my)) {
            // 点在弹窗内但没命中任何控件：失焦
            g_watchInputActive = false;
            needRedraw = true;
            continue;
          } else {
            // 点在弹窗外部：失焦但不关闭
            g_watchInputActive = false;
          }
        }

        // 帮助窗口：关闭 / 拖拽 / 点击外部关闭
        if (g_helpOpen) {
          if (HitHelpClose(mx, my)) {
            g_helpOpen = false;
            needRedraw = true;
            continue;
          } else if (HitHelpTitle(mx, my)) {
            g_helpDragging = true;
            g_helpDragOffX = mx - g_helpX;
            g_helpDragOffY = my - g_helpY;
            SetCapture(GetHWnd());
            needRedraw = true;
            continue;
          } else {
            // 点在内容区/滚动条：仅消耗点击（滚动由滚轮处理）；
            // 点在弹窗外：关闭
            if (!PtInHelpWindow(mx, my))
              g_helpOpen = false;
            needRedraw = true;
            continue;
          }
        }

        // 关于窗口：关闭 / 拖拽 / 点击外部关闭
        if (g_aboutOpen) {
          if (HitAboutClose(mx, my)) {
            g_aboutOpen = false;
            needRedraw = true;
            continue;
          } else if (HitAboutTitle(mx, my)) {
            g_aboutDragging = true;
            g_aboutDragOffX = mx - g_aboutX;
            g_aboutDragOffY = my - g_aboutY;
            SetCapture(GetHWnd());
            needRedraw = true;
            continue;
          } else {
            if (!PtInAboutWindow(mx, my))
              g_aboutOpen = false;
            needRedraw = true;
            continue;
          }
        }

        // 右键菜单打开时，左键优先处理菜单项点击
        if (g_ctxMenuOpen) {
          int ctxIdx;
          if (HitTestContextMenu(mx, my, ctxIdx)) {
            // 命中菜单项：仅启用项（且非分隔线）执行操作
            int act = g_ctxItems[ctxIdx].action;
            if (act != 0 && g_ctxItems[ctxIdx].enabled) {
              ExecuteContextItem(act);
            }
          }
          // 无论是否命中，左键都关闭右键菜单
          g_ctxMenuOpen = false;
          g_ctxMenuHoverIdx = -1;
          needRedraw = true;
          continue; // 消耗本次点击，避免同时触发编辑区选区
        }

        // 函数跳转列表打开时：左键优先处理
        if (g_funcListOpen) {
          int fidx;
          if (HitTestFuncList(mx, my, fidx)) {
            JumpToLine(g_funcList[fidx].line);
          }
          // 无论是否命中，左键都关闭函数列表
          g_funcListOpen = false;
          g_funcListHover = -1;
          needRedraw = true;
          continue;
        }

        // 滚动条拖拽（优先级高于编辑区、菜单栏等）
        {
          int vHit = HitTestVScrollBar(mx, my);
          if (vHit > 0) {
            if (g_autocompleteOpen) CloseAutocomplete();
            if (vHit == 1) {
              // 命中滑块：开始拖拽
              g_scrollDrag = 1;
              // 计算鼠标到滑块顶部的像素差
              int trackTop = MENU_BAR_HEIGHT;
              int bottom = WINDOW_HEIGHT - STATUS_BAR_HEIGHT;
              int trackH = bottom - trackTop;
              int viewCnt = (int)g_viewToDoc.size();
              int visLines = GetVisibleLineCount();
              int thumbH = (visLines * trackH) / viewCnt;
              if (thumbH < 14) thumbH = 14;
              int maxTop = viewCnt - visLines;
              int thumbY = trackTop + (g_topLine * (trackH - thumbH)) / maxTop;
              g_scrollDragOff = my - thumbY;
            } else if (vHit == 2) {
              // 上方轨道：上翻一页
              g_topLine -= GetVisibleLineCount();
              if (g_topLine < 0) g_topLine = 0;
            } else {
              // 下方轨道：下翻一页
              g_topLine += GetVisibleLineCount();
              ClampScroll();
            }
            SetCapture(GetHWnd());
            needRedraw = true;
            continue; // 消费本次点击，不透传给编辑器
          }
          int hHit = HitTestHScrollBar(mx, my);
          if (hHit > 0) {
            if (g_autocompleteOpen) CloseAutocomplete();
            if (hHit == 1) {
              g_scrollDrag = 2;
              int sbX = EDIT_AREA_WIDTH - SCROLLBAR_W;
              int hbW = sbX - LINE_NUM_WIDTH;
              int visCols = GetVisibleColCount();
              int maxCol = 0;
              int rows = g_buffer.getRowCount();
              for (int r = 0; r < rows; r++) {
                int cc = g_buffer.getColCount(r);
                if (cc > maxCol) maxCol = cc;
              }
              int thumbW = (visCols * hbW) / maxCol;
              if (thumbW < 14) thumbW = 14;
              int maxLeft = maxCol - visCols;
              int thumbX = LINE_NUM_WIDTH + (g_leftCol * (hbW - thumbW)) / maxLeft;
              g_scrollDragOff = mx - thumbX;
            } else if (hHit == 2) {
              g_leftCol -= GetVisibleColCount();
              if (g_leftCol < 0) g_leftCol = 0;
            } else {
              g_leftCol += GetVisibleColCount();
              ClampScroll();
            }
            SetCapture(GetHWnd());
            needRedraw = true;
            continue;
          }
        }

        // 点击编辑区时关闭补全弹窗
        if (g_autocompleteOpen)
          CloseAutocomplete();

        int subIdx = HitTestSubMenu(mx, my);
        if (subIdx >= 0) {
          const char *item = g_menus[g_activeMenu].subItems[subIdx];
          // 退出：所有标签的未保存修改逐个确认，取消则不退出
          if (strcmp(item, "退出") == 0) {
            if (ConfirmDiscardAllTabs())
              running = false;
          }
          // 文件菜单
          else if (strcmp(item, "新建") == 0)
            FileNew();
          else if (strcmp(item, "打开") == 0)
            FileOpen();
          else if (strcmp(item, "保存") == 0)
            FileSave();
          else if (strcmp(item, "另存为") == 0)
            FileSaveAs();
          // 运行菜单
          else if (strcmp(item, "语法检测") == 0)
            SyntaxCheck();
          else if (strcmp(item, "编译") == 0)
            CompileCurrent();
          else if (strcmp(item, "运行") == 0)
            RunProgram();
          else if (strcmp(item, "编译并运行") == 0)
            TestCompileAndRun();
          else if (strcmp(item, "调试运行 (F5)") == 0)
            DebugRun();
          else if (strcmp(item, "诊断配置...") == 0)
            OpenDiagConfig();
          // 编辑菜单
          else if (strcmp(item, "撤销") == 0)
            EditUndo();
          else if (strcmp(item, "重做") == 0)
            EditRedo();
          else if (strcmp(item, "剪切") == 0)
            EditCut();
          else if (strcmp(item, "复制") == 0)
            EditCopy();
          else if (strcmp(item, "粘贴") == 0)
            EditPaste();
          else if (strcmp(item, "查找与替换") == 0)
            Find();
          else if (strcmp(item, "全部折叠") == 0)
            FoldAll();
          else if (strcmp(item, "全部展开") == 0)
            ExpandAll();
          else if (strcmp(item, "切换主题") == 0)
            ToggleTheme();
          else if (strcmp(item, "转到行...") == 0)
            GoToLine();
          else if (strcmp(item, "转到函数...") == 0)
            GoToFunction();
          // 帮助菜单
          else if (strcmp(item, "使用说明") == 0)
            OpenHelpWindow();
          else if (strcmp(item, "关于") == 0)
            OpenAboutWindow();
          // 其余项暂未实现
          else
            ShowNotImplemented(item);
          g_activeMenu = -1;
          g_subMenuHover = -1;
          needRedraw = true; // 文件操作后状态栏可能变化
        } else {
          int menuIdx = HitTestMenuBar(mx, my);
          if (menuIdx >= 0) {
            g_activeMenu = (g_activeMenu == menuIdx) ? -1 : menuIdx;
            g_subMenuHover = -1;
            needRedraw = true;
          } else if (g_activeMenu >= 0) {
            g_activeMenu = -1;
            g_subMenuHover = -1;
            needRedraw = true;
          } else if (my >= MENU_BAR_HEIGHT && my < TEXT_ORIGIN_Y &&
                     mx < EDIT_AREA_WIDTH) {
            // 标签栏点击：切换标签 / 关闭标签（×）
            int ti = HitTestTab(mx, my);
            if (ti >= 0) {
              if (HitTestTabClose(mx, my, ti))
                CloseTab(ti);
              else
                SwitchTab(ti);
              SetFocus(GetHWnd());
            }
            needRedraw = true;
          } else {
            // 问题面板点击：单击选中并跳到行首；双击精确定位到诊断列
            int diagIdx = HitTestDiagPanel(mx, my);
            if (diagIdx >= 0) {
              DWORD nowTick = GetTickCount();
              bool isDouble =
                  (g_diagClickIdx == diagIdx) &&
                  (nowTick - g_diagClickTick) <=
                      (DWORD)GetDoubleClickTime() &&
                  abs(mx - g_diagClickX) <= 6 && abs(my - g_diagClickY) <= 6;
              g_diagClickTick = nowTick;
              g_diagClickIdx = diagIdx;
              g_diagClickX = mx;
              g_diagClickY = my;

              g_diagSelIdx = diagIdx;
              // 双击跳到行+列；单击只跳到行首
              JumpToDiagnostic(diagIdx, isDouble);
              SetFocus(GetHWnd());
              needRedraw = true;
            } else {
              // 点在面板空白处：重置双击配对
              g_diagClickIdx = -1;
              // 断点栏点击：切换断点
              if (mx < BREAKPOINT_GUTTER_W && my >= TEXT_ORIGIN_Y &&
                       my < WINDOW_HEIGHT - STATUS_BAR_HEIGHT) {
                EnsureFolds();
                int viewCnt = (int)g_viewToDoc.size();
                int vr = (my - TEXT_ORIGIN_Y) / LINE_HEIGHT + g_topLine;
                if (vr >= 0 && vr < viewCnt) {
                  int row = g_viewToDoc[vr];
                  toggleBreakpoint(row);
                  SetFocus(GetHWnd());
                  needRedraw = true;
                }
              }
              // gutter 折叠标记点击：切换该块折叠/展开
              else {
                int foldRow = HitTestFoldGutter(mx, my);
                if (foldRow >= 0) {
                  ToggleFold(foldRow);
                  g_sel.active = false;
                  SetFocus(GetHWnd());
                  needRedraw = true;
                }
                // 行号栏点击：选中整行（高亮），不立即删除
                else if (mx < LINE_NUM_WIDTH && my >= TEXT_ORIGIN_Y &&
                         my < WINDOW_HEIGHT - STATUS_BAR_HEIGHT) {
                  EnsureFolds();
                  int viewCnt = (int)g_viewToDoc.size();
                  int vr = (my - TEXT_ORIGIN_Y) / LINE_HEIGHT;
                  if (vr >= 0 && vr < viewCnt) {
                    int row = g_viewToDoc[vr]; // 视图行→文档行
                    // 选区覆盖该整行：行首到行尾
                    g_sel.startRow = row;
                    g_sel.startCol = 0;
                    g_sel.endRow = row;
                    g_sel.endCol = g_buffer.getColCount(row);
                    g_sel.active = true;
                    // 光标移到行尾（视觉上整行被选中）
                    g_cursor.row = row;
                    g_cursor.col = g_buffer.getColCount(row);
                    g_highlightLine = -1;
                    SetFocus(GetHWnd());
                    needRedraw = true;
                  }
                }
                // 编辑区点击:移动光标并开始选区
                else if (HandleMouseClickInEditArea(mx, my)) {
                  needRedraw = true;
                }
              }
            }
          }
        }
      }
      if (umsg == WM_MOUSEMOVE && g_scrollDrag) {
        // 拖拽滚动条滑块：根据鼠标位置反算 g_topLine / g_leftCol
        EnsureFolds();
        if (g_scrollDrag == 1) {
          // 垂直拖拽
          int trackTop = MENU_BAR_HEIGHT;
          int bottom = WINDOW_HEIGHT - STATUS_BAR_HEIGHT;
          int trackH = bottom - trackTop;
          int viewCnt = (int)g_viewToDoc.size();
          int visLines = GetVisibleLineCount();
          if (viewCnt > visLines) {
            int thumbH = (visLines * trackH) / viewCnt;
            if (thumbH < 14) thumbH = 14;
            int maxTop = viewCnt - visLines;
            int travel = trackH - thumbH;
            if (travel > 0) {
              int thumbY = my - g_scrollDragOff;
              if (thumbY < trackTop) thumbY = trackTop;
              if (thumbY > trackTop + travel) thumbY = trackTop + travel;
              g_topLine = ((thumbY - trackTop) * maxTop) / travel;
              if (g_topLine < 0) g_topLine = 0;
              if (g_topLine > maxTop) g_topLine = maxTop;
            }
          }
        } else {
          // 水平拖拽
          int sbX = EDIT_AREA_WIDTH - SCROLLBAR_W;
          int hbLeft = LINE_NUM_WIDTH;
          int hbW = sbX - hbLeft;
          int visCols = GetVisibleColCount();
          int maxCol = 0;
          int rows = g_buffer.getRowCount();
          for (int r = 0; r < rows; r++) {
            int cc = g_buffer.getColCount(r);
            if (cc > maxCol) maxCol = cc;
          }
          if (maxCol > visCols) {
            int thumbW = (visCols * hbW) / maxCol;
            if (thumbW < 14) thumbW = 14;
            int maxLeft = maxCol - visCols;
            int travel = hbW - thumbW;
            if (travel > 0) {
              int thumbX = mx - g_scrollDragOff;
              if (thumbX < hbLeft) thumbX = hbLeft;
              if (thumbX > hbLeft + travel) thumbX = hbLeft + travel;
              g_leftCol = ((thumbX - hbLeft) * maxLeft) / travel;
              if (g_leftCol < 0) g_leftCol = 0;
              if (g_leftCol > maxLeft) g_leftCol = maxLeft;
            }
          }
        }
        needRedraw = true;
      } else if (umsg == WM_MOUSEMOVE && g_findInputDragging) {
        // 输入框内拖选文本：光标跟随鼠标（焦点字段，拖出边界自动夹到首/尾）
        const std::string &ft = FieldText();
        int hidden = FindBoxHiddenBytes(ft, FIND_BOX_W - 20);
        int pos = FindBoxXToByte(ft, mx - (g_findBoxX + 15), hidden);
        if (pos != g_findCursor) {
          g_findCursor = pos;
          needRedraw = true;
        }
      } else if (umsg == WM_MOUSEMOVE && g_findBoxDragging) {
        // 拖动查找弹窗：跟随鼠标，夹紧在窗口范围内
        g_findBoxX = mx - g_findDragOffX;
        g_findBoxY = my - g_findDragOffY;
        ClampFindBoxPos();
        needRedraw = true;
      } else if (umsg == WM_MOUSEMOVE && g_findResultDragging) {
        // 拖动查找结果独立弹窗
        g_findResultX = mx - g_findResultDragOffX;
        g_findResultY = my - g_findResultDragOffY;
        ClampFindResultPos();
        needRedraw = true;
      } else if (umsg == WM_MOUSEMOVE && g_watchDragging) {
        // 拖动变量监视窗口
        g_watchX = mx - g_watchDragOffX;
        g_watchY = my - g_watchDragOffY;
        ClampWatchPos();
        needRedraw = true;
      } else if (umsg == WM_MOUSEMOVE && g_watchWindowOpen &&
                 !g_watchDragging) {
        // 变量监视窗口悬停反馈
        bool newClose = HitWatchClose(mx, my);
        bool newQuery = HitWatchQueryBtn(mx, my);
        bool newRefresh = HitWatchRefreshBtn(mx, my);
        int newHover = HitWatchList(mx, my);
        bool changed = (newClose != g_watchHoverClose ||
                        newQuery != g_watchHoverQuery ||
                        newRefresh != g_watchHoverRefresh ||
                        newHover != g_watchHover);
        g_watchHoverClose = newClose;
        g_watchHoverQuery = newQuery;
        g_watchHoverRefresh = newRefresh;
        g_watchHover = newHover;
        if (changed) needRedraw = true;
      } else if (umsg == WM_MOUSEMOVE && g_helpDragging) {
        // 拖动帮助窗口
        g_helpX = mx - g_helpDragOffX;
        g_helpY = my - g_helpDragOffY;
        ClampHelpPos();
        needRedraw = true;
      } else if (umsg == WM_MOUSEMOVE && g_helpOpen) {
        // 帮助窗口 × 按钮悬停反馈
        bool nc = HitHelpClose(mx, my);
        if (nc != g_helpHoverClose) {
          g_helpHoverClose = nc;
          needRedraw = true;
        }
      } else if (umsg == WM_MOUSEMOVE && g_aboutDragging) {
        // 拖动关于窗口
        g_aboutX = mx - g_aboutDragOffX;
        g_aboutY = my - g_aboutDragOffY;
        ClampAboutPos();
        needRedraw = true;
      } else if (umsg == WM_MOUSEMOVE && g_aboutOpen) {
        // 关于窗口 × 按钮悬停反馈
        bool nc = HitAboutClose(mx, my);
        if (nc != g_aboutHoverClose) {
          g_aboutHoverClose = nc;
          needRedraw = true;
        }
      } else if (umsg == WM_MOUSEMOVE && (g_findBoxOpen || g_findResultOpen)) {
        // 查找弹窗 × / "开始查找"按钮悬停反馈
        bool hc = g_findBoxOpen ? HitFindBoxClose(mx, my) : g_findBoxHoverClose;
        bool hb = g_findBoxOpen ? HitFindBoxBtn(mx, my) : g_findBoxHoverBtn;
        // 结果弹窗：关闭按钮悬停 + 列表项悬停
        bool newRClose = g_findResultOpen ? HitFindResultClose(mx, my) : false;
        int newResHover = g_findResultOpen ? HitTestFindResults(mx, my) : -1;
        if (g_findBoxOpen &&
            (hc != g_findBoxHoverClose || hb != g_findBoxHoverBtn)) {
          g_findBoxHoverClose = hc;
          g_findBoxHoverBtn = hb;
          needRedraw = true;
        }
        if (g_findResultOpen && (newRClose != g_findResultHoverClose ||
            newResHover != g_findResultHover)) {
          g_findResultHoverClose = newRClose;
          g_findResultHover = newResHover;
          needRedraw = true;
        }
      } else if (umsg == WM_MOUSEMOVE && g_mouseSelecting) {
        // 鼠标拖拽:更新选区终点。
        // strictX=false：横向越界 clamp 到行首/行尾；
        // clampY=true：纵向越出编辑区 clamp 到首行/末行，
        // 这样向菜单栏/状态栏方向拖也能把选区扩到边界行。
        int r, c;
        if (PixelToCell(mx, my, r, c, false, true)) {
          g_cursor.row = r;
          g_cursor.col = c;
          if (r != g_selAnchorRow || c != g_selAnchorCol) {
            g_sel.active = true;
            g_sel.startRow = g_selAnchorRow;
            g_sel.startCol = g_selAnchorCol;
            g_sel.endRow = r;
            g_sel.endCol = c;
          } else {
            g_sel.active = false;
          }
          needRedraw = true;
        }
      } else if (umsg == WM_MOUSEMOVE && g_ctxMenuOpen) {
        // 右键菜单悬停高亮
        int idx;
        int newHover = -1;
        if (HitTestContextMenu(mx, my, idx)) {
          // 仅启用且非分隔线才高亮
          if (g_ctxItems[idx].action != 0 && g_ctxItems[idx].enabled) {
            newHover = idx;
          }
        }
        if (newHover != g_ctxMenuHoverIdx) {
          g_ctxMenuHoverIdx = newHover;
          needRedraw = true;
        }
      } else if (umsg == WM_MOUSEMOVE && g_funcListOpen) {
        // 函数列表悬停高亮
        int fidx;
        int newHover = -1;
        if (HitTestFuncList(mx, my, fidx))
          newHover = fidx;
        if (newHover != g_funcListHover) {
          g_funcListHover = newHover;
          needRedraw = true;
        }
      } else if (umsg == WM_MOUSEMOVE && g_activeMenu >= 0) {
        // 下拉子菜单悬停高亮：移入某项则高亮，移出（含移到菜单栏）恢复
        int newHover = HitTestSubMenu(mx, my);
        if (newHover != g_subMenuHover) {
          g_subMenuHover = newHover;
          needRedraw = true;
        }
        // 子菜单展开时菜单栏悬停也要跟踪：否则 g_menuBarHover
        // 残留旧菜单索引，切换到别的菜单后原菜单会误显悬停灰底
        int newBarHover = HitTestMenuBar(mx, my);
        if (newBarHover != g_menuBarHover) {
          g_menuBarHover = newBarHover;
          needRedraw = true;
        }
      } else if (umsg == WM_MOUSEMOVE) {
        // 顶部菜单栏悬停高亮：鼠标在菜单栏内时，对应项颜色加深
        int newHover = HitTestMenuBar(mx, my);
        if (newHover != g_menuBarHover) {
          g_menuBarHover = newHover;
          needRedraw = true;
        }
      } else if (umsg == WM_LBUTTONUP) {
        // 结束滚动条拖拽
        if (g_scrollDrag) {
          g_scrollDrag = 0;
          ReleaseCapture();
          needRedraw = true;
        }
        // 结束查找弹窗拖动
        g_findBoxDragging = false;
        // 结束结果弹窗拖动
        if (g_findResultDragging) {
          g_findResultDragging = false;
          ReleaseCapture();
          needRedraw = true;
        }
        // 结束变量监视窗口拖动
        if (g_watchDragging) {
          g_watchDragging = false;
          ReleaseCapture();
          needRedraw = true;
        }
        // 结束帮助窗口拖动
        if (g_helpDragging) {
          g_helpDragging = false;
          ReleaseCapture();
          needRedraw = true;
        }
        // 结束关于窗口拖动
        if (g_aboutDragging) {
          g_aboutDragging = false;
          ReleaseCapture();
          needRedraw = true;
        }
        // 结束输入框内拖选（零宽选区视为取消）
        if (g_findInputDragging) {
          g_findInputDragging = false;
          if (g_findSelAnchor == g_findCursor)
            g_findSelAnchor = -1;
          ReleaseCapture();
          needRedraw = true;
        }
        // 结束鼠标拖拽选区（选区保留现状）
        if (g_mouseSelecting) {
          g_mouseSelecting = false;
          ReleaseCapture();
          needRedraw = true; // 选中文字底色从拖选橙黄恢复常态浅蓝
        }
      } else if (umsg == WM_RBUTTONDOWN) {
        // 右键打开上下文菜单（仿 Dev-C++）
        // 先关闭可能已打开的顶部子菜单，避免视觉冲突
        if (g_activeMenu >= 0) {
          g_activeMenu = -1;
          g_subMenuHover = -1;
        }
        // 帮助 / 关于窗口打开时为模态：右键只关菜单不新开
        if (g_helpOpen || g_aboutOpen) {
          if (g_ctxMenuOpen) {
            g_ctxMenuOpen = false;
            g_ctxMenuHoverIdx = -1;
          }
          continue;
        }
        // 关闭旧的右键菜单（若已打开），允许在别处重新右键
        bool wasOpen = g_ctxMenuOpen;

        // 仅在编辑区内打开右键菜单
        int r, c;
        if (PixelToCell(mx, my, r, c, true)) {
          int menuH = g_ctxItemCount * g_ctxMenuItemH;
          int x = mx, y = my;
          // 防止菜单越出窗口右/下边界
          if (x + g_ctxMenuW > WINDOW_WIDTH)
            x = WINDOW_WIDTH - g_ctxMenuW;
          if (y + menuH > WINDOW_HEIGHT - STATUS_BAR_HEIGHT)
            y = WINDOW_HEIGHT - STATUS_BAR_HEIGHT - menuH;
          if (x < 0)
            x = 0;
          if (y < MENU_BAR_HEIGHT)
            y = MENU_BAR_HEIGHT;

          g_ctxMenuX = x;
          g_ctxMenuY = y;
          // 撤销/重做菜单项随双栈状态动态启用/禁用
          g_ctxItems[0].enabled = !g_undoStack.empty();
          g_ctxItems[1].enabled = !g_redoStack.empty();
          g_ctxMenuOpen = true;
          g_ctxMenuHoverIdx = -1;
          needRedraw = true;
        } else if (wasOpen) {
          // 右键在编辑区外（菜单栏/状态栏/诊断面板）：关闭旧菜单
          g_ctxMenuOpen = false;
          g_ctxMenuHoverIdx = -1;
          needRedraw = true;
        }
      }
    }

    // 2) 低级钩子击键 -> 可打印字符
    //    钩子回调只缓存 vk/scan；这里逐帧用钩子维护的键盘状态
    //    （Shift/Ctrl/Alt/Caps）经 ToAscii 按键盘布局翻译。
    //    标点键 VK 码是 VK_OEM_*（','=0xBC、'.'=0xBE、'/'=0xBF、
    //    ';'=0xBA 等），翻译后才能得到 < > : " | { } , . / ; 等符号。
    //    方向键/Backspace/Enter/Tab/Ctrl 组合等 ToAscii 不产生
    //    可打印字符，交给 HandleKeyboard() 处理。
    //
    //    中文输入法（IME）处理：当输入法处于"中文（本地）"模式时，
    //    击键由 IME 接管（拼音组词、中文全角标点），提交的字符经
    //    WM_CHAR 消息进入步骤 1 的字符分支 —— 此时钩子不再翻译，
    //    否则每个键会插入两份（ASCII 一份 + IME 的全角/汉字一份）。
    //    英文模式（或无 IME）时 WM_CHAR 收不到真实击键（探针实测），
    //    完全靠本钩子翻译。
    bool imeNativeMode = false;
    {
      HIMC himc = ImmGetContext(GetHWnd());
      if (himc) {
        // 输入法已关闭（如 Ctrl+Space 关 IME）时直接按英文处理
        if (ImmGetOpenStatus(himc)) {
          DWORD dwConv = 0, dwSentence = 0;
          if (ImmGetConversionStatus(himc, &dwConv, &dwSentence)) {
            imeNativeMode = (dwConv & IME_CMODE_NATIVE) != 0;
          }
        }
        ImmReleaseContext(GetHWnd(), himc);
      }
    }
    if (!imeNativeMode || g_findBoxOpen || g_findRepFocused) {
      LowLevelKeyEvent ev;
      while (LLDequeue(ev)) {
        if (!ev.down)
          continue;
        // 帮助 / 关于窗口打开时为模态：只排空事件，不向编辑区插入
        if (g_helpOpen || g_aboutOpen)
          continue;
        WORD out = 0;
        int r = ToAscii(ev.vk, ev.scan, g_llKb, &out, 0);
        if (r == 1 && out >= 0x20) {
          // 查找弹窗/替换框激活时：钩子翻译的字符进弹窗焦点输入框。
          // 与 HandleKeyboard 的按键轮询通道互为冗余，靠
          // FindBoxInsertChar 内部的"同字符 25ms"去重合并。
          if (g_findBoxOpen || g_findRepFocused) {
            FindBoxInsertChar((char)out);
            g_lastHookCharTick =
                GetTickCount(); // 标记钩子已投递，WM_CHAR 端据此去重
            needRedraw = true;
            continue;
          }
          // 去重：消息环刚送来过字符（内部线程 TranslateMessage
          // 正常工作时英文键都会产出 WM_CHAR），钩子不再重复插入；
          // 环万一丢消息，超过去重窗口后由钩子兜底。
          if (GetTickCount() - g_lastRingCharTick < CHAR_DEDUP_WINDOW_MS)
            continue;
          if (InsertTypedChar((char)out)) {
            g_lastHookCharTick = GetTickCount();
            g_modified = true;
            needRedraw = true;
            UpdateAutocomplete();
          }
        }
      }
    } else {
      // 中文模式：击键归 IME，钩子事件仅排空丢弃
      LowLevelKeyEvent ev;
      while (LLDequeue(ev)) {
      }
    }

    // 3) 键盘消息（光标移动 + 文本编辑）
    int oldRow = g_cursor.row;
    int oldCol = g_cursor.col;
    bool edited = HandleKeyboard();
    if (edited) {
      g_modified = true; // 任意编辑后置"已修改"
      needRedraw = true;
      EnsureCursorVisible(); // 编辑后让光标回到可见区
    } else if (g_cursor.row != oldRow || g_cursor.col != oldCol) {
      needRedraw = true;     // 光标位置变化
      EnsureCursorVisible(); // 方向键移动后跟焦
    }
    if (g_foldsNeedRedraw) { // 快捷键触发的折叠/展开
      g_foldsNeedRedraw = false;
      needRedraw = true;
      ClampScroll(); // 折叠后行数变化，夹紧滚动偏移
    }
    if (g_findBoxDirty) { // 查找弹窗键盘输入/开关（HandleKeyboard 内产生）
      g_findBoxDirty = false;
      needRedraw = true;
    }

    // 3.5) 鼠标滚轮：消费累计 delta，转为 g_topLine / g_leftCol 变化
    //   每 WHEEL_DELTA(120) 一档：垂直滚动 3 行，水平滚动 8 列。
    //   主线程统一处理，避免与渲染线程竞态。
    //   若鼠标悬停在查找结果面板上，滚轮改滚结果列表。
    if (g_wheelPending != 0 || g_hWheelPending != 0) {
      // 检查鼠标是否在查找结果面板范围内
      POINT mp;
      GetCursorPos(&mp);
      HWND hw = GetHWnd();
      ScreenToClient(hw, &mp);
      if (g_helpOpen && PtInHelpWindow(mp.x, mp.y)) {
        // 鼠标在帮助窗口：滚轮滚动帮助内容（每档 3 行）
        int vnotches = g_wheelPending / WHEEL_DELTA;
        g_wheelPending -= vnotches * WHEEL_DELTA;
        g_hWheelPending = 0;
        g_helpScroll -= vnotches * 3;
        if (g_helpScroll < 0) g_helpScroll = 0;
        if (g_helpScroll > HelpMaxScroll()) g_helpScroll = HelpMaxScroll();
        needRedraw = true;
      } else if (g_aboutOpen && PtInAboutWindow(mp.x, mp.y)) {
        // 关于窗口内容固定：仅吞掉滚轮，避免编辑区跟着滚动
        g_wheelPending = 0;
        g_hWheelPending = 0;
      } else if (g_findResultOpen && PtInFindResults(mp.x, mp.y)) {
        int vnotches = g_wheelPending / WHEEL_DELTA;
        g_wheelPending -= vnotches * WHEEL_DELTA;
        g_hWheelPending = 0; // 清除水平滚轮
        int total = (int)g_findResults.size();
        int rx, ry, rw, rh;
        GetFindResultRect(rx, ry, rw, rh);
        int visN = (rh - 24) / FIND_RESULT_ITEM_H;
        int maxScroll = total - visN;
        if (maxScroll < 0) maxScroll = 0;
        g_findResultScroll -= vnotches * 3; // 每档 3 行
        if (g_findResultScroll < 0) g_findResultScroll = 0;
        if (g_findResultScroll > maxScroll) g_findResultScroll = maxScroll;
        needRedraw = true;
      } else if (g_watchWindowOpen && PtInWatchWindow(mp.x, mp.y)) {
        // 鼠标在变量监视窗口：滚轮滚动列表
        int vnotches = g_wheelPending / WHEEL_DELTA;
        g_wheelPending -= vnotches * WHEEL_DELTA;
        g_hWheelPending = 0;
        std::vector<int> filtered = GetFilteredSnapshots();
        int total = (int)filtered.size();
        int rx, ry, rw, rh;
        GetWatchRect(rx, ry, rw, rh);
        int listTop = ry + WATCH_TITLE_H + WATCH_INPUT_H + GetOutputSectionHeight();
        int listBot = ry + rh - WATCH_BOTTOM_H;
        int visN = (listBot - listTop) / WATCH_ITEM_H;
        int maxScroll = total - visN;
        if (maxScroll < 0) maxScroll = 0;
        g_watchScroll -= vnotches * 3;
        if (g_watchScroll < 0) g_watchScroll = 0;
        if (g_watchScroll > maxScroll) g_watchScroll = maxScroll;
        needRedraw = true;
      } else {
      int linesPerNotch = 3;
      int colsPerNotch = 8;
      int vnotches = g_wheelPending / WHEEL_DELTA;
      int hnotches = g_hWheelPending / WHEEL_DELTA;
      // 保留余数，下次累加（高速滚动时单帧可能不到 120）
      g_wheelPending -= vnotches * WHEEL_DELTA;
      g_hWheelPending -= hnotches * WHEEL_DELTA;

      EnsureFolds();
      int viewCnt = (int)g_viewToDoc.size();
      int visLines = GetVisibleLineCount();
      int maxTop = viewCnt - visLines;
      if (maxTop < 0)
        maxTop = 0;
      g_topLine -= vnotches * linesPerNotch;
      if (g_topLine < 0)
        g_topLine = 0;
      if (g_topLine > maxTop)
        g_topLine = maxTop;

      int maxCol = 0;
      int rows = g_buffer.getRowCount();
      for (int r = 0; r < rows; r++) {
        int cc = g_buffer.getColCount(r);
        if (cc > maxCol)
          maxCol = cc;
      }
      int visCols = GetVisibleColCount();
      int maxLeft = maxCol - visCols + 1;
      if (maxLeft < 0)
        maxLeft = 0;
      g_leftCol += hnotches * colsPerNotch;
      if (g_leftCol < 0)
        g_leftCol = 0;
      if (g_leftCol > maxLeft)
        g_leftCol = maxLeft;

      needRedraw = true;
      } // end else（非结果面板滚轮）
    }

    // 3) 光标闪烁（每 500ms 重绘一次，保证闪烁效果）
    DWORD now = GetTickCount();
    if (now - lastDraw >= 500) {
      lastDraw = now;
      needRedraw = true;
    }

    // 4) 状态栏"文件:"区悬停 → 文件路径预览气泡
    //    用实时光标位置判定（而非仅 WM_MOUSEMOVE），鼠标移出窗口/移到
    //    其他区域时气泡能立即消失；右键菜单或拖拽选区时不显示。
    {
      static bool s_tipVisible = false; // 上一帧气泡是否可见
      bool over = false;
      POINT pt;
      if (!g_ctxMenuOpen && !g_mouseSelecting && GetCursorPos(&pt)) {
        ScreenToClient(GetHWnd(), &pt);
        RECT cc;
        GetClientRect(GetHWnd(), &cc);
        if (PtInRect(&cc, pt) && pt.y >= WINDOW_HEIGHT - STATUS_BAR_HEIGHT &&
            pt.x >= 0 && pt.x < STATUS_FILE_REGION_W) {
          over = true;
        }
      }
      if (over != g_statusFileHover) {
        g_statusFileHover = over;
        g_statusHoverTick = now;
      }
      // 气泡可见性跳变（进入/满 500ms 弹出/离开消失）时才重绘
      bool tipNow = over && (now - g_statusHoverTick >= 500);
      if (tipNow != s_tipVisible) {
        s_tipVisible = tipNow;
        needRedraw = true;
      }
    }

    if (needRedraw && !g_inSizeMove)
      RedrawAll();

    // 非拉伸状态下每帧把批量缓冲区刷到屏幕：
    // 窗口被拖出屏幕外再拖回时 Windows 会使窗口失效（WM_PAINT），
    // 每帧 FlushBatchDraw()（仅一次 BitBlt，开销极低）确保内容恢复。
    // 拉伸模态中绝不 Flush —— 否则旧帧会覆盖内部线程刚画的色块，
    // 这正是之前残影/双状态栏的根因。
    if (!g_inSizeMove)
      FlushBatchDraw();

    Sleep(10);
  }

  // 卸载低级键盘钩子
  if (g_llHook) {
    UnhookWindowsHookEx(g_llHook);
    g_llHook = NULL;
  }

  EndBatchDraw();
  closegraph();
  return 0;
}
