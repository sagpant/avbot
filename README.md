#  avbot = 聊天机器人（QQ云秘书）[![Build Status](https://travis-ci.org/avplayer/avbot.png?branch=master)](https://travis-ci.org/avplayer/avbot)

avbot 连通 IRC、XMPP 和 QQ群, 并作为 AVIM 群机器人实现 AVIM 群聊功能. 能实时记录聊天信息。每日自动生成新的日志文件。

使用方法和介绍参考 [社区维基的avbot介绍](http://wiki.avplayer.org/avbot)

编译请参考 [社区维基的avbot编译指导](http://wiki.avplayer.org/%E7%BC%96%E8%AF%91avbot)

想了解 avbot 最重要的子模块 libwebqq 请点开 libwebqq 目录查看其 README.md

# 支持的系统

cmake >= 3.0

## GCC 系

centos >= 7

ubuntu >= 14.04

debian >= 7

和其他一些 gcc >= 4.8 的系统。

## MSVC 系

VisutalStudio 2013 (支持 Vista 以上系统)

VisutalStudio 2013 - vc120_xp toolset （支持 Windows XP 以上系统）

 启用步骤

  > cmake -G "VisualStudio 12 2013 Win64" -T "vc120_xp"

## icc 系
  icc >= 14

## clang 系
  clang >= 3.4

# 编译注意事项

请不要在源码文件夹里直接执行 cmake. 务必创建一个专用的文件夹存放编译中间文件，如建立个 build 文件夹。
然后在 build 文件夹里执行 cmake PATH_TO_AVBOT

因为 cmake 有很多时候，需要删除 build 文件夹重新执行，而在源码内部直接 cmake ，则因为文件夹混乱，不好清除中间文件

## boost 相关

boost 需要至少 1.57 版本。

boost 请静态编译， gentoo 用户注意 USE=static-libs emerge boost

win 下, boost 请使用 link=static runtime-link=static 执行静态编译 (包括 mingw 下)。

linux 下如果必须自己编译 boost 的话，请使用参数 link=static runtime-link=shared --layout=system variant=release --prefix=/usr 执行编译。

link=static 表示编译为静态库， runtime-link=static 则表示，应用程序最终会使用静态链接的 C++ 运行时。这个在 windows 平台是必须的要求。因为 VC 的 C 和 C++ 运行时打包起来非常麻烦。（mingw 的也一样）

linux 那边 runtime-link=shared 表示使用动态链接的 libstdc++.so， libstdc++.so 无需静态链接，不是么 ;)

添加 --layout=system variant=release 才能编译出 libbosot_context.a 这样的不带各种后缀的库版本。

## MSVC 相关

理论上 2012 版本也是支持的，不过没有测试过。

cmake 生成好 VC 工程然后打开 avbot.sln 即可。

如果 boost 在 c:/boost 则无需额外设置
如果不是，需要设定 BOOST_ROOT, 可以在 cmake-gui 里点 configure 按钮前，通过 "Add Enytry" 按钮添加。

# 关于历史

avbot 的历史剧烈膨胀, 达到 127MB 之巨, 已经严重影响到国内用户执行 git clone 了. 正好 avbot 经历了一次重构, 因此重构后 avbot 丢弃全部历史轻装上阵.
当然历史并非真的丢弃, 已经有一份完整的历史在 https://github.com/microcai/avbot 备份了.

# 关于商业开发

avbot 提供一份[商业授权](/COPYING.Commerical). 因为 avbot 对 XMPP 协议的支持是使用的 gloox, gloox 是个GPL授权的库. 因此 xmpp 支持会被禁用.
除非你同时购买了 gloox 的商业授权.

购买商业授权后, 您可以:

	1. 修改avbot的代码并无需公开自己的修改

	2. 获得为期一年的技术支持 (可续)

	3. 将 avbot 集成到自己的商业产品中

