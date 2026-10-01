# Pokémon FireRed / LeafGreen 3DS

面向 Nintendo 3DS 的《宝可梦 火红／叶绿》原生移植项目。

默认构建提供双屏图形与输入演示。仓库同时保留两种原生移植实现，目前都不能作为完整可玩版本，也未验证真机运行、中文或 Pokémon HOME 传输。仓库不包含商业 ROM 或原版游戏素材。

| 实现 | 源码位置与当前能力 |
| --- | --- |
| 原生兼容层与绘图实现 | 位于仓库根目录；可选原版启动目标已到达标题，主菜单尚未实现。[参考绘图依赖补丁](ports/native-startup/README.md) |
| 复用完整 FireRed 双屏实现 | [3DS 适配补丁和构建说明](ports/firered-dualscreen/README.md)；固定上游版本，在 Azahar 到达标题，下屏界面尚未接入 |

两种实现独立保存。下面的默认构建命令运行图形演示；双屏 FireRed 适配按其独立说明构建。Windows 构建尚未验证。

## 构建与运行

需要 Git、Make 和 devkitPro 的 `3ds-dev` 工具链，当前目标使用 devkitARM 和 libctru。系统工具链需自行安装，项目脚本不会自动执行系统安装。

在仓库根目录运行：

```sh
make build-3ds
```

构建输出为 `platform/3ds/frlg-3ds-p2c3.3dsx` 和对应的 `.smdh`。可使用兼容的 3DS homebrew 启动器或 Azahar 打开 `.3dsx`；当前没有真机可用性保证。

演示中，方向键滚动上屏背景，A 键切换图层优先级，下屏显示按键和触控状态，同时按 X + Y 退出。更多操作说明见 [3DS 程序说明](platform/3ds/README.md)。

## 依赖与素材

第三方源码通过 Git 子模块固定，来源见 [.gitmodules](.gitmodules)，版本见 [源码依赖清单](config/source-dependencies.tsv)。需要恢复子模块时运行 `git submodule update --init`，第三方代码保留各自的许可与版权声明。

仓库不提供商业 ROM、个人存档或从 ROM 提取的资源。当前演示使用项目自有图形，无需提供 ROM。
