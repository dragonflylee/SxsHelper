### 编译方法
1. 微软官网下载[WDK7.1](http://download.microsoft.com/download/4/A/2/4A25C7D5-EFBE-4182-B6A9-AE6850409A78/GRMWDK_EN_7600_1.ISO)并安装Build Environments组件
2. 如果要编译64位程序,打开开始菜单 -> Windows Driver Kits -> WDK 7600.16385.1 -> Build Environments -> Windows 7 -> x64 Free Build Environment
3. 使用 CD /D 命令切换到代码所在目录(比如 `CD /D D:\SxsHelper`)
4. 输入 `build -c` 回车后开始编译，在 bin 目录中便能找到编译好的文件
