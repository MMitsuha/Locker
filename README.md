# Locker

## 描述

绕过杀软(静态扫描)

## 构建

克隆项目

```bash
git clone https://github.com/caizhe666/Locker.git
```

安装 `vcpkg`

```bash
git clone https://github.com/microsoft/vcpkg
.\vcpkg\bootstrap-vcpkg.bat
.\vcpkg\vcpkg integrate install
```

安装 `xmake`

```powershell
Invoke-Expression (Invoke-Webrequest 'https://xmake.io/psget.text' -UseBasicParsing).Content
```

执行编译

```bash
xmake
```

## 工作原理

杀软通过`字符串扫描`,`哈希验证`和`导入表扫描`来判断一个程序是否携带病毒

### 字符串扫描

简单来说就是扫描程序中所有字符串,看其中是否含有恶意网址或者类似于`\\.\PhysicalDrive`这样不怀好意的字符串,如果有,嫌疑大大增加

### 哈希验证

没什么好说的,就是做个SHA256之类的比较

### 导入表扫描

PE(可执行文件)中含有一个`导入表`用于程序重定位后方便调用外部函数而生的,里面包含了程序中`静态链接`的函数名,以及这些函数所在的Dll(动态链接库)名,杀软通过扫描这些表,加上他们的调用顺序,以此判断程序是否恶意
如:

```
CreateFileW (参数: "\\.\PhysicalDrive0")
WriteFile (紧跟着CreateFileW)
```

就可以判断这个程序大概率会修改扇区,再加上没有签名,**必杀**

### 绕过

通过`lazy-importer`,动态调用函数,不在导入表留下信息

```
LI_FN(ReadFile)(DiskHandle, Sector, static_cast<DWORD>(sizeof(Sector)), &Bytes, nullptr)
```

通过`XorString`加密字符串

```
XorStrW(L"\\\\.\\PhysicalDrive0")
```
