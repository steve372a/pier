---
title: Pier 软件包管理器
date: 2022-08-29 23:50:25
---
<center>
<div style="width:100px; height:100px; border-radius:100%; overflow:hidden;">
    <img src="https://steve372a.github.io/images/avatar.gif" alt>
</div>
<br>
<font size="6"><center>？、アイドル宣言</center></font>
<br>
<font size="3"><center>一个非常自由的软件包管理器</center></font>
<br>

<a href="/images/">返回到首页</a>

<br>

</center>

自己造的轮子哦！
pier(Windows Package Installer) 是一个 Windows 批处理 构成的软件包管理器，他轻量、便捷，但是不安全。所以，请仔细甄别网上的 Pier 命令。
<font color=red>该轮子制作者：？、アイドル宣言
</font>

## 安装软件

这里以 qgenerator 为例子，CMD 窗口输入：
```
pier install qgenerator
```
就这样，就能安装好qgenerator。

## 使用方法
```
本软件可以完全自由的使用，上传元数据到官方源请访问网页：
< https://gitee.com/steve372/pier/tree/master/sources >

pier install <packagename> - 安装选定的软件包。
pier remove <packagename> - 卸载选定的软件包。
pier --setlang set <language> - 设置 Package Installer 的语言。
pier --setlang install <language> - 从源镜像中下载其他语言。
pier list offline - 从本地拉取可用软件包列表。
pier list online - 从远程服务器拉取可用软件包列表。
pier list piersourcecode - 查看 Package Installer 的源代码。
pier license - 阅读许可证。
pier --help - 显示该帮助。
```
## 卸载软件

卸载软件也很简单，CMD 窗口输入：
```
pier remove qgenerator
```
这样，qgenerator就卸载了。

## 版本：2.0.0 Release
发布时间：2026 年 2 月 5 日 18:02
1.将配置文件从行号识别改为正则表达式搜索。
2.提示安装界面改为 mshta，仍兼容 Windows XP。
3.pie 包从可执行文件变为 ZIP 包，更安全。
4.新增安全审计，若检测到非法命令，就不执行自动运行（autorun）
5.新增配套建立包的 mshta 应用程序。
6.完善软件。
下载：[点击此处下载](https://steve372a.github.io/pier/pier-2.0.0-release.zip)

# 旧版本的 Package Installer（将不再更新）

**<font color=red> 注意！因为架构大改变，2.0.0b1 及以下版本无法继续使用。 </font>**

## 版本：2.0.0 Beta 1
发布时间：2025 年 05 月 27 日 01:59
1.更新支持中英文简介。
2.优化安装时的显示。
3.支持搜索软件包，使用 pier search xx
4.优化获取下载源的方式，支持本地部署。
5.pier sources 可选官方源和雪碧镜像源
6.新增 pier sources 选项，可以查看目前的源
7.新增加了语言参数的简写版，现在可以使用 pier sl 参数了。
8.优化了安装、卸载、更新元数据时的运行速度。
9.调用语言时，现在会调用 lang.ini 里的数据
10.优化数据包里的内容。
11.可以换源，支持检测源镜像规范信息文件 info.sque。
12.支持展示源镜像主人。
13.废弃 pier list offline 选项。
14.更改了 pier list online 的格式。
下载：[点击此处下载](https://steve372a.github.io/pier/pier-2.0.0-beta1.zip)

## 版本：1.0.2 Beta 1
发布时间：2025 年 05 月 20 日 16:09
1.优化获取下载源的方式，支持本地部署。（旧版无法使用）
2.新增 pier repo 选项，可以查看目前的源
3.新增加了语言参数的简写版，现在可以使用 pier sl 参数了。
4.优化了安装、卸载、更新元数据时的运行速度。
下载：[点击此处下载](https://steve372a.github.io/pier/pier-1.0.2-beta1.zip)

## 版本：1.0.1 Release
发布时间：2025 年 05 月 20 日 03:44
**时隔三年的更新！~**
1.支持参数加 -y;yes;y 跳过询问是否安装
2.遗弃 pier GUI 版
3.添加 piercmd，作为原来 piergui 的代替。
4.默认源改为个人小站 https://steve372a.github.io。原 Gitee 源废弃
5.可以通知用户软件是否会创建桌面快捷方式。
6.更新 Offline 软件包列表。
7.更改软件作者名为现在的名字：Sanakaprix
下载：[点击此处下载](https://steve372a.github.io/pier/pier-1.0.1-release.zip)

## 版本：1.0.0(Package Installer release)
发布时间：2022 年 08 月 31 日
1.支持拉取软件包数据。
2.从这个版本开始支持卸载软件
3.增加该软件遵循的许可证。
4.可快速查看 Windows Package Installer 的源代码。
下载：[点击此处下载](https://steve372a.github.io/pier/pier-1.0.0-release.zip)

## 版本：0.1.0(Package Installer beta 2)
发布时间：2022 年 08 月 30 日
支持切换语言和下载新语言。
下载：[点击此处下载](https://steve372a.github.io/pier/pier-0.1.0.zip)

## 版本：0.0.1(Package Installer beta 1)
发布时间：2022 年 08 月 29 日
开始构建
下载：[点击此处下载](https://steve372a.github.io/pier/pier-0.0.1.zip)

# 最后
感谢各位的使用，谢谢你们。
